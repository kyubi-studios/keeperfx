#!/usr/bin/env python3
"""Drive PPSSPPHeadless through its websocket debugger (KeeperFX PSP port testing).

usage: PYTHONPATH=<dir with websocket-client> ppsspp_drive.py <eboot> <root> <outdir> <script>
env:   EMU_TIMEOUT  seconds passed to PPSSPPHeadless --timeout (default 3000)

script: semicolon-separated steps:
  wait:SECONDS        real-time sleep (headless runs unthrottled, often faster than real time)
  shot:NAME           pause the CPU, read VRAM (0x04000000) and save both 512-stride
                      RGBA framebuffers as NAME_fb0.png / NAME_fb1.png; headless can't
                      download GPU output, but the software GPU renders into VRAM
  press:BUTTON[:MS]   press and release a button (cross, circle, square, triangle,
                      start, select, up, down, left, right, ltrigger, rtrigger)
  hold:BUTTON / release:BUTTON
  analog:X:Y          left stick, -1..1 (note: negative Y moves the pointer down)
"""
import base64, json, os, socket, subprocess, sys, time
import websocket

eboot, root, outdir, script = sys.argv[1:5]
_sock = socket.socket(); _sock.bind(("127.0.0.1", 0)); port = _sock.getsockname()[1]; _sock.close()
os.makedirs(outdir, exist_ok=True)
log = open(os.path.join(outdir, "emu.log"), "w")
proc = subprocess.Popen([os.path.expanduser("~/ppsspp-build/PPSSPPHeadless"), eboot, "-r", root,
                         "--timeout=%s" % os.environ.get("EMU_TIMEOUT", "3000"), "--graphics=software", "--debugger=%d" % port],
                        stdout=log, stderr=subprocess.STDOUT)
import atexit, signal
atexit.register(lambda: proc.poll() is None and proc.kill())
signal.signal(signal.SIGTERM, lambda *a: sys.exit(1))
ws = None
for _ in range(300):
    try:
        ws = websocket.create_connection("ws://127.0.0.1:%d/debugger" % port, timeout=30)
        break
    except Exception:
        time.sleep(0.2)
if ws is None:
    sys.exit("could not connect to debugger")

ticket = 0
def call(event, **args):
    global ticket
    ticket += 1
    msg = dict(event=event, ticket=str(ticket), **args)
    ws.send(json.dumps(msg))
    while True:
        r = json.loads(ws.recv())
        if r.get("ticket") == str(ticket):
            return r
        if r.get("event") == "error" and r.get("ticket") == str(ticket):
            return r

def send(event, **args):
    ws.send(json.dumps(dict(event=event, **args)))

def wait_event(name, timeout=30):
    end = time.time() + timeout
    while time.time() < end:
        r = json.loads(ws.recv())
        if r.get("event") == name:
            return r
    return None

send("cpu.resume")
held = {}
buttons_state = {}
for step in [s for s in script.split(";") if s.strip()]:
    parts = step.strip().split(":")
    cmd = parts[0]
    if cmd == "wait":
        time.sleep(float(parts[1]))
    elif cmd == "shot":
        send("cpu.stepping")
        wait_event("cpu.stepping")
        # Headless can't download GPU output; read VRAM (SoftGPU renders there)
        # and decode both candidate 512-stride framebuffers as RGBA8888.
        r = call("memory.read", address=0x04000000, size=0x200000)
        send("cpu.resume")
        raw = base64.b64decode(r.get("base64", ""))
        import zlib, struct
        def png(path, rows, w, h):
            def ch(t, d): return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xffffffff)
            with open(path, "wb") as f:
                f.write(b"\x89PNG\r\n\x1a\n" + ch(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
                        + ch(b"IDAT", zlib.compress(b"".join(rows))) + ch(b"IEND", b""))
        open(os.path.join(outdir, parts[1] + ".vram"), "wb").write(raw)
        for i, base in enumerate((0, 0x88000)):
            rows = []
            for y in range(272):
                line = raw[base + y * 2048: base + y * 2048 + 480 * 4]
                rgb = bytearray()
                for x in range(480):
                    rgb += line[x * 4: x * 4 + 3]
                rows.append(b"\x00" + bytes(rgb))
            png(os.path.join(outdir, "%s_fb%d.png" % (parts[1], i)), rows, 480, 272)
        print("shot", parts[1], len(raw), flush=True)
    elif cmd == "press":
        ms = int(parts[2]) if len(parts) > 2 else 150
        call("input.buttons.send", buttons={parts[1]: True})
        time.sleep(ms / 1000)
        call("input.buttons.send", buttons={parts[1]: False})
        time.sleep(0.1)
    elif cmd == "hold":
        call("input.buttons.send", buttons={parts[1]: True})
    elif cmd == "release":
        call("input.buttons.send", buttons={parts[1]: False})
    elif cmd == "analog":
        call("input.analog.send", x=float(parts[1]), y=float(parts[2]), stick="left")
    else:
        print("unknown step", step)
ws.close()
try:
    proc.wait(timeout=float(os.environ.get("EMU_TIMEOUT", "0")) or 1)
except Exception:
    proc.kill()
