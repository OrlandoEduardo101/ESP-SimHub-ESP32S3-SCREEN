"""Send a frame per TrackId and ask the board whether it resolved a map.

Covers the naming styles different sims use for the same circuit, plus the
lookalikes that a naive substring match would get wrong.
"""
import socket, time

HOST, RAW, PERF = "192.168.0.5", 10002, 10004
CASES = [
    ("monza",                 True,  "plain id"),
    ("ks_barcelona",          True,  "AC prefix -> catalunya alias"),
    ("spa",                   True,  "short id, exact"),
    ("spa_francorchamps",     True,  "long form"),
    ("shanghai",              True,  "newly converted SVG"),
    ("hockenheimring",        True,  "newly converted SVG"),
    ("red_bull_ring",         True,  "multi-word alias"),
    ("monza_1966",            True,  "mod suffix"),
    ("spain",                 False, "must NOT match 'spa'"),
    ("lemans_2017",           False, "no SVG for this circuit"),
    ("nordschleife",          False, "no SVG for this circuit"),
]


def perf(t=5):
    s = socket.create_connection((HOST, PERF), timeout=t)
    s.settimeout(t); b = b""
    try:
        while True:
            c = s.recv(4096)
            if not c: break
            b += c
    except OSError: pass
    s.close(); return b.decode("utf-8", "replace")


def frame(track):
    f = ["0"] * 72
    f[0] = "120"
    f[42] = ""          # keep alertMessage empty so no overlay covers the page
    f[71] = track
    return (";".join(f) + ";~").encode()


s = socket.create_connection((HOST, RAW), timeout=5)
s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
s.settimeout(0.001)

fails = 0
for track, want, why in CASES:
    for _ in range(12):
        s.sendall(frame(track))
        try: s.recv(4096)
        except OSError: pass
        time.sleep(0.05)
    time.sleep(0.4)
    out = perf()
    line = next((l for l in out.splitlines() if l.startswith("speed=")), "")
    got = "map=FOUND" in line
    ok = got == want
    fails += 0 if ok else 1
    print(f"  {'ok ' if ok else 'FAIL'}  {track:22} -> {'FOUND' if got else 'none ':5}  ({why})")

s.close()
print(f"\n{len(CASES)-fails}/{len(CASES)} cases correct")
