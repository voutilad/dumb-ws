#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include <err.h>
#include <errno.h>

#include "dws.h"

static char LONG_MSG[] =
    "{\"name\": \"dave\", \"age\": 99,"
    " \"stuff\": [1, 2, 3, 4, 5],"
    " \"more_stuff\": { \"ok\": true },"
    " \"more_and_more\": [ { \"name\": \"maple\" } ],"
    " \"date\": \"2020-06-18T12:34:56\" }";

static char SHORT_MSG[] = "{\"msg\": \"websockets are dumb\"}";

static uint8_t buf[1024];

int
main(int argc, char **argv)
{
	int ch, ret;
	int use_tls = 0;
	size_t len;
	char *host, *port;
	struct websocket ws;

	if (argc < 4)
		errx(1, "too few arguments");

	while ((ch = getopt(argc, argv, "th:p:")) != -1) {
		switch (ch) {
		case 'h':
			host = optarg;
			break;
		case 'p':
			port = optarg;
			break;
		case 't':
			use_tls = 1;
			break;
		default:
			errx(1, "usage: [-t] [-h host] [-p port]");
		}
	}

	printf("connecting to %s:%s\n", host, port);

	memset(&ws, 0, sizeof(struct websocket));
	if (use_tls)
		ret = dumb_connect_tls(&ws, host, port, 1);
	else
		ret = dumb_connect(&ws, host, port);

	if (ret < 0)
		err(-ret, "dumb_connect (%d)", ret);

	ret = dumb_handshake(&ws, host, "/");
	if (ret)
		err(-ret, "handshake (%d)", ret);

	printf("handshake complete\n");

	// don't send the null byte
	printf("sending small payload (%lu bytes)\n", sizeof(SHORT_MSG) - 1);
	len = dumb_send(&ws, &SHORT_MSG, sizeof(SHORT_MSG) - 1);

	memset(buf, 0, sizeof(buf));
	len = dumb_recv(&ws, buf, sizeof(buf));
	printf("received payload of %lu bytes:\n---\n%s\n---\n", len, buf);

	printf("sending large payload (%lu bytes)\n", sizeof(LONG_MSG) - 1);
	len = dumb_send(&ws, &LONG_MSG, sizeof(LONG_MSG) - 1);

	memset(buf, 0, sizeof(buf));
	len = dumb_recv(&ws, buf, sizeof(buf));
	printf("received payload of %lu bytes:\n---\n%s\n---\n", len, buf);

	ret = dumb_ping(&ws);
	if (ret) {
		fprintf(stderr, "dumb_ping returned %d\n", ret);
		errx(-ret, "dumb_ping");
	}

	ret = dumb_close(&ws);
	if (ret != 0)
		errx(-ret, "dumb_close");

	// Our socket should be closed now
	ret = dumb_recv(&ws, buf, sizeof(buf));
	if (ret != -2)
		errx(7, "shutdown: socket not closed");

	return 0;
}
