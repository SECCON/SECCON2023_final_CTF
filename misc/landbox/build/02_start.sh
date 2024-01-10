#!/bin/sh
set -eu

##
## 1. Proof-of-Work
##
LENGTH=9
STRENGTH=26
challenge=`dd bs=32 count=1 if=/dev/urandom 2>/dev/null | base64 | tr +/ ab | cut -c -$LENGTH`
echo hashcash -mb$STRENGTH $challenge

echo "hashcash token: "
read token
if [ `expr "$token" : "^[a-zA-Z0-9\_\+\.\:\/]\{52\}$"` -eq 52 ]; then
    hashcash -cdb$STRENGTH -f /tmp/hashcash.sdb -r $challenge $token 2> /dev/null
    if [ $? -eq 0 ]; then
        echo "[+] Correct"
    else
        echo "[-] Wrong"
        exit
    fi
else
    echo "[-] Invalid token"
    exit
fi

##
## 2. Download your exploit
##
s=`dd bs=18 count=1 if=/dev/urandom 2>/dev/null | base64 | tr +/ ab`

echo "exploit url: "
read url
wget -q "$url" -O "/tmp/$s" && chmod 555 "/tmp/$s"

##
## 3. Spawn instance
##
echo "[+] Spawning instance..."
python3 -c 'import pty; pty.spawn(["/usr/bin/docker", "run", "--rm", "--name", "'$s'", "--privileged", "-v", "'"/tmp/$s"':/app/exploit:ro", "-it", "landbox"])'
