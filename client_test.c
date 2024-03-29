#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef _WIN32
#include <sys/socket.h>
#if defined(__X86_64__) || defined(__LP64__)
#define SSIZE_T_PARAM "%ld"
#else
#define SSIZE_T_PARAM "%d"
#endif //x86_64
#else
#include <WinSock2.h>
#include <stdint.h>
#define SSIZE_T_PARAM "%lld"
#endif

#include <assert.h>
#include <errno.h>
#include <tls.h>

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
	uint16_t port = 8000;
	ssize_t len;
	char *host = "localhost";
	char out[1024];
	struct websocket ws;

	while ((ch = getopt(argc, argv, "th:p:")) != -1) {
		switch (ch) {
		case 'h':
			host = optarg;
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 't':
			use_tls = 1;
			break;
		default:
			printf("client_test usage: [-t] [-h host] [-p port]\n");
			exit(1);
		}
	}

	printf("connecting to %s:%u\n", host, port);

	memset(&ws, 0, sizeof(struct websocket));
	if (use_tls)
		assert(0 == dumb_connect_tls(&ws, host, port, 1));
	else
		assert(0 == dumb_connect(&ws, host, port));

	assert(0 == dumb_handshake(&ws, "/", "dumb-ws"));
	printf("handshake complete\n");

	assert(0 == dumb_ping(&ws));
	printf("PINGed and got PONG frame!\n");

	printf("sending small payload (%zu bytes)\n", SHORT_MSG_LEN);
	len = dumb_send(&ws, &SHORT_MSG, SHORT_MSG_LEN);
	assert(len == (ssize_t) SHORT_MSG_LEN + 6);
	printf("sent " SSIZE_T_PARAM " bytes (header + payload)\n", len);

	memset(buf, 0, sizeof(buf));
	len = 0;
	do {
		len = dumb_recv(&ws, buf, sizeof(buf));
	} while (len == DWS_WANT_POLL);
	snprintf(out, sizeof(out), "%s", buf);
	printf("received payload of " SSIZE_T_PARAM " bytes:\n---\n%s\n---\n",
		len, out);

	printf("sending large payload (%zu bytes)\n", LONG_MSG_LEN);
	len = dumb_send(&ws, &LONG_MSG, LONG_MSG_LEN);
	assert(len == (ssize_t) LONG_MSG_LEN + 8);
	printf("sent " SSIZE_T_PARAM " bytes (header + payload)\n", len);

	memset(buf, 0, sizeof(buf));
	do {
		len = dumb_recv(&ws, buf, sizeof(buf));
	} while (len == DWS_WANT_POLL);
	snprintf(out, sizeof(out), "%s", buf);
	printf("received payload of " SSIZE_T_PARAM " bytes:\n---\n%s\n---\n",
		len, out);

	assert(DWS_OK == dumb_close(&ws));
	printf("sent a CLOSE frame!\n");

	// Our socket should be closed now
	assert(DWS_ERR_READ == dumb_recv(&ws, buf, sizeof(buf)));
	printf("socket looks closed!\n");

	return 0;
}
