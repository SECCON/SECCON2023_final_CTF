from ptrlib import *

flag = b"SECCON{y3t_4n0th3r_m0vfu5c4t0r?}"

seed = 0x1b75f5867fda13b0
def rand():
    global seed
    x = seed
    x ^= (x << 13) % 0x1_0000_0000_0000_0000;
    x ^= (x >>  7) % 0x1_0000_0000_0000_0000;
    x ^= (x << 17) % 0x1_0000_0000_0000_0000;
    seed = x
    return seed

head = u64(flag[0:8])
flag = p64(head ^ rand()) + flag[8:]
seed = head

for i in range(8, len(flag), 8):
    flag = flag[:i] + p64(u64(flag[i:i+8]) ^ rand()) + flag[i+8:]

print(", ".join(map(lambda c: f'0x{c:02x}', flag)))
