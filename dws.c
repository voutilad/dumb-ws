/*
 * Copyright (c) 2020 Dave Voutila <voutilad@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include <err.h>
#include <errno.h>

#include <netdb.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/socket.h>

#include "dws.h"

// It's ludicrous to think we'd have a server handshake response larger
#define HANDSHAKE_BUF_SIZE 1024

static const char server_handshake[] = "HTTP/1.1 101 Switching Protocols";

static const char HANDSHAKE_TEMPLATE[] =
    "GET %s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Upgrade: websocket\r\n"
    "Connection: upgrade\r\n"
    "Sec-WebSocket-Key: %s\r\n"
    "Sec-WebSocket-Protocol: dumb-ws\r\n"
    "Sec-WebSocket-Version: 13\r\n\r\n";

static const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
 * This is the dumbest 16-byte base64 data generator.
 *
 * Since RFC-6455 says we don't care about the random 16-byte value used
 * for the key (the server never decodes it), why bother actually writing
 * propery base64 encoding when we can just pick 22 valid base64 characters
 * to make our key?
 */
static void
dumb_key(char *out)
{
	int i, r;

	/* 25 because 22 for the fake b64 + == + NULL */
	memset(out, 0, 25);
	for (i=0; i<22; i++) {
		r = arc4random_uniform(sizeof(B64) - 1);
		out[i] = B64[r];
	}
	out[22] = '=';
	out[23] = '=';
	out[24] = '\0';
}

/*
 * As the name implies, it just makes a random mask for use in our frames.
 *
 * TODO: change this to just return a new mask, not populate one.
 */
static void
dumb_mask(int8_t mask[4])
{
	uint32_t r;
	r = arc4random();
	mask[0] = r >> 24;
	mask[1] = (r & 0x00FF0000) >> 16;
	mask[2] = (r & 0x0000FF00) >> 8;
	mask[3] = (r & 0x000000FF);
}

/*
 * Initialize a frame buffer, returning the payload offset
 */
static ssize_t
init_frame(uint8_t *frame, enum FRAME_OPCODE type, uint8_t mask[4], size_t len)
{
	int idx = 0;
	uint16_t payload;

	// Just a quick safety check: we don't do large payloads
	if (len > (1 << 24))
		return -1;

	frame[0] = 0x80 + type;
	if (len < 126) {
		// The trivial "7 bit" payload case
		frame[1] = 0x80 + (uint8_t) len;
		idx = 1;
	} else {
		// The "7+16 bits" payload len case
		frame[1] = 0x80 + 126;

		// Payload length in network byte order
		payload = htons(len);
		frame[2] = payload & 0xFF;
		frame[3] = payload >> 8;
		idx = 3;
	}
	// And that's it, because 2^24 bytes should be enough for anyone!

	// Gotta send a copy of the mask
	frame[++idx] = mask[0];
	frame[++idx] = mask[1];
	frame[++idx] = mask[2];
	frame[++idx] = mask[3];

	return idx;
}

#ifdef DEBUG
static void
dump_frame(uint8_t *frame, size_t len)
{
	size_t i;
	int first = 1;

	for (i = 0; i < len; i++) {
		printf("0x%02x ", frame[i]);
		if (!first && (i+1) % 4 == 0)
			printf("\n");
		first = 0;
	}
	printf("\n");
}
#endif

/*
 * dumb_frame
 *
 * Construct a Binary frame containing a given payload
 *
 * Parameters:
 *  (out) frame: pointer to a buffer to write the frame data to
 *  data: pointer to the binary data payload to frame
 *  len: length of the binary payload to frame
 *
 * Assumptions:
 *  - you've properly sized the destination buffer (*out)
 *
 * Returns:
 *  size of the frame in bytes,
 * -1 when len is too large
 *
 * For reference, this is what frames look like per RFC-6455 sec. 5.2:
 *
 *     0                   1                   2                   3
 *     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *    +-+-+-+-+-------+-+-------------+-------------------------------+
 *    |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
 *    |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
 *    |N|V|V|V|       |S|             |   (if payload len==126/127)   |
 *    | |1|2|3|       |K|             |                               |
 *    +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
 *    |     Extended payload length continued, if payload len == 127  |
 *    + - - - - - - - - - - - - - - - +-------------------------------+
 *    |                               |Masking-key, if MASK set to 1  |
 *    +-------------------------------+-------------------------------+
 *    | Masking-key (continued)       |          Payload Data         |
 *    +-------------------------------- - - - - - - - - - - - - - - - +
 *    :                     Payload Data continued ...                :
 *    + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
 *    |                     Payload Data continued ...                |
 *    +---------------------------------------------------------------+
 */
static ssize_t
dumb_frame(uint8_t *frame, uint8_t *data, size_t len)
{
	size_t i, offset;
	uint8_t mask[4] = { 0, 0, 0, 0 };

	// Just a quick safety check: we don't do large payloads
	if (len > (1 << 24))
		return -1;

	// Pretend we're in Eyes Wide Shut
	dumb_mask(mask);

	offset = init_frame(frame, BINARY, mask, len);
	for (i = 0; i < len; i++) {
		// We just transmit in host byte order, someone else's problem
		frame[++offset] = data[i] ^ mask[i % 4];
	}
	frame[++offset] = '\0';

	return offset;
}

