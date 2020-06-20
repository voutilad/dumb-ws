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

static char LONG_MSG[] =
    "{\"name\": \"dave\", \"age\": 99,"
    " \"stuff\": [1, 2, 3, 4, 5],"
    " \"more_stuff\": { \"ok\": true },"
    " \"more_and_more\": [ { \"name\": \"maple\" } ],"
    " \"date\": \"2020-06-18T12:34:56\" }";

static char SHORT_MSG[] = "{\"msg\": \"websockets are dumb\"}";

/*
 * This is the dumbest 16-byte base64 data generator.
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

static void
dumb_mask(int8_t *mask)
{
	uint32_t r;
	r = arc4random();
	mask[0] = r >> 24;
	mask[1] = (r & 0x00FF0000) >> 16;
	mask[2] = (r & 0x0000FF00) >> 8;
	mask[3] = (r & 0x000000FF);
}

/*
 * dumb_frame
 *
 * Frame some data for a crime it did not commit.
 *
 * Assumptions:
 *   - sending data as binary frames
 *   - all data is sent in 1 frame...simple, but no splitting/chunking
 *
 * Parameters:
 *   - out: pointer to a buffer to write the frame data to
 *   - out_len: size of the out buffer to make sure we don't exceed it,
 *              should include the null byte at the end!
 *   - data: pointer to the binary data payload to frame
 *   - len: length of the binary payload (note that you probably don't
 *          want't to include any null terminator)
 *
 * Returns:
 *   - size of the frame in bytes
 *
 * For reference, this is what frames look like:
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
 8    | Masking-key (continued)       |          Payload Data         |
 *    +-------------------------------- - - - - - - - - - - - - - - - +
 *    :                     Payload Data continued ...                :
 *    + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
 *    |                     Payload Data continued ...                |
 *    +---------------------------------------------------------------+
 */
size_t
dumb_frame(uint8_t *out, size_t out_len, char *data, size_t len)
{
	int idx = 0;
	uint16_t payload;
	size_t i;
	int8_t mask[4];

	// Just a quick safety check...
	if (len > out_len || len > (1 << 24))
		errx(1, "frame_it");

	memset(mask, 0, sizeof(mask));
	dumb_mask(mask);

	// We always set the same first 8 bits w/ binary frame opcode
	out[0] = 0x82;

	if (len < 126) {
		// the trivial 7 bit payload len case
		out[1] = 0x80 + (uint8_t) len;
		idx = 1;
	} else {
		// the 7+16 bits payload len case
		out[1] = 0x80 + 126;

		// payload length in network byte order
		payload = htons(len);
		out[2] = payload & 0xFF;
		out[3] = payload >> 8;
		idx = 3;
	}
	// and that's it, because 2^24 bytes should be enough for anyone

	out[++idx] = mask[0];
	out[++idx] = mask[1];
	out[++idx] = mask[2];
	out[++idx] = mask[3];

	for (i = 0; i < len; i++) {
		// We just transmit in host byte order cause YOLO
		out[++idx] = data[i] ^ mask[i % 4];
	}

	out[++idx] = '\0';
	return idx;
}

/*
 * dumb_handshake
 *
 * Take an existing, connected socket (s) and do the secret websocket
 * fraternity handshake.
 */
int
dumb_handshake(int s, char *host, char *path)
{
	int ret;
	size_t len;
	char key[25];
	uint8_t *buf;

	buf = calloc(sizeof(uint8_t), HANDSHAKE_BUF_SIZE);
	if (!buf)
		err(errno, "calloc");

	memset(key, 0, sizeof(key));
	dumb_key(key);

	len = snprintf(buf, HANDSHAKE_BUF_SIZE, HANDSHAKE_TEMPLATE,
	    path, host, key);
	if (len < 1) {
		free(buf);
		err(errno, "snprintf");
	}

	len = send(s, buf, len, 0);
	if (len < 1) {
		free(buf);
		errx(3, "send");
	}

	memset(buf, 0, HANDSHAKE_BUF_SIZE);
	len = recv(s, buf, HANDSHAKE_BUF_SIZE, 0);
	if (len < 1) {
		free(buf);
		err(errno, "recv");
	}

	// XXX: if we gave a crap, we'd validate the returned key

	ret = memcmp(server_handshake, buf, sizeof(server_handshake) - 1);

	free(buf);
	return ret;
}

/*
 * dumb_connect
 *
 * Ugh, just connect to a host/port, ok?
 */
int
dumb_connect(char *host, int port)
{
	int s;
	struct sockaddr_in addr;
	struct hostent *hostinfo;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		err(errno, "socket");

	hostinfo = gethostbyname(host);
	if (hostinfo == NULL)
		err(1, "gethostbyname");

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = hostinfo->h_addrtype;
	addr.sin_len = hostinfo->h_length;
	addr.sin_port = htons(port);
	memcpy(&addr.sin_addr, hostinfo->h_addr_list[0], hostinfo->h_length);

	if (connect(s, (struct sockaddr*) &addr, sizeof(addr)))
		err(errno, "connect");

	return s;
}

int
main()
{
	int i, s;
	size_t len;
	uint8_t *buf;

	size_t max_frame = 1024 * 8;
	char *msgs[] = { LONG_MSG, SHORT_MSG };
	size_t sizes[] = { sizeof(LONG_MSG), sizeof(SHORT_MSG) };

	buf = calloc(sizeof(uint8_t), max_frame);
	if (buf == NULL)
		err(1, "calloc");

	s = dumb_connect("localhost", 8000);

	if (dumb_handshake(s, "localhost", "/"))
		err(1, "handshake");


	for (i = 0; i < 2; i++) {

		len = dumb_frame(buf, max_frame, msgs[i], sizes[i]);
		printf("Sending a frame with %lu bytes...\n", len);

		len = send(s, buf, len, 0);
		if (len < 1)
			err(1, "send");

		printf("Listening for response...\n");

		memset(buf, 0, max_frame);
		len = recv(s, buf, max_frame, 0);

		printf("Got %lu bytes...\n", len);
		printf("  header bytes: 0x%02x 0x%02x\n", buf[0], buf[1]);
		printf("  raw message:\n%s,\n", buf);

		memset(buf, 0, max_frame);
	}

	if (shutdown(s, SHUT_RDWR))
		err(5, "shutdown");

	return 0;
}
