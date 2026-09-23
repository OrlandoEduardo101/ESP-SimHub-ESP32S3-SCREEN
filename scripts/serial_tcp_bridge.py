"""Serial -> TCP bridge for the WiFi dashboard, replacing HW VSP3.

SimHub's Custom Serial Device writes to one end of a com0com pair (COM20);
this reads the other end (COM21) and forwards each telemetry frame to the
board's raw port (192.168.0.5:10002) the moment its '~' terminator arrives.

Why not HW VSP3: measured on the real path, HW VSP3 delivered frames in
bursts -- about four times a second the stream stalled for up to ~250ms and
then 4 frames arrived glued into one TCP segment (gaps_over_100ms and
multi_frame_chunks matched exactly, window after window). The board drew each
burst in a few ms, so the screen visibly updated ~4x/s even though ~21
frames/s were arriving. The same frames sent straight over TCP -- Nagle on
or off -- arrived one per segment with no stalls, so the accumulation is in
the virtual-serial layer, not the board, WiFi or TCP. Here the read loop and
the send timing are ours: nothing is held back once a frame is complete.

Usage:
    python scripts/serial_tcp_bridge.py [--port COM21] [--host 192.168.0.5]

Only one client may be connected to the raw port at a time: the board turns
raw mode off when *any* raw client disconnects, so HW VSP3 must not also be
connected to 10002 while this runs.
"""
import argparse
import socket
import sys
import time

import serial

TERMINATOR = b"~"
MAX_JUNK = 16384  # bytes without a terminator before we assume junk and drop them


def connect_tcp(host, port):
    s = socket.create_connection((host, port), timeout=3)
    s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    s.settimeout(3)
    return s


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default="COM21", help="com0com end this bridge reads")
    ap.add_argument("--host", default="192.168.0.5")
    ap.add_argument("--tcp-port", type=int, default=10002)
    ap.add_argument("--stats", type=float, default=10.0, help="seconds between stats lines (0 = off)")
    args = ap.parse_args()

    ser = serial.Serial(args.port, 115200, timeout=0.005)
    ser.dtr = True
    ser.rts = True
    print("serial %s aberta" % args.port, flush=True)

    sock = None
    next_connect = 0.0
    buf = bytearray()

    frames = 0
    dropped = 0
    last_send = None
    max_gap = 0.0
    stats_t0 = time.time()

    try:
        while True:
            if sock is None and time.time() >= next_connect:
                try:
                    sock = connect_tcp(args.host, args.tcp_port)
                    print("conectado em %s:%d" % (args.host, args.tcp_port), flush=True)
                    buf.clear()  # start on a frame boundary, never mid-frame
                except OSError as e:
                    print("falha ao conectar (%s), tentando de novo em 1s" % e, flush=True)
                    next_connect = time.time() + 1.0

            data = ser.read(max(1, ser.in_waiting))
            if data:
                buf += data

            while True:
                i = buf.find(TERMINATOR)
                if i < 0:
                    break
                frame = bytes(buf[: i + 1])
                del buf[: i + 1]
                if sock is None:
                    dropped += 1  # keep draining so SimHub never blocks on a full port
                    continue
                try:
                    sock.sendall(frame)
                except OSError as e:
                    print("conexao caiu (%s), reconectando" % e, flush=True)
                    try:
                        sock.close()
                    except OSError:
                        pass
                    sock = None
                    next_connect = time.time() + 1.0
                    continue
                now = time.time()
                if last_send is not None:
                    max_gap = max(max_gap, now - last_send)
                last_send = now
                frames += 1

            if len(buf) > MAX_JUNK:
                buf.clear()

            if args.stats and time.time() - stats_t0 >= args.stats:
                win = time.time() - stats_t0
                print("%.0fs: %d quadros (%.1f/s), maior intervalo %.0fms, descartados %d" %
                      (win, frames, frames / win, max_gap * 1000, dropped), flush=True)
                frames = dropped = 0
                max_gap = 0.0
                stats_t0 = time.time()
    except KeyboardInterrupt:
        pass
    finally:
        ser.close()
        if sock is not None:
            sock.close()


if __name__ == "__main__":
    sys.exit(main())
