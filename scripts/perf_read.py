import socket, sys, time

def read():
    s = socket.create_connection(("192.168.0.5", 10002), timeout=6)
    buf = b""
    try:
        while True:
            b = s.recv(4096)
            if not b:
                break
            buf += b
    except OSError:
        pass
    s.close()
    return buf.decode("utf-8", "replace")

secs = int(sys.argv[1]) if len(sys.argv) > 1 else 5
read()                      # reset the measurement window
time.sleep(secs)
print(read())
