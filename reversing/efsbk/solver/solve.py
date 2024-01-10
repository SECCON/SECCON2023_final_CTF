from Crypto.Cipher import AES
from cryptography.hazmat.primitives.serialization import pkcs12
from cryptography.hazmat.primitives.asymmetric import padding
import subprocess

with open("../files/flag.bin", "rb") as f:
    buf = f.read()

# Get EFEK, encrypted flag, and encrypted PFX
efek = buf[0x1e8:0x2e8][::-1]
pfx_begin = buf.index(b"\x30\x82")
enc = buf[0x514:pfx_begin]
encpfx = buf[pfx_begin:]

# Get private key from PFX
priv, _, _ = pkcs12.load_key_and_certificates(encpfx, b"SECCON CTF 2023 Finals")

# Decrypt EFEK
fek = priv.decrypt(efek, padding.PKCS1v15())[0x10:]

# Decrypt flag
flag = b""
for i in range(0, len(enc), 0x200):
    iv1 = int.to_bytes(0x5816657be9161312 + i, 8, 'little')
    iv2 = int.to_bytes(0x1989adbe44918961 + i, 8, 'little')

    aes = AES.new(fek, AES.MODE_CBC, iv=iv1+iv2)
    flag += aes.decrypt(enc[i:i+0x200])

print(flag.decode())
