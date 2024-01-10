import os
import random
import re

marker = set()
addr2label = {}
dumping = False
output = ""
for line in open("disasm.txt", "r"):
    if dumping and line.strip() == '':
        dumping = False
        output += ".@L0:\n"
        output += "  pop rax\n"
        output += "  inc rax\n"
        output += "  inc rax\n"
        output += "  push rax\n"
        output += "  ret\n"
        output += f".@L{label}:\n"
        continue

    if '>:' in line:
        addr = int(line[:line.index(' ')], 16)
        name = line[line.index("<")+1:line.index(">")]
        addr2label[addr] = name

    line = line.replace("PTR ", "")
    if '#' in line:
        r = re.findall("# [0-9a-f]+ <(.+)>", line)
        name = r[0]
        if '+' in name:
            if   "0xece" in name: name = "s_correct"
            elif "0xecc" in name: name = "s_end"
            elif "0xec4" in name: name = "s_start"
            elif "0xeb5" in name: name = "s_noflag"
            elif "0xed8" in name: name = "s_wrong"
        i, j, k = line.index("["), line.index("]"), line.index("#")
        line = line[:i] + "[rel " + name + line[j:k]

    if ':\t' in line and '<' in line and '>' in line:
        r = re.findall("([0-9a-f]+) <.+>", line)
        marker.add(r[0])
        line = line[:line.index("<")]

    if dumping:
        r = re.findall("([0-9a-f]+):\s+(([0-9a-f]{2} )+)\s+(.+)", line)
        if len(r):
            sl = f".@L{label}"
            addr = int(r[0][0], 16)
            if addr not in addr2label:
                addr2label[addr] = sl
            dis = r[0][1]
            ope = r[0][-1]
            label += 1
            output += "  db 0x48, 0xB8\n"
            output += f"{sl}:\n"
            output += f"  {ope}\n"
            if dis.count(' ') < 7:
                output += f"  jmp .@L{label}\n"
                length = 8 - dis.count(' ') - 2
                if   length == 1:
                    output += f"  nop\n"
                elif length == 2:
                    output += f"  mov al, {random.randrange(0, 0x100)}\n"
                elif length == 3:
                    output += f"  mov sil, {random.randrange(0, 0x100)}\n"
                elif length == 4:
                    output += f"  mov ax, {random.randrange(0, 0x10000)}\n"
                elif length == 5:
                    output += f"  mov eax, {random.randrange(0, 0x100000000)}\n"
                elif length != 0:
                    raise Exception
            else:
                output +=  "  db 0x8d\n"

    else:
        r = re.findall("^[0-9a-f]+ <(.+)>:", line)
        if len(r):
            label = 1
            dumping = True
            output += f"{r[0]}:\n"
            output +=  "  call .@L0\n"
            continue

output += ".@L0:\n"
output += "  pop rax\n"
output += "  inc rax\n"
output += "  inc rax\n"
output += "  push rax\n"
output += "  ret\n"
output += f".@L{label}:\n"

for m in marker:
    l = addr2label[int(m, 16)]
    while m in output:
        output = output.replace(m, l)

print("""
section .data
seed: dq 1978757569318622128
enc:
  db 0xc4, 0x0e, 0xba, 0x13, 0xe8, 0x71, 0xe6, 0xbd
  db 0x2a, 0x83, 0xd3, 0xbf, 0x78, 0x38, 0x31, 0xfe
  db 0x84, 0x7a, 0x74, 0xa7, 0x6f, 0x96, 0xe4, 0xef
  db 0x53, 0xf0, 0x93, 0xcc, 0xcf, 0x45, 0x6a, 0xac

section .rodata
s_noflag: db "No flag given", 0x0a, 0
s_wrong: db "Wrong...", 0x0a, 0
s_correct: db "Correct!", 0x0a, 0
s_start: db "SECCON{", 0
s_end: db "}", 0

section .text
global _start
""")
print(output)        
