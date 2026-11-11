#!/usr/bin/env python3
"""End-to-end tests for ircserv.

Starts the server on a free port, connects several raw TCP clients and checks
the replies for every mandatory command.

    make && python3 tests/test_ircserv.py
"""
import os
import socket
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BINARY = os.path.join(ROOT, "ircserv")
PASSWORD = "s3cret"

passed = 0
failed = 0


def check(name, cond):
    global passed, failed
    if cond:
        passed += 1
        print(f"  \033[32mPASS\033[0m {name}")
    else:
        failed += 1
        print(f"  \033[31mFAIL\033[0m {name}")


class Conn:
    def __init__(self, port):
        self.sock = socket.create_connection(("127.0.0.1", port), timeout=2)
        self.buf = b""

    def send(self, data):
        self.sock.sendall(data.encode() if isinstance(data, str) else data)

    def cmd(self, line):
        self.send(line + "\r\n")

    def read(self, wait=0.3):
        """Return every line received within `wait` seconds."""
        end = time.time() + wait
        self.sock.settimeout(0.05)
        while time.time() < end:
            try:
                chunk = self.sock.recv(4096)
                if not chunk:
                    break
                self.buf += chunk
            except socket.timeout:
                pass
        lines = self.buf.decode(errors="replace").split("\r\n")
        self.buf = lines.pop().encode()
        return lines

    def expect(self, needle, wait=0.3):
        lines = self.read(wait)
        return any(needle in l for l in lines), lines

    def close(self):
        self.sock.close()


def register(port, nick, password=PASSWORD):
    c = Conn(port)
    c.cmd(f"PASS {password}")
    c.cmd(f"NICK {nick}")
    c.cmd(f"USER {nick} 0 * :{nick} Real")
    c.read()
    return c


def free_port():
    s = socket.socket()
    s.bind(("127.0.0.1", 0))
    port = s.getsockname()[1]
    s.close()
    return port


