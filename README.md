# ugh, why?
I just need an integration shim for [chocolate-doom](https://github.com/voutilad/chocolate-doom) to talk to a data collector I'm going to run in "the cloud." Sadly, most cloud providers force you to use HTTP if you want to run in their "serverless" environments.

## ughhhhhh
Yeah, this in no way is going to be a fully [RFC6455](https://tools.ietf.org/html/rfc6455) compliant WebSocket client. Don't even bother asking because...
- **no plan to support Text frames** since they require utf-8 support (wicked gross)
- **super lazy key generation** (have you even /read/ RFC6455?)
- **no server key verification** (/might/ add this...but not seeing the point yet)
- **no fragmentation support** (don't need it)
- **no payloads for ping/pong/close** (just stop it)
- **zero extension support** (figure it out yourself, ok?)

## ok, i'll bite
I'm developing on OpenBSD, but you might need to add `libbsd` if on Linux. (On debian-like systems, try: `sudo apt install libbsd-dev`.)

```bash
#!/bin/sh
# for you linux nerds
export CFLAGS=$(pkg-config --cflags libbsd-overlay)
export LDFLAGS=$(pkg-config --libs libbsd-overlay)
make
```

If you're integrating dumb-ws, all you should need are `dws.c` and `dws.h` so just drop those into whatever project you want to taint with this abomination.

## but, testing?
I'm testing against some popular (for some definition of "popular") websocket implementations in:
- [Go](./go-test) -- run `go build` and run the resulting binary
- [NodeJS](./nodejs-test) -- run `npm i` and `node index.js`
- [Rust](./rust-test) -- run `cargo run`

> Note on Rust: you might need to set `OPENSSL_LIB_DIR` and `OPENSSL_INCLUDE_DIR`...ymmv.

# never asked questions (naq)

## uhhhh, TLS?
Yeah, sure. It's my next step, but I'm going to be a slacker and only support [libtls](https://man.openbsd.org/tls_init.3) from the [libressl](https://libressl.org) project.

## soooo, proxy?
Yes, this is actually a priority for me after TLS.

## um, auth?
Maybe I'll add basic-auth support.

## abwaah? close on invalid data?
Per sec. 10.7, I might add in closure on bad data. /Might./

# the license?
Why would you even think of using this horrible mess? Fine, ISC. Buyer beware: you get what you pay for.
