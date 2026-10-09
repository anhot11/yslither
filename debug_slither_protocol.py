import asyncio
import time
import struct

async def main():
    # Import websockets from termux python
    import websockets
    url = "ws://23.29.125.178:444/slither"
    print(f"Connecting to {url}...")
    async with websockets.connect(url, origin="https://slither.com") as ws:
        print("Connected!")
        # 1. Send [1]
        await ws.send(bytes([1]))
        # 2. Send ['c', 0]
        await ws.send(bytes([ord('c'), 0]))

        start = time.time()
        last_ping = 0
        last_angle = 0
        wfpr = False
        spawned = False

        async def pinger():
            nonlocal last_ping, last_angle, wfpr
            while True:
                now = time.time()
                if spawned:
                    if not wfpr and now - last_ping >= 0.25:
                        last_ping = now
                        wfpr = True
                        await ws.send(bytes([251]))
                    if now - last_angle >= 0.1:
                        last_angle = now
                        # send angle
                        await ws.send(bytes([100]))
                await asyncio.sleep(0.01)

        ptask = asyncio.create_task(pinger())

        try:
            while True:
                msg = await ws.recv()
                now = time.time()
                elapsed = now - start
                cmd = chr(msg[0]) if msg[0] >= 32 else f"\\x{msg[0]:02x}"
                print(f"[{elapsed:6.2f}s] RECV len={len(msg):4d} cmd={cmd}")

                if msg[0] == ord('6'):
                    # decode secret
                    packet = msg
                    string = bytearray(92)
                    string_idx = 0
                    c = 23; d = 0; e = 0; f = 1
                    packet_len = len(packet)
                    while f < 184 and f < packet_len:
                        b = packet[f]
                        f += 1
                        if b <= 96: b += 32
                        b = (b - 97 - c) % 26
                        if b < 0: b += 26
                        d = (d * 16 + b) & 0xFF
                        c += 17
                        if e == 1:
                            if string_idx < 92:
                                string[string_idx] = d
                                string_idx += 1
                            e = 0; d = 0
                        else: e += 1
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
                        if a >= 97: d2 += 32; a -= 32
                        a -= 65
                        if i == 0: b2 = 3 + a
                        e2 = (a + b2) % 26
                        b2 += 2 + a
                        result[i] = e2 + d2
                    print(f"[{elapsed:6.2f}s] Sending secret response")
                    await ws.send(bytes(result))

                    # Send spawn
                    CLIENT_VERSION = 291
                    nick = b"Diagnostic"
                    ba = bytearray()
                    ba.append(115) # 's'
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
                    spawned = True

                elif msg[0] == ord('p'):
                    wfpr = False
                elif msg[0] < 32:
                    # check subpackets
                    m = 0
                    while m < len(msg):
                        if msg[m] < 32:
                            if m + 1 < len(msg):
                                sub_len = (msg[m] << 8) | msg[m + 1]
                                m += 2
                            else:
                                break
                        else:
                            sub_len = msg[m] - 32
                            m += 1
                        if m + sub_len <= len(msg):
                            sub = msg[m:m+sub_len]
                            sub_cmd = chr(sub[0]) if len(sub) > 0 and sub[0] >= 32 else "bin"
                            if sub_cmd == 'p':
                                wfpr = False
                            if sub_cmd in ['v', 'k', 's']:
                                print(f"  --> SUBPACKET [{elapsed:6.2f}s] cmd='{sub_cmd}' len={len(sub)}")
                            m += sub_len
                        else:
                            break

                if elapsed > 30:
                    print("SUCCESS! Survived 30 seconds without disconnect!")
                    break

        except websockets.exceptions.ConnectionClosed as exc:
            print(f"[{time.time() - start:6.2f}s] CLOSED: code={exc.code}, reason={exc.reason}")
        finally:
            ptask.cancel()

asyncio.run(main())
