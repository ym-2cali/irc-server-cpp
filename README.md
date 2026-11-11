# IRC Server (C++98)

A lightweight IRC server written in C++98, compatible with standard IRC clients
(irssi, HexChat, WeeChat, `nc`). Built from scratch on raw POSIX sockets with a
single `poll()` event loop and fully non-blocking I/O — no threads, no forking.

## Features

- **Event-driven networking** — one `poll()` call multiplexes the listening
  socket and every client; reads and writes never block.
- **Robust message framing** — partial packets are buffered and reassembled
  (`PRI` + `VMSG #chan :hi\r\n` works), both `\r\n` and bare `\n` are accepted,
  and oversized input is bounded.
- **Authentication** — connection password required before registration.
- **Channels** — creation, membership, operator privileges, topics.
- **Clean shutdown** — handles `SIGINT`/`SIGTERM`, frees every resource
  (valgrind: no leaks, 0 errors).

### Supported commands

| Category       | Commands |
|----------------|----------|
| Registration   | `PASS`, `NICK`, `USER`, `CAP` |
| Connection     | `PING`, `PONG`, `QUIT` |
| Channels       | `JOIN`, `PART`, `TOPIC`, `NAMES`, `WHO` |
| Messaging      | `PRIVMSG`, `NOTICE` (users and channels, comma-separated targets) |
| Operators      | `KICK`, `INVITE`, `MODE` |

### Channel modes

| Mode | Effect |
|------|--------|
| `i`  | Invite-only channel |
| `t`  | Only operators may change the topic |
| `k`  | Channel key (password) required to join |
| `o`  | Give / take operator privilege |
| `l`  | Limit the number of members |

## Build & run

```sh
make
./ircserv <port> <password>
```

Connect with any client:

```sh
irssi -c 127.0.0.1 -p 6667 -w <password> -n alice
# or raw:
nc -C 127.0.0.1 6667
PASS <password>
NICK alice
USER alice 0 * :Alice
JOIN #general
PRIVMSG #general :hello
```

## Tests

An end-to-end suite spins up the server and drives multiple raw TCP clients
through every command, error reply and channel mode (50 checks), including
fragmented packets, abrupt disconnects and a 30-client broadcast.

```sh
make && python3 tests/test_ircserv.py
```

## Architecture

```
inc/ src/
├── main.cpp       argument parsing, signal handling
├── Server         poll loop, accept/read/write, dispatch, client lifecycle
├── Commands       one handler per IRC command (RFC 1459 / 2812 numerics)
├── Client         per-connection state + input/output buffers
├── Channel        members, operators, invites, modes, broadcast
├── Message        IRC line parser: [:prefix] COMMAND params [:trailing]
└── Utils          nick/channel validation, string helpers
```

Output is never written directly to a socket from a command handler: replies
are queued in the client's output buffer and flushed when `poll()` reports the
socket writable, so a slow client can never stall the server.
