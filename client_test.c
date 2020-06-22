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
	int s, port, ret;
	size_t len;
	char *host;

	if (argc < 3)
		errx(1, "too few arguments");

	host = argv[argc - 2];
	port = atoi(argv[argc - 1]);
	printf("connecting to %s:%d\n", host, port);

	s = dumb_connect(argv[argc-2], atoi(argv[argc-1]));

	if (dumb_handshake(s, host, "/"))
		err(2, "handshake");

	printf("handshake complete\n");

	// don't send the null byte
	printf("sending small payload (%lu bytes)\n", sizeof(SHORT_MSG) - 1);
	len = dumb_send(s, &SHORT_MSG, sizeof(SHORT_MSG) - 1);

	memset(buf, 0, sizeof(buf));
	len = dumb_recv(s, buf, sizeof(buf));
	printf("received payload of %lu bytes:\n---\n%s\n---\n", len, buf);

	printf("sending large payload (%lu bytes)\n", sizeof(LONG_MSG) - 1);
	len = dumb_send(s, &LONG_MSG, sizeof(LONG_MSG) - 1);

	memset(buf, 0, sizeof(buf));
	len = dumb_recv(s, buf, sizeof(buf));
	printf("received payload of %lu bytes:\n---\n%s\n---\n", len, buf);

	ret = dumb_ping(s);
	if (ret) {
		fprintf(stderr, "dumb_ping returned %d\n", ret);
		errx(5, "dumb_ping");
	}

	if (dumb_close(s))
		errx(6, "dumb_close");

	if (shutdown(s, SHUT_RDWR))
		err(7, "shutdown");

	return 0;
}
