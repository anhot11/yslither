import asyncio
import websockets
import time

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
    async with websockets.connect(url, origin="https://slither.com") as ws:
        await ws.send(bytes([1]))
        await ws.send(bytes([ord('c'), 0]))

        start_time = time.time()
        spawned = False
        last_ping = 0
        last_angle = 0
        wfpr = False

        async def sender():
            nonlocal last_ping, last_angle, wfpr
            ang_val = 0
            while True:
                if spawned:
                    now = time.time()
                    if not wfpr and now - last_ping >= 0.25:
                        last_ping = now
                        wfpr = True
                        await ws.send(bytes([251]))
                    if now - last_angle >= 0.10: # Turn in a circle!
                        last_angle = now
                        ang_val = (ang_val + 5) % 250
                        await ws.send(bytes([ang_val]))
                await asyncio.sleep(0.02)

        sender_task = asyncio.create_task(sender())

        try:
            while True:
                msg = await ws.recv()
                now = time.time()
                elapsed = now - start_time
                if not isinstance(msg, bytes) or len(msg) == 0:
                    continue

                # Parse packet stream
                sub_packets = []
                m = 0
                total_len = len(msg)
                if msg[0] < 32:
                    while m < total_len:
                        if msg[m] < 32:
                            if m + 1 >= total_len: break
                            sub_len = (msg[m] << 8) | msg[m+1]
                            m += 2
                        else:
                            sub_len = msg[m] - 32
                            m += 1
                        if m + sub_len > total_len: break
                        sub_packets.append(msg[m : m + sub_len])
                        m += sub_len
                else:
                    sub_packets.append(msg)

                for p in sub_packets:
                    if len(p) == 0: continue
                    cmd_byte = p[0]
                    # If packet has time delta prefix (len >= 3 and 2-byte time delta)
                    if len(p) >= 3 and p[2] in [ord(c) for c in '6aegGhnNlvrpFsbfy']:
                        c = chr(p[2])
                    else:
                        c = chr(p[0]) if 32 <= p[0] < 127 else f'\\x{p[0]:02x}'

                    if c == 'p':
                        wfpr = False
                    elif c == 'v':
                        print(f"[{elapsed:6.2f}s] *** RECEIVED 'v' (DEATH/KILL) PACKET! len={len(p)} hex={p.hex()[:20]}")
                    elif c in ['6', 'a', 's']:
                        print(f"[{elapsed:6.2f}s] RECV '{c}' (len {len(p)})")

                    if p[0] == ord('6'):
                        secret = decode_secret(p)
                        await ws.send(secret)
                        CLIENT_VERSION = 291
                        nick = b"Circler"
                        ba = bytearray()
                        ba.append(115); ba.append(30)
                        ba.append((CLIENT_VERSION >> 8) & 0xFF)
                        ba.append(CLIENT_VERSION & 0xFF)
                        cwa = bytes([54, 206, 204, 169, 97, 178, 74, 136, 124, 117, 14, 210, 106, 236, 8, 208, 136, 213, 140, 111])
                        ba.extend(cwa); ba.append(8); ba.append(len(nick)); ba.extend(nick); ba.append(0); ba.append(255)
                        await ws.send(bytes(ba))
                        spawned = True
                        print(f"[{elapsed:6.2f}s] Spawned! Circling continuously...")

                if elapsed > 35:
                    print(f"[{elapsed:6.2f}s] SUCCESS! Survived >35s without disconnection!")
                    break
        except websockets.exceptions.ConnectionClosed as e:
            elapsed = time.time() - start_time
            print(f"[{elapsed:6.2f}s] CONNECTION CLOSED! Code={e.code}, Reason={e.reason}")
        finally:
            sender_task.cancel()

asyncio.run(main())
