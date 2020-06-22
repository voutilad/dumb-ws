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

#ifndef DWS_H
#define	DWS_H

#include <sys/types.h>

// We only do binary frames; text frames imply utf-8 support (yuck)
enum FRAME_OPCODE {
	BINARY	= 0x2,
	CLOSE	= 0x8,
	PING	= 0x9,
	PONG	= 0xa,
};

int dumb_connect(char*, int);
int dumb_handshake(int, char*, char*);
ssize_t dumb_send(int, uint8_t*, size_t);
ssize_t dumb_recv(int, uint8_t*, size_t);
int dumb_ping(int);
int dumb_close(int);

#endif /* DWS_H */
