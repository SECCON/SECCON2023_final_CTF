from ptrlib import *

prefix = b"SECCON{"
enc = b"\xc4\x0e\xba\x13\xe8\x71\xe6\xbd\x2a\x83\xd3\xbf\x78\x38\x31\xfe\x84\x7a\x74\xa7\x6f\x96\xe4\xef\x53\xf0\x93\xcc\xcf\x45\x6a\xac"

def decrypt(seed, enc):
    flag = p64(seed ^ u64(enc[0:8]))
    seed = u64(flag)
    for i in range(8, len(enc), 8):
        seed ^= (seed << 13) % 0x1_0000_0000_0000_0000;
        seed ^= (seed >>  7) % 0x1_0000_0000_0000_0000;
        seed ^= (seed << 17) % 0x1_0000_0000_0000_0000;
        flag += p64(seed ^ u64(enc[i:i+8]))
    return flag

for c in range(0x20, 0x7f):
    seed = u64(prefix + bytes([c])) ^ u64(enc[0:8])
    flag = decrypt(seed, enc)
    if flag[-1:] == b'}':
        print(flag)
