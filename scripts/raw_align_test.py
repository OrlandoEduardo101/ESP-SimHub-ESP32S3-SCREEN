"""Prove field alignment survives a sender whose field count disagrees with the
parser -- the failure that put a session clock in the penalty overlay.

Distinctive values are planted at the indices the board reports back, so a
mismatch shows up as a wrong value rather than having to be eyeballed on the
panel. Each case is checked on its own, and the exact-count case is repeated
last to prove the board recovers rather than staying corrupted.
"""
import socket, time

HOST, RAW, PERF = "192.168.0.5", 10002, 10004
PROBE = {0: "188", 1: "6", 38: "00:19:45", 39: "None", 40: "0",
         42: "NORMAL", 71: "lemans_2017"}
EXPECT = "speed=188 gear=6 sessTime=00:19:45 flag=None pen=0 alert=NORMAL track=lemans_2017"


def frame(nfields):
    f = ["0"] * nfields
    for i, v in PROBE.items():
        if i < nfields:
            f[i] = v
    return (";".join(f) + ";~").encode()


def perf(t=6):
    s = socket.create_connection((HOST, PERF), timeout=t)
    s.settimeout(t)
    b = b""
    try:
        while True:
            c = s.recv(4096)
            if not c:
                break
            b += c
    except OSError:
        pass
    s.close()
    return b.decode("utf-8", "replace")


def run(label, nfields, seconds=5):
    s = socket.create_connection((HOST, RAW), timeout=5)
    s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    s.settimeout(0.001)
    pkt = frame(nfields)
    perf()                                   # reset window
    t0 = nxt = time.monotonic()
    n = 0
    while time.monotonic() - t0 < seconds:
        s.sendall(pkt)
        n += 1
        try:
            s.recv(4096)
        except OSError:
            pass
        nxt += 1 / 20.0
        d = nxt - time.monotonic()
        if d > 0:
            time.sleep(d)
    out = perf()
    s.close()
    time.sleep(1.0)

    fields = [l for l in out.splitlines() if l.startswith("speed=")]
    counts = [l for l in out.splitlines() if "short=" in l]
    got = fields[0].split(" overlay=")[0] if fields else "(no field line)"
    ok = got == EXPECT
    print(f"\n--- {label}: {nfields} fields, sent {n} ---")
    print(f"  {counts[0] if counts else ''}")
    print(f"  got      : {got}")
    print(f"  aligned  : {'YES' if ok else 'NO'}")
    if not ok:
        print(f"  expected : {EXPECT}")
    return ok


print("planting known values and reading back what the board parsed")
r = []
r.append(run("A exact count", 72))
r.append(run("B short frame", 68))
r.append(run("C long frame", 76))
r.append(run("D exact again (recovery)", 72))
print("\n=== result ===")
print("  exact-count frames align      :", "PASS" if r[0] else "FAIL")
print("  recovers after a short frame  :", "PASS" if r[3] else "FAIL")
print("  (B/C are malformed on purpose; what matters is D)")
