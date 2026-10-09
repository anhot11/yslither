import asyncio
import time

async def client(name, delay, duration):
    await asyncio.sleep(delay)
    import websockets
    url = "ws://23.29.125.178:444/slither"
    try:
        async with websockets.connect(url, origin="https://slither.com", open_timeout=5) as ws:
            print(f"[{time.strftime('%X')}] {name} connected")
            await ws.send(bytes([1]))
            await ws.send(bytes([ord('c'), 0]))
            msg = await asyncio.wait_for(ws.recv(), 4.0)
            if msg[0] == ord('6'):
                # solve challenge
                packet = msg
                string = bytearray(92)
                string_idx = 0; c = 23; d = 0; e = 0; f = 1
                while f < 184 and f < len(packet):
                    b = packet[f]; f += 1
                    if b <= 96: b += 32
                    b = (b - 97 - c) % 26
                    if b < 0: b += 26
                    d = (d * 16 + b) & 0xFF; c += 17
                    if e == 1:
                        if string_idx < 92: string[string_idx] = d; string_idx += 1
                        e = 0; d = 0
                    else: e += 1
                secret_1 = bytearray(27); idx = 0
                for i in range(92):
                    if (9 <= i <= 13) or (20 <= i <= 41):
                        if idx < 27: secret_1[idx] = string[i]; idx += 1
                b2 = 0; result = bytearray(27)
                for i in range(27):
                    d2 = 65; a = secret_1[i]
                    if a >= 97: d2 += 32; a -= 32
                    a -= 65
                    if i == 0: b2 = 3 + a
                    e2 = (a + b2) % 26; b2 += 2 + a
                    result[i] = e2 + d2
                await ws.send(bytes(result))
                # spawn packet
                nick = name.encode()
                ba = bytearray([115, 30, (291 >> 8) & 0xFF, 291 & 255])
                ba.extend(bytes([54, 206, 204, 169, 97, 178, 74, 136, 124, 117, 14, 210, 106, 236, 8, 208, 136, 213, 140, 111]))
                ba.append(8); ba.append(len(nick)); ba.extend(nick); ba.extend([0, 0])
                await ws.send(bytes(ba))
                print(f"[{time.strftime('%X')}] {name} spawned!")
                t_end = time.time() + duration
                while time.time() < t_end:
                    sub = await asyncio.wait_for(ws.recv(), 2.0)
                    await ws.send(bytes([251])) # ping
                    await asyncio.sleep(0.5)
                print(f"[{time.strftime('%X')}] {name} FINISHED SUCCESSFULLY!")
                return True
    except Exception as exc:
        print(f"[{time.strftime('%X')}] {name} FAILED: {exc}")
        return False

async def main():
    print("Testing 1 Player + 2 Feeder bots with 3s stagger:")
    t_player = client("Player", 0.0, 15.0)
    t_bot1 = client("[FEED] #1", 3.0, 12.0)
    t_bot2 = client("[FEED] #2", 6.0, 9.0)
    res = await asyncio.gather(t_player, t_bot1, t_bot2)
    print("Test finished. All successful?:", res)

asyncio.run(main())
