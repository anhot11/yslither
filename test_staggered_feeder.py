import asyncio
import time

async def spawn_one_bot(bot_id, delay, host="23.29.125.178", port=444):
    await asyncio.sleep(delay)
    import websockets
    url = f"ws://{host}:{port}/slither"
    try:
        async with websockets.connect(url, origin="https://slither.com", open_timeout=5) as ws:
            print(f"[{time.strftime('%X')}] [Bot {bot_id}] Connected!")
            await ws.send(bytes([1]))
            await ws.send(bytes([ord('c'), 0]))
            
            # Wait for challenge '6'
            msg = await asyncio.wait_for(ws.recv(), timeout=4.0)
            if len(msg) > 0 and msg[0] == ord('6'):
                # Decode secret
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
                await ws.send(bytes(result))
                
                # Send spawn packet
                nick = f"[FEED] #{bot_id}".encode()
                ba = bytearray()
                ba.append(115) # 's'
                ba.append(30)
                ba.append((291 >> 8) & 0xFF)
                ba.append(291 & 255)
                cwa = bytes([54, 206, 204, 169, 97, 178, 74, 136, 124, 117, 14, 210, 106, 236, 8, 208, 136, 213, 140, 111])
                ba.extend(cwa)
                ba.append(8) # Lime green skin
                ba.append(len(nick))
                ba.extend(nick)
                ba.append(0)
                ba.append(0)
                await ws.send(bytes(ba))
                print(f"[{time.strftime('%X')}] [Bot {bot_id}] Spawn request sent! Keeping alive...")
                
                t0 = time.time()
                while time.time() - t0 < 8:
                    sub_msg = await asyncio.wait_for(ws.recv(), timeout=2.0)
                    cmd = chr(sub_msg[0]) if sub_msg[0] >= 32 else sub_msg[0]
                    await ws.send(bytes([251])) # ping
                    await asyncio.sleep(0.3)
                print(f"[{time.strftime('%X')}] [Bot {bot_id}] Successfully stayed alive for 8s!")
                return True
    except Exception as exc:
        print(f"[{time.strftime('%X')}] [Bot {bot_id}] Error: {exc}")
        return False

async def main():
    print("Testing staggered connection of 4 feeder bots (1.5s interval)...")
    tasks = [spawn_one_bot(i, (i - 1) * 1.5) for i in range(1, 5)]
    results = await asyncio.gather(*tasks)
    print("Results:", results)

asyncio.run(main())
