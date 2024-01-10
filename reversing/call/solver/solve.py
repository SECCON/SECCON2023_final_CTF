from ptrlib import *

array = []
base = None
with open("../files/call.exe.dmp", "rb") as f:
    f.seek(0x72b64)
    for _ in range(0x1ffe*2):
        if base is None:
            base = u32(f.read(4))
            f.read(1)
        target = u32(f.read(4))
        f.read(1)

        if target == base + 0x10:
            base = None
            array.append(True)
        else:
            array.append(False)
            base = target

def trace_arr(array, route, n):
    index = offset = 0
    base = 1
    for _ in range(n):
        p = index + offset + (route & 1)
        if array[p] == False:
            return False
        base <<= 1
        offset <<= 1
        offset += 2 * (route & 1)
        route >>= 1
        index += base
    return True

flag = ""
for i in range(32):
    found = False
    for c in range(128):
        route = c << 5 | i
        if trace_arr(array, route, 7+5):
            if found:
                print("[-] Multiple solutions!")
                exit(1)
            flag += chr(c)
            found = True

    if found is False:
        print("[-] Unsat")
        exit(1)

print("SECCON{"+flag+"}")
