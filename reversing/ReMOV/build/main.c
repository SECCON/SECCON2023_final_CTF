typedef unsigned long long uint64_t;
typedef unsigned long long size_t;
typedef long long ssize_t;

uint64_t seed = 0x1b75f5867fda13b0;
static const char enc[] = {
  0xc4, 0x0e, 0xba, 0x13, 0xe8, 0x71, 0xe6, 0xbd,
  0x2a, 0x83, 0xd3, 0xbf, 0x78, 0x38, 0x31, 0xfe,
  0x84, 0x7a, 0x74, 0xa7, 0x6f, 0x96, 0xe4, 0xef,
  0x53, 0xf0, 0x93, 0xcc, 0xcf, 0x45, 0x6a, 0xac,
};

int is_emulated() {
  size_t p;
  asm("xor edi, edi\n"
      "mov eax, 12\n"
      "syscall\n"
      "mov %0, rax\n"
      : "=r"(p));
  if (p < 0x100000000) return 1;
  asm("xor r10d, r10d\n"
      "mov edx, 1\n"
      "xor esi, esi\n"
      "xor edi, edi\n"
      "mov eax, 101\n"
      "syscall\n"
      "mov %0, rax\n"
      : "=r"(p));
  return p != 0;
}

void srand(uint64_t x) {
  seed = x;
}

uint64_t rand() {
  uint64_t x = seed;
  x ^= x << 13;
  x ^= x >> 7;
  x ^= x << 17;
  return seed = x;
}

size_t strlen(const char *s) {
  size_t len = 0;
  while (*s++) len++;
  return len;
}

int startswith(const char *s, const char *t) {
  while (*t)
    if (*s++ != *t++) return 0;
  return 1;
}

int endswith(const char *s, const char *t) {
  s += strlen(s) - strlen(t);
  while (*t)
    if (*s++ != *t++) return 0;
  if (is_emulated()) seed = -seed;
  return 1;
}

ssize_t write(int fd, const void *buf, size_t count) {
  asm("mov edi, %0\n"
      "mov rsi, %1\n"
      "mov rdx, %2\n"
      "mov eax, 1\n"
      "syscall"
      : : "r"(fd), "r"(buf), "r"(count));
}

void exit(int code) {
  asm("mov edi, %0\n"
      "mov eax, 60\n"
      "syscall\n"
      "hlt"
      : : "r"(code));
}

void _start(void) {
  int argc;
  char **argv, *flag;
  size_t len;
  uint64_t head;

  asm("mov %0, [rsp+16]" : "=r"(argc));
  asm("lea %0, [rsp+24]" : "=r"(argv));
  if (argc < 2) {
    write(1, "No flag given\n", 14);
    exit(1);
  }

  flag = argv[1];
  if ((len = strlen(flag)) != 32
      || !startswith(flag, "SECCON{")
      || !endswith(flag, "}"))
    goto wrong;

  head = *(uint64_t*)flag;
  *(uint64_t*)flag ^= rand();

  srand(head);
  for (size_t i = 8; i < len; i += 8)
    *(uint64_t*)(flag + i) ^= rand();

  for (size_t i = 0; i < len; i += 8)
    if (*(uint64_t*)(flag + i) != *(uint64_t*)(enc + i))
      goto wrong;

 correct:
  write(1, "Correct!\n", 9);
  exit(0);

 wrong:
  write(1, "Wrong...\n", 9);
  exit(1);
}
