"""One definitive, self-contained run against a freshly-booted board.

Covers, in order, on a single connection with no leftover state from any
earlier run:

1. FCY: hold "YELLOW FLAG" continuously for 5s (well past ALERT_DURATION_MS
   = 3000ms) -> the overlay must show at first, then clear on its own even
   though the condition never stops being true.
2. AMS2 green-at-every-post: pulse "GREEN FLAG" for 0.5s / "NORMAL" for 4s,
   four times -> only the first pulse (a genuinely new text) should show an
   overlay; the following three (same text) must be suppressed.
3. Sanity: a truly different text ("BLUE FLAG") afterwards must still show,
   proving the fix suppresses repeats without ever getting permanently stuck.
"""
import socket
import time

HOST, RAW, PERF = "192.168.0.5", 10002, 10004


def frame(alert_text):
    f = ["0"] * 72
    f[0] = "80"
    f[4] = "3000"
    f[42] = alert_text
    f[43] = ""
    return (";".join(f) + ";~").encode()


def read_perf(t=4):
    sock = socket.create_connection((HOST, PERF), timeout=t)
    sock.settimeout(t)
    buf = b""
    try:
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            buf += chunk
    except OSError:
        pass
    sock.close()
    return buf.decode("utf-8", "replace")


def overlay_of(text):
    for line in text.splitlines():
        if "overlay=[" in line:
            return line.split("overlay=[", 1)[1].split("]", 1)[0]
    return None


def prev_alert_of(text):
    for line in text.splitlines():
        if "prevAlertText=[" in line:
            return line.split("prevAlertText=[", 1)[1].split("]", 1)[0]
    return None


s = socket.create_connection((HOST, RAW), timeout=5)
s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
s.settimeout(0.001)


def send_for(text, seconds, hz=10):
    n = max(1, int(seconds * hz))
    for _ in range(n):
        s.sendall(frame(text))
        try:
            s.recv(4096)
        except OSError:
            pass
        time.sleep(1.0 / hz)


boot = read_perf()
print("boot state:", "prevAlertText=[" + str(prev_alert_of(boot)) + "]",
      "overlay=[" + str(overlay_of(boot)) + "]")

print("\n=== 1) FCY: YELLOW FLAG held 5s straight ===")
send_for("YELLOW FLAG", 0.5)
early = read_perf()
print("  at 0.5s:", "overlay=[" + str(overlay_of(early)) + "]",
      "prevAlertText=[" + str(prev_alert_of(early)) + "]")
send_for("YELLOW FLAG", 4.5)
late = read_perf()
print("  at 5.0s:", "overlay=[" + str(overlay_of(late)) + "]",
      "prevAlertText=[" + str(prev_alert_of(late)) + "]")
test1_pass = bool(overlay_of(early)) and not overlay_of(late)
print("  ->", "PASS" if test1_pass else "FAIL")

print("\n=== 2) AMS2 green-at-every-post: 4 pulses, 4s gaps ===")
shown = []
for i in range(4):
    send_for("GREEN FLAG", 0.5)
    mid = overlay_of(read_perf())
    shown.append(bool(mid))
    print(f"  pulse {i + 1}: overlay=[{mid}]")
    send_for("NORMAL", 4.0)
test2_pass = shown[0] and not any(shown[1:])
print("  ->", "PASS" if test2_pass else "FAIL", "(shown per pulse:", shown, ")")

print("\n=== 3) Sanity: a genuinely different text still announces ===")
send_for("BLUE FLAG", 0.5)
sanity = read_perf()
test3_pass = bool(overlay_of(sanity))
print("  overlay=[" + str(overlay_of(sanity)) + "]", "->", "PASS" if test3_pass else "FAIL")

print("\n=== overall:", "ALL PASS" if (test1_pass and test2_pass and test3_pass) else "SOME FAILED", "===")
s.close()
