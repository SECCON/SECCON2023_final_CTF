from pwn import *

dat = asm("""
mov rax, 0x1111111111111190
int3
int3
int3
int3
""", arch='amd64')

for c in range(0x100):
    s = str(disasm(bytes([c]) + dat))
    v = s.split("\n")
    if "nop" in v[1]:
        print(s)
        print("-"*10)
