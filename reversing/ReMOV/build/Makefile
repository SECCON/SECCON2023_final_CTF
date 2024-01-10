all: obfuscated.S
	nasm obfuscated.S -fELF64
	gcc -nostdlib -nostartfiles -pie obfuscated.o -o mov
	strip --strip-all mov
	rm obfuscated.S

obfuscated.S: main.c
#	gcc -S main.c -o main.S -fno-asynchronous-unwind-tables -masm=intel -O1
	gcc main.c -o chall -nostdlib -nostartfiles -static-pie -masm=intel -O1
	objdump -S -M intel chall > disasm.txt
	python obfuscate.py > obfuscated.S
	rm chall disasm.txt
