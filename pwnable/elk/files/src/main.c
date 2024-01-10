#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "elk.h"

#define MEM_SIZE 0x100000

// Prints all arguments, one by one, delimit by space
static jsval_t js_print(struct js *js, jsval_t *args, int nargs) {
  for (int i = 0; i < nargs; i++) {
    const char *space = i == 0 ? "" : " ";
    printf("%s%s", space, js_str(js, args[i]));
  }
  putchar('\n');  // Finish by newline
  return js_mkundef();
}

/* Array.create */
static jsval_t js_array_create(struct js *js, jsval_t *args, int nargs) {
  if (!js_chkargs(args, nargs, "d"))
    return js_mkerr(js, "invalid argument");

  size_t len = (size_t) js_getnum(args[0]);
  if (len > 0x100)
    return js_mkerr(js, "size too big");

  return js_mkarr(js, len);
}

/* Array.get */
static jsval_t js_array_get(struct js *js, jsval_t *args, int nargs) {
  if (!js_chkargs(args, nargs, "ad"))
    return js_mkerr(js, "invalid argument");

  size_t i = (size_t) js_getnum(args[1]);
  if (i >= js_arrsize(js, args[0]))
    return js_mkerr(js, "index out-of-bounds");

  return js_arrget(js, args[0], i);
}

/* Array.set */
static jsval_t js_array_set(struct js *js, jsval_t *args, int nargs) {
  if (!js_chkargs(args, nargs, "adj"))
    return js_mkerr(js, "invalid argument");

  size_t i = (size_t) js_getnum(args[1]);
  if (i >= js_arrsize(js, args[0]))
    return js_mkerr(js, "index out-of-bounds");

  return js_arrset(js, args[0], i, args[2]);
}

/* Array.indexOf */
static jsval_t js_array_indexof(struct js *js, jsval_t *args, int nargs) {
  if (!js_chkargs(args, nargs, "aj"))
    return js_mkerr(js, "invalid argument");

  size_t n = js_arrsize(js, args[0]);
  for (size_t i = 0; i < n; i++)
    if (js_arrget(js, args[0], i) == args[1])
      return js_mknum(i);

  return js_mknum(-1);
}

int main(int argc, char *argv[]) {
  FILE *fp;
  long size;
  char *mem, *code;
  struct js *js;
  jsval_t array;

  // Open file
  if (argc < 2) {
    printf("Usage: %s <file.js>\n", argv[0]);
    return 1;
  }

  assert (fp = fopen(argv[1], "r"));
  assert (fseek(fp, 0, SEEK_END) == 0);
  assert ((size = ftell(fp)) > 0);
  assert (code = (char*)malloc(size));
  assert (fseek(fp, 0, SEEK_SET) == 0);
  assert (fread(code, 1, size, fp) == size);
  fclose(fp);

  // Create global
  assert (mem = (char*)malloc(MEM_SIZE));
  js = js_create(mem, MEM_SIZE);
  js_set(js, js_glob(js), "print", js_mkfun(js_print));

  // Create array
  array = js_mkobj(js);
  js_set(js, js_glob(js), "Array", array);
  js_set(js, array, "create" , js_mkfun(js_array_create));  // Array.create
  js_set(js, array, "get"    , js_mkfun(js_array_get));     // Array.get
  js_set(js, array, "set"    , js_mkfun(js_array_set));     // Array.set
  js_set(js, array, "indexOf", js_mkfun(js_array_indexof)); // Array.indexOf

  // Evaluate
  js_eval(js, code, ~0U);
  return EXIT_SUCCESS;
}
