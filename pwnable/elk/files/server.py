#!/usr/bin/env python3
import os
import subprocess
import tempfile

os.chdir("/home/pwn")
print("[+] Enter your code (End with '__EOF__'):")

code = ''
while True:
    if len(code) > 0x10 * 0x1000:
        print("[-] Code too long")
        exit(1)

    line = input()
    if line == '__EOF__':
        break

    code += line + '\n'

with tempfile.NamedTemporaryFile("w") as f:
    f.write(code)
    f.flush()

    p = subprocess.Popen(['./elk', f.name],
                         stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    result = p.communicate(timeout=30)
    print(result[0].decode())
