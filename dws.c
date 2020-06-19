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

static char msg[] = "Hey dude!";

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
frame_it(uint8_t *out, size_t out_len, char *data, size_t len)
{
	int idx = 0;
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
		printf("XXX trivial, len : %lu\n", len);
		// the trivial 7 bit payload len case
		out[1] = 0x80 + (uint8_t) len;
		printf("xxxxx: 0x%x\n", out[1]);
		idx = 1;
	} else {
		// the 7+16 bits payload len case
		out[1] = (char) 0x80 + 126;

		// payload length in network byte order
		out[2] = len >> 8;
		out[3] = len & 0x00FF;
		idx = 3;
	}

	out[++idx] = mask[0];
	out[++idx] = mask[1];
	out[++idx] = mask[2];
	out[++idx] = mask[3];

	for (i = 0; i < len; i++) {
		out[++idx] = data[i] ^ mask[i % 4];
	}

	out[++idx] = '\0';
	return idx;
}


int
main()
{
	int ret, s;
	char key[25];
	uint8_t buf[1024];
	char *hostname = "localhost";
	size_t len;

	struct sockaddr_in addr;
	struct hostent *hostinfo;

	// Pre-generate our dumb key
	memset(key, 0, sizeof(key));
	dumb_key(key);

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		err(1, "socket");

	hostinfo = gethostbyname(hostname);
	if (hostinfo == NULL)
		err(2, "gethostbyname");

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = hostinfo->h_addrtype;
	addr.sin_len = hostinfo->h_length;
	addr.sin_port = htons(8000);
	memcpy(&addr.sin_addr, hostinfo->h_addr_list[0], hostinfo->h_length);

	ret = connect(s, (struct sockaddr*) &addr, sizeof(addr));
	if (ret)
		err(errno, "connect");

	memset(buf, 0, sizeof(buf));
	len = snprintf(buf, sizeof(buf), HANDSHAKE_TEMPLATE, "/", hostname, key);

	printf("Going to send:\n%s\n", buf);
	len = send(s, buf, len, 0);

	if (len < 0)
		errx(3, "send");

	printf("Sent %lu bytes\n", len);

	memset(buf, 0, sizeof(buf));
	len = recv(s, buf, sizeof(buf), 0);
	if (len == 0)
		err(4, "recv (no data)");
	if (len < 0)
		err(errno, "recv");

	printf("----------\nGot %lu bytes:\n%s\n", len, buf);

	if (memcmp(server_handshake, buf, sizeof(server_handshake) - 1))
		err(5, "not ok?");

	printf("Going to send junk...\n");
	memset(buf, 0, sizeof(buf));

	len = frame_it(buf, sizeof(buf), msg, sizeof(msg));
	printf("junk is %lu bytes...\n", len);

	printf("xxx: header is 0x%02x 0x%02x 0x%02x 0x%02x\n",
	    buf[0], buf[1], buf[2], buf[3]);
	printf("xxx: payload len is %d\n", (127 & buf[1]));
	len = send(s, buf, len, 0);
	if (len < 1)
		err(1, "send junk");

	printf("Listening for response...\n");

	memset(buf, 0, sizeof(buf));
	len = recv(s, buf, sizeof(buf), 0);

	printf("Got %lu bytes:\n", len);
	printf("header bytes: 0x%02x 0x%02x\n", buf[0], buf[1]);
	printf("raw message: %s\n", buf);

	if (shutdown(s, SHUT_RDWR))
		err(5, "shutdown");

	return 0;
}