def main():
    port = free_port()
    server = subprocess.Popen([BINARY, str(port), PASSWORD],
                              stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    time.sleep(0.3)
    try:
        run(port)
    finally:
        server.terminate()
        server.wait(timeout=3)
    check("server exits cleanly on SIGTERM", server.returncode == 0)
    print(f"\n{passed} passed, {failed} failed")
    sys.exit(1 if failed else 0)


def run(port):
    print("arguments")
    r = subprocess.run([BINARY], capture_output=True)
    check("no arguments -> usage + exit 1", r.returncode == 1 and b"Usage" in r.stderr)
    r = subprocess.run([BINARY, "99999", "pw"], capture_output=True)
    check("invalid port rejected", r.returncode == 1)

    print("registration")
    c = Conn(port)
    c.cmd("NICK early")
    ok, _ = c.expect(" 464 ")
    check("NICK before PASS -> 464", ok)
    c.cmd("PASS wrong")
    ok, _ = c.expect(" 464 ")
    check("wrong password -> 464", ok)
    c.cmd("JOIN #x")
    ok, _ = c.expect(" 451 ")
    check("command before registration -> 451", ok)
    c.cmd(f"PASS {PASSWORD}")
    c.cmd("NICK 1bad")
    ok, _ = c.expect(" 432 ")
    check("invalid nick -> 432", ok)
    c.cmd("NICK alice")
    c.cmd("USER alice 0 *")
    ok, _ = c.expect(" 461 ")
    check("USER missing params -> 461", ok)
    c.cmd("USER alice 0 * :Alice Liddell")
    ok, lines = c.expect(" 001 alice ")
    check("full registration -> 001 welcome", ok)
    check("welcome burst includes 002-004", all(any(f" {n} " in l for l in lines) for n in ("002", "003", "004")))
    c.cmd("USER again 0 * :x")
    ok, _ = c.expect(" 462 ")
    check("USER after registration -> 462", ok)
    alice = c

    bob = Conn(port)
    bob.cmd(f"PASS {PASSWORD}")
    bob.cmd("NICK ALICE")
    ok, _ = bob.expect(" 433 ")
    check("nick collision is case-insensitive -> 433", ok)
    bob.cmd("NICK bob")
    bob.cmd("USER bob 0 * :Bob")
    ok, _ = bob.expect(" 001 bob ")
    check("second client registers", ok)

    print("partial data / framing")
    alice.send("PI")
    time.sleep(0.1)
    alice.send("NG tok")
    time.sleep(0.1)
    alice.send("en\r\n")
    ok, _ = alice.expect("PONG ircserv :token")
    check("command split across 3 packets is reassembled", ok)
    alice.send("PING lf-only\n")
    ok, _ = alice.expect("PONG ircserv :lf-only")
    check("bare LF line ending accepted (nc)", ok)
    alice.cmd("FOOBAR")
    ok, _ = alice.expect(" 421 ")
    check("unknown command -> 421", ok)

    print("JOIN / PRIVMSG / PART")
    alice.cmd("JOIN #42")
    ok, lines = alice.expect("JOIN #42")
    check("JOIN echoes to joiner", ok)
    check("creator gets operator in NAMES", any(" 353 " in l and "@alice" in l for l in lines))
    bob.cmd("JOIN #42")
    ok, _ = alice.expect(":bob!bob@127.0.0.1 JOIN #42")
    check("existing members see the JOIN", ok)
    bob.read()
    alice.cmd("PRIVMSG #42 :hello channel")
    ok, _ = bob.expect(":alice!alice@127.0.0.1 PRIVMSG #42 :hello channel")
    check("channel message delivered", ok)
    ok, _ = alice.expect("hello channel", 0.2)
    check("sender does not receive own channel message", not ok)
    bob.cmd("PRIVMSG alice :hi in private")
    ok, _ = alice.expect("PRIVMSG alice :hi in private")
    check("private message delivered", ok)
    bob.cmd("PRIVMSG nobody :x")
    ok, _ = bob.expect(" 401 ")
    check("PRIVMSG unknown nick -> 401", ok)
    bob.cmd("PRIVMSG #42")
    ok, _ = bob.expect(" 412 ")
    check("PRIVMSG without text -> 412", ok)
    bob.cmd("NOTICE nobody :x")
    ok, _ = bob.expect(" 401 ", 0.2)
    check("NOTICE never generates errors", not ok)

    carol = register(port, "carol")
    carol.cmd("PRIVMSG #42 :outsider")
    ok, _ = carol.expect(" 404 ")
    check("non-member cannot send to channel -> 404", ok)

    print("operator commands")
    bob.cmd("KICK #42 alice")
    ok, _ = bob.expect(" 482 ")
    check("KICK by non-operator -> 482", ok)
    alice.cmd("TOPIC #42 :Welcome to 42")
    ok, _ = bob.expect("TOPIC #42 :Welcome to 42")
    check("TOPIC set and broadcast", ok)
    bob.cmd("TOPIC #42")
    ok, _ = bob.expect(" 332 bob #42 :Welcome to 42")
    check("TOPIC query -> 332", ok)

    alice.cmd("MODE #42 +t")
    ok, _ = bob.expect("MODE #42 +t")
    check("MODE +t broadcast", ok)
    bob.cmd("TOPIC #42 :hijack")
    ok, _ = bob.expect(" 482 ")
    check("+t blocks topic change by non-op", ok)

    alice.cmd("MODE #42 +o bob")
    ok, _ = bob.expect("MODE #42 +o bob")
    check("MODE +o grants operator", ok)
    bob.cmd("TOPIC #42 :bob is op now")
    ok, _ = alice.expect("TOPIC #42 :bob is op now")
    check("new operator can change topic under +t", ok)
    alice.cmd("MODE #42 -o bob")
    alice.read()
    bob.read()

    alice.cmd("MODE #42 +i")
    alice.read()
    carol.cmd("JOIN #42")
    ok, _ = carol.expect(" 473 ")
    check("+i blocks uninvited join -> 473", ok)
    alice.cmd("INVITE carol #42")
    ok, _ = alice.expect(" 341 alice carol #42")
    check("INVITE -> 341 to inviter", ok)
    ok, _ = carol.expect("INVITE carol :#42")
    check("INVITE delivered to target", ok)
    carol.cmd("JOIN #42")
    ok, _ = carol.expect("JOIN #42")
    check("invited user can join +i channel", ok)
    alice.cmd("MODE #42 -i")
    alice.read()
    bob.read()
    carol.read()

    alice.cmd("MODE #42 +k key42")
    alice.read()
    dave = register(port, "dave")
    dave.cmd("JOIN #42")
    ok, _ = dave.expect(" 475 ")
    check("+k blocks join without key -> 475", ok)
    dave.cmd("JOIN #42 key42")
    ok, _ = dave.expect("JOIN #42")
    check("+k allows join with correct key", ok)
    dave.cmd("PART #42 :bye")
    alice.cmd("MODE #42 -k")
    alice.read()

    alice.cmd("MODE #42 +l 3")
    alice.read()
    dave.cmd("JOIN #42")
    ok, _ = dave.expect(" 471 ")
    check("+l blocks join when full -> 471", ok)
    alice.cmd("MODE #42")
    ok, _ = alice.expect(" 324 alice #42 +tl 3")
    check("MODE query -> 324 with current modes", ok)
    alice.cmd("MODE #42 +x")
    ok, _ = alice.expect(" 472 ")
    check("unknown mode -> 472", ok)
    alice.cmd("MODE #42 -l")
    alice.read()

    alice.cmd("KICK #42 carol :be nice")
    ok, _ = carol.expect("KICK #42 carol :be nice")
    check("KICK delivered to target", ok)
    carol.cmd("PRIVMSG #42 :still here?")
    ok, _ = carol.expect(" 404 ")
    check("kicked user is out of the channel", ok)

    print("NICK change / PART / QUIT")
    bob.read()
    alice.read()
    bob.cmd("NICK robert")
    ok, _ = alice.expect(":bob!bob@127.0.0.1 NICK :robert")
    check("NICK change visible to channel peers", ok)
    bob.cmd("PART #42 :later")
    ok, _ = alice.expect("PART #42 :later")
    check("PART broadcast with reason", ok)
    bob.cmd("JOIN #42")
    alice.read()
    bob.cmd("QUIT :gone")
    ok, _ = alice.expect(":robert!bob@127.0.0.1 QUIT :Quit: gone")
    check("QUIT broadcast to channel peers", ok)

    print("robustness")
    e = register(port, "eve")
    e.sock.close()
    time.sleep(0.2)
    alice.cmd("PING alive")
    ok, _ = alice.expect("PONG ircserv :alive")
    check("server survives abrupt client disconnect", ok)
    f = Conn(port)
    f.send("PASS " + "A" * 20000)
    time.sleep(0.2)
    alice.cmd("PING still")
    ok, _ = alice.expect("PONG ircserv :still")
    check("server survives oversized input without newline", ok)
    many = [register(port, f"u{i}") for i in range(30)]
    for m in many:
        m.cmd("JOIN #crowd")
    many[0].read()
    many[1].cmd("PRIVMSG #crowd :broadcast")
    got = sum(1 for m in many[2:] if m.expect("broadcast", 0.1)[0])
    check("30 clients in one channel all receive a broadcast", got == 28)
    for m in many:
        m.close()
    for x in (alice, carol, dave, f):
        x.close()


if __name__ == "__main__":
    main()
