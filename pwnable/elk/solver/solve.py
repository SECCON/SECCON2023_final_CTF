from ptrlib import *
import os

HOST = os.getenv("SECCON_HOST", "localhost")
PORT = int(os.getenv("SECCON_PORT", "9999"))

sock = Socket(HOST, PORT)
sock.sendlineafter(":\n", open("exploit.js").read() + "\n__EOF__")
print(sock.recvline())
sock.close()