/*
 * dumb_handshake
 *
 * Take an existing, connected socket and do the secret websocket fraternity
 * handshake to prove we are a dumb websocket client.
 *
 * Parameters:
 *  s: file descriptor for a connected socket
 *  host: string representing the hostname
 *  path: the uri path, like "/" or "/dumb"
 *
 * Returns:
 *  0 on success,
 * -1 if it failed to generate the handshake buffer,
 * -2 if it failed to send the handshake,
 * -3 if it failed to receive the response,
 * -4 if the response was invalid.
 *  (in all cases, check errno)
 */
int
dumb_handshake(int s, char *host, char *path)
{
	int ret;
	ssize_t len;
	char key[25];
	uint8_t *buf;

	buf = calloc(sizeof(uint8_t), HANDSHAKE_BUF_SIZE);
	if (!buf)
		err(errno, "calloc");

	memset(key, 0, sizeof(key));
	dumb_key(key);

	ret = snprintf(buf, HANDSHAKE_BUF_SIZE, HANDSHAKE_TEMPLATE,
	    path, host, key);
	if (ret < 1) {
		free(buf);
		return -1;
	}

	len = send(s, buf, ret, 0);
	if (len < 1) {
		free(buf);
		return -2;
	}

	memset(buf, 0, HANDSHAKE_BUF_SIZE);
	len = recv(s, buf, HANDSHAKE_BUF_SIZE, 0);
	if (len < 1) {
		free(buf);
		return -3;
	}

	/* XXX: If we gave a crap, we'd validate the returned key per the
	 * requirements of RFC-6455 sec. 4.1, but we don't.
	 */
	ret = memcmp(server_handshake, buf, sizeof(server_handshake) - 1);
	if (ret)
		ret = -4;

	free(buf);
	return ret;
}

/*
 * dumb_connect
 *
 * Ugh, just connect to a host/port, ok? This just simplifies some of the
 * setup of a socket connection, so is totally optional.
 *
 * Parameters:
 *  host: hostname or ip address a string
 *  port: tcp port number
 *
 * Returns:
 *  int fd for a new socket connected to given host/port,
 * -1 if it failed to create a socket,
 * -2 if it failed to resolve host (check h_errno),
 * -3 if it failed to connect.
 */
int
dumb_connect(char *host, int port)
{
	int s;
	struct sockaddr_in addr;
	struct hostent *hostinfo;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		return -1;

	hostinfo = gethostbyname(host);
	if (hostinfo == NULL)
		return -2;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = hostinfo->h_addrtype;
	addr.sin_len = hostinfo->h_length;
	addr.sin_port = htons(port);
	memcpy(&addr.sin_addr, hostinfo->h_addr_list[0], hostinfo->h_length);

	if (connect(s, (struct sockaddr*) &addr, sizeof(addr)))
		return -3;

	return s;
}

ssize_t
dumb_send(int s, uint8_t *payload, size_t len)
{
	uint8_t *frame;
	uint8_t mask[4];
	size_t frame_len;
	ssize_t n;

	// We need payload + 14 bytes minimum, but pad a little extra
	frame = calloc(sizeof(uint8_t), len + 16);
	if (!frame)
		return -1;

	memset(mask, 0, sizeof(mask));
	dumb_mask(mask);

	frame_len = dumb_frame(frame, payload, len);
	n = send(s, frame, frame_len, 0);

	free(frame);
	return n;
}

ssize_t
dumb_recv(int s, uint8_t *out, size_t len)
{
	uint8_t *buf;
	ssize_t n;

	buf = calloc(sizeof(uint8_t), len + 16);
	if (!buf)
		return -1;

	n = recv(s, buf, len + 15, 0);
	if (n < 1) {
		free(buf);
		return -2;
	}

	memcpy(out, buf, n);

	free(buf);
	return n;
}

/*
 * dumb_ping
 *
 */
int
dumb_ping(int s)
{
	ssize_t len;
	uint8_t mask[4];
	uint8_t frame[64];

	memset(frame, 0, sizeof(frame));
	dumb_mask(mask);

	len = init_frame(frame, PING, mask, 0);
	frame[++len] = '\0';

	len = send(s, frame, len, 0);
	if (len < 1)
		return -1;

	memset(frame, 0, sizeof(frame));
	len = recv(s, frame, len, 0);
	if (len < 1)
		return -2;

#ifdef DEBUG
	dump_frame(frame, len);
#endif

	if (frame[0] != (0x80 + PONG))
		return -3;

	return 0;
}

/*
 * dumb_close
 *
 * Sadly, websockets have some "close" frame that some servers expect. If a
 * client disconnects without sending one, they sometimes get snippy.
 *
 * Parameters:
 *  s: socket descriptor to close
 *
 * Returns:
 *  0 on success,
 * -1 on failure to send the close frame,
 * -2 on failure to receive a response,
 * -3 on failure to receive a valid close response
 */
int
dumb_close(int s)
{
	ssize_t len;
	uint8_t mask[4];
	uint8_t frame[128];

	memset(frame, 0, sizeof(frame));
	dumb_mask(mask);

	len = init_frame(frame, CLOSE, mask, 0);
	frame[++len] = '\0';

	len = send(s, frame, len, 0);
	if (len < 1)
		return -1;

	memset(frame, 0, sizeof(frame));
	len = recv(s, frame, len, 0);
	if (len < 1)
		return -2;

	if (frame[0] != (0x80 + CLOSE))
		return -3;

	return 0;
}
