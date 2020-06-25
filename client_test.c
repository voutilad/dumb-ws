#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include <assert.h>
#include <errno.h>

#include "dws.h"

static char LONG_MSG[] =
    "{\"name\": \"dave\", \"age\": 99,"
    " \"stuff\": [1, 2, 3, 4, 5],"
    " \"more_stuff\": { \"ok\": true },"
    " \"more_and_more\": [ { \"name\": \"maple\" } ],"
    " \"date\": \"2020-06-18T12:34:56\" }";
static size_t LONG_MSG_LEN = sizeof(LONG_MSG) - 1;

static char SHORT_MSG[] = "{\"msg\": \"websockets are dumb\"}";
static size_t SHORT_MSG_LEN = sizeof(SHORT_MSG) - 1;

static uint8_t buf[1024];

int
main(int argc, char **argv)
{
	int ch;
	int use_tls = 0;
	ssize_t len;
	char *host = "localhost", *port = "8080";
	char out[1024];
	struct websocket ws;

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
			printf("client_test usage: [-t] [-h host] [-p port]");
			exit(1);
		}
	}

	printf("connecting to %s:%s\n", host, port);

	memset(&ws, 0, sizeof(struct websocket));
	if (use_tls)
		assert(0 == dumb_connect_tls(&ws, host, port, 1));
	else
		assert(0 == dumb_connect(&ws, host, port));

	assert(0 == dumb_handshake(&ws, host, "/"));
	printf("handshake complete\n");

	// don't send the null byte
	printf("sending small payload (%zu bytes)\n", SHORT_MSG_LEN);
	len = dumb_send(&ws, &SHORT_MSG, SHORT_MSG_LEN);
	assert(len == (ssize_t) SHORT_MSG_LEN + 6);
	printf("sent %ld bytes (header + payload)\n", len);

	memset(buf, 0, sizeof(buf));
	len = dumb_recv(&ws, buf, sizeof(buf));
	snprintf(out, sizeof(out), "%s", buf);
	printf("received payload of %ld bytes:\n---\n%s\n---\n", len, out);

	printf("sending large payload (%zu bytes)\n", LONG_MSG_LEN);
	len = dumb_send(&ws, &LONG_MSG, LONG_MSG_LEN);
	assert(len == (ssize_t) LONG_MSG_LEN + 8);
	printf("sent %ld bytes (header + payload)\n", len);

	memset(buf, 0, sizeof(buf));
	len = dumb_recv(&ws, buf, sizeof(buf));
	snprintf(out, sizeof(out), "%s", buf);
	printf("received payload of %ld bytes:\n---\n%s\n---\n", len, out);

	assert(0 == dumb_ping(&ws));

	assert(0 == dumb_close(&ws));

	// Our socket should be closed now
	assert(-2 == dumb_recv(&ws, buf, sizeof(buf)));

	return 0;
}
