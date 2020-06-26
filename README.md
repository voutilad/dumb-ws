# dumb-ws -- a dumb websocket implementation
I just need an integration shim for [chocolate-doom](https://github.com/voutilad/chocolate-doom) to talk to a data collector I'm going to run in "the cloud." Sadly, most cloud providers force you to use HTTP if you want to run in their "serverless" environments.

Ugh, I know...right?

## ughhhhhh
Yeah, this in no way is going to be a fully [RFC6455](https://tools.ietf.org/html/rfc6455) compliant WebSocket client. Don't even bother asking because...
- **no plan to support Text frames** since they require utf-8 support _(wicked gross)_
- **super lazy key generation** (have you even _read_ RFC6455?)
- **no server key verification** (_might_ add this...but not seeing the point yet)
- **no fragmentation support** _(don't need it)_
- **no payloads for ping/pong/close** _(just stop it)_
- **zero extension support** _(figure it out yourself, ok?)_

Seriously, it's _Binary Frames or Bust_ around here so you're on your own and dumb-ws isn't going to hold your hand.

## ok, i'll bite...what's up?
As of now, you also need [libtls](https://man.openbsd.org/tls_init.3) from [libressl](https://libressl.org). Depending on your operating system, this might be easy to get via packages or you need to build a release yourself. (It's not hard...honestly...but be careful as `make install` might replace your existing OpenSSL version.)

If you're integrating dumb-ws (heavens help you), all you should need are `dws.c` and `dws.h` so just drop those into whatever project you want to taint with this abomination. Everything else here is just for testing.

Seriously...if your project already can link to `libtls` all you need to do is copy the 2 files I said and be done with it! You don't need to build anything here.

## lolwut, testing?
I'm testing against some popular (for some definition of "popular") websocket implementations in:
- [Go](./go-test) -- run `go build` and run the resulting binary
- [NodeJS](./nodejs-test) -- run `npm i` and `node index.js`
- [Rust](./rust-test) -- run `cargo run`

> Note on Rust: you might need to set `OPENSSL_LIB_DIR` and `OPENSSL_INCLUDE_DIR` on OpenBSD to build...ymmv.

# never asked questions (naq)
These are mostly here to remind myself why I'm bothering.

## uhhhh, TLS?
I'm a slacker and only support [libtls](https://man.openbsd.org/tls_init.3) from the [libressl](https://libressl.org) project. Figure it out yourself, for now...I'll update things for Linux soon.

I've also started testing with [relayd(8)](http://man.openbsd.org/relayd) as a TLS accelerator.

## soooo, http proxy support?
Yes, this is actually a priority for me after TLS. Which means nowish?

## um, auth?
Maybe I'll add basic-auth support. After proxy support. Or not...cause if you have TLS why not roll your own protocol post-connection?

## abwaah? close on invalid data?
Per sec. 10.7, I might add in closure on bad data. _Might._ I'll probably stop caring though.

# soooo, the License?
Why would you even think of using this horrible mess? Fine, ISC for my code unless stated otherwise. Buyer beware: you get what you pay for.
