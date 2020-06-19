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
// static const char MAGIC_JUNK[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

/*
 * This is the dumbest 16-byte base64 data generator.
 */
void
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

int
main()
{
	int ret, s;
	char key[25];
	char buf[1024];
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

/* Via RFC-6455, a frame looks like:

      0                   1                   2                   3
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     +-+-+-+-+-------+-+-------------+-------------------------------+
     |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
     |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
     |N|V|V|V|       |S|             |   (if payload len==126/127)   |
     | |1|2|3|       |K|             |                               |
     +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
     |     Extended payload length continued, if payload len == 127  |
     + - - - - - - - - - - - - - - - +-------------------------------+
     |                               |Masking-key, if MASK set to 1  |
     +-------------------------------+-------------------------------+
     | Masking-key (continued)       |          Payload Data         |
     +-------------------------------- - - - - - - - - - - - - - - - +
     :                     Payload Data continued ...                :
     + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
     |                     Payload Data continued ...                |
     +---------------------------------------------------------------+
*/
	memset(buf, 0, sizeof(buf));
	buf[0] = 0x82;
	buf[1] = 0x87;
	// dumb mask
	buf[2] = 0xff;
	buf[3] = 0xff;
	buf[4] = 0xff;
	buf[5] = 0xff;
	buf[6] = 'H' ^ 0xff;
	buf[7] = 'e' ^ 0xff;
	buf[8] = 'l' ^ 0xff;
	buf[9] = 'l' ^ 0xff;
	buf[10] = 'o' ^ 0xff;
	buf[11] = '!' ^ 0xff;
	buf[12] = '\n' ^ 0xff;
	buf[13] = '\0';

	printf("Going to send junk...\n");
	len = send(s, buf, 14, 0);
	if (len < 1)
		err(1, "send junk");

	printf("Listening for response...\n");

	len = recv(s, buf, sizeof(buf), 0);

	printf("Got %lu bytes:\n", len);
	printf("header bytes: 0x%02x 0x%02x\n", (unsigned char) buf[0], (unsigned char) buf[1]);
	printf("raw message: %s\n", buf);

	if (shutdown(s, SHUT_RDWR))
		err(5, "shutdown");

	return 0;
}
