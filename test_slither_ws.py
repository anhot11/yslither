import asyncio
import websockets
import time
import struct

def decode_secret(packet):
    string = bytearray(92)
    string_idx = 0
    c = 23; d = 0; e = 0; f = 1
    packet_len = len(packet)
    while f < 184 and f < packet_len:
        b = packet[f]
        f += 1
        if b <= 96:
            b += 32
        b = (b - 97 - c) % 26
        if b < 0:
            b += 26
        d = (d * 16 + b) & 0xFF
        c += 17
        if e == 1:
            if string_idx < 92:
                string[string_idx] = d
                string_idx += 1
            e = 0
            d = 0
        else:
            e += 1
    secret_1 = bytearray(27)
    idx = 0
    for i in range(92):
        if (9 <= i <= 13) or (20 <= i <= 41):
            if idx < 27:
                secret_1[idx] = string[i]
                idx += 1
    b2 = 0
    result = bytearray(27)
    for i in range(27):
        d2 = 65
        a = secret_1[i]
        if a >= 97:
            d2 += 32
            a -= 32
        a -= 65
        if i == 0:
            b2 = 3 + a
        e2 = (a + b2) % 26
        b2 += 2 + a
        result[i] = e2 + d2
    return bytes(result)

async def main():
    url = "ws://23.29.125.178:444/slither"
    extra_headers = {"Origin": "https://slither.com"}
    print(f"Connecting to {url}...")
    async with websockets.connect(url, origin="https://slither.com") as ws:
        print("Connected! Sending init bytes: [1], ['c', 0]")
        await ws.send(bytes([1]))
        await ws.send(bytes([ord('c'), 0]))

        start_time = time.time()
        last_ping = 0
        last_angle = 0

        async def sender():
            nonlocal last_ping, last_angle
            while True:
                now = time.time()
                # Ping every 250ms
                if now - last_ping >= 0.25:
                    last_ping = now
                    # print(f"[{now - start_time:.2f}s] Sending ping 251")
                    await ws.send(bytes([251]))
                # Angle heartbeat every 350ms
                if now - last_angle >= 0.35:
                    last_angle = now
                    # print(f"[{now - start_time:.2f}s] Sending angle 125")
                    await ws.send(bytes([125]))
                await asyncio.sleep(0.05)

        sender_task = asyncio.create_task(sender())

        try:
            while True:
                msg = await ws.recv()
                now = time.time()
                elapsed = now - start_time
                if isinstance(msg, bytes) and len(msg) > 0:
                    cmd = chr(msg[0]) if msg[0] >= 32 else f"\\x{msg[0]:02x}"
                    if msg[0] < 32 and len(msg) >= 3:
                        sub_cmd = chr(msg[2]) if msg[2] >= 32 else f"\\x{msg[2]:02x}"
                        print(f"[{elapsed:6.2f}s] RECV len={len(msg):4d} sub_cmd='{sub_cmd}' (byte {msg[2]})")
                    else:
                        print(f"[{elapsed:6.2f}s] RECV len={len(msg):4d} cmd='{cmd}'")

                    if msg[0] == ord('6'):
                        secret = decode_secret(msg)
                        print(f"[{elapsed:6.2f}s] Decoded secret: {secret}")
                        await ws.send(secret)
                        # Construct spawn packet
                        CLIENT_VERSION = 291
                        nick = b"TestPython"
                        ba = bytearray()
                        ba.append(115)
                        ba.append(30)
                        ba.append((CLIENT_VERSION >> 8) & 0xFF)
                        ba.append(CLIENT_VERSION & 0xFF)
                        cwa = bytes([54, 206, 204, 169, 97, 178, 74, 136, 124, 117, 14, 210, 106, 236, 8, 208, 136, 213, 140, 111])
                        ba.extend(cwa)
                        ba.append(8) # skin
                        ba.append(len(nick))
                        ba.extend(nick)
                        ba.append(0)
                        ba.append(255) # accessory
                        print(f"[{elapsed:6.2f}s] Sending spawn packet len={len(ba)}")
                        await ws.send(bytes(ba))

                if elapsed > 25:
                    print("Test lasted >25s successfully!")
                    break
        except websockets.exceptions.ConnectionClosed as e:
            elapsed = time.time() - start_time
            print(f"[{elapsed:6.2f}s] CONNECTION CLOSED BY SERVER! Code={e.code}, Reason={e.reason}")
        finally:
            sender_task.cancel()

asyncio.run(main())
