int main() {
  char flag[48], *buf;
  init();

  printf("FLAG: ");
  if (scanf("%47s", flag) != 1 ||
      strlen(flag) != 40 ||
      memcmp(flag, "SECCON{", 7) != 0 ||
      strcmp(flag+32+7, "}") != 0)
    goto wrong;

  buf = flag + 7;
  for (unsigned i = 0; i < 32; i++)
    if (buf[i] <= ' ' || buf[i] >= '}')
      goto wrong;

  for (unsigned i = 0; i < 32; i++) {
    unsigned index = 0, offset = 0, base = 1;
    unsigned route = (unsigned)buf[i] << 5 | i;
    for (unsigned j = 0; j < 5+7; j++) {
      size_t p = (size_t)FUNC_0000 + (index + offset + (route & 1)) * 0x20 + 0x10;
      ((size_t (*)())p)();
      base <<= 1;
      offset <<= 1;
      offset += 2 * (route & 1);
      route >>= 1;
      index += base;
    }
  }

  puts("Correct!");
  return 0;

 wrong:
  puts("Wrong...");
  return 1;
}
