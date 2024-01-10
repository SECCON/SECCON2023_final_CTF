#include <assert.h>
#include <fcntl.h>
#include <linux/landlock.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>

#define PATH_FLAG "/flag-XXXX.txt"

#define LANDLOCK_ACCESS_FS_REFER (1ULL << 13)
#define LANDLOCK_ACCESS_FS_TRUNCATE (1ULL << 14)

static struct landlock_ruleset_attr default_landlock_ruleset_attr = {
  .handled_access_fs =
  LANDLOCK_ACCESS_FS_EXECUTE |
  LANDLOCK_ACCESS_FS_WRITE_FILE |
  LANDLOCK_ACCESS_FS_READ_FILE |
  LANDLOCK_ACCESS_FS_READ_DIR |
  LANDLOCK_ACCESS_FS_REMOVE_DIR |
  LANDLOCK_ACCESS_FS_REMOVE_FILE |
  LANDLOCK_ACCESS_FS_MAKE_CHAR |
  LANDLOCK_ACCESS_FS_MAKE_DIR |
  LANDLOCK_ACCESS_FS_MAKE_REG |
  LANDLOCK_ACCESS_FS_MAKE_SOCK |
  LANDLOCK_ACCESS_FS_MAKE_FIFO |
  LANDLOCK_ACCESS_FS_MAKE_BLOCK |
  LANDLOCK_ACCESS_FS_MAKE_SYM |
  LANDLOCK_ACCESS_FS_REFER |
  LANDLOCK_ACCESS_FS_TRUNCATE,
};

static inline int
landlock_create_ruleset(const struct landlock_ruleset_attr *const attr,
                        const size_t size,
                        const __u32 flags) {
  return syscall(__NR_landlock_create_ruleset, attr, size, flags);
}

static inline int
landlock_restrict_self(const int ruleset_fd,
                       const __u32 flags) {
  return syscall(__NR_landlock_restrict_self, ruleset_fd, flags);
}

void give_up_flag(void) {
  int abi, ruleset_fd;
  struct landlock_ruleset_attr *ruleset_attr = &default_landlock_ruleset_attr;

  abi = landlock_create_ruleset(NULL, 0, LANDLOCK_CREATE_RULESET_VERSION);
  assert (abi >= 0);

  switch (abi) {
    case 1:
      ruleset_attr->handled_access_fs &= ~LANDLOCK_ACCESS_FS_REFER;
      __attribute__((fallthrough));
    case 2:
      ruleset_attr->handled_access_fs &= ~LANDLOCK_ACCESS_FS_TRUNCATE;
  }

  ruleset_fd = landlock_create_ruleset(ruleset_attr, sizeof(*ruleset_attr), 0);
  assert (ruleset_fd >= 0);

  assert (!prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0));

  landlock_restrict_self(ruleset_fd, 0);
}

void read_flag(void) {
  char flag[0x100];
  int fd;
  size_t s;

  fd = open(PATH_FLAG, O_RDONLY);
  if (fd < 0) {
    perror("Cannot read flag");
  } else {
    s = read(fd, flag, 0x100);
    assert (s >= 0);
    printf("FLAG: %s", flag);
    close(fd);
  }
}

int main() {
  give_up_flag();
  read_flag();
  return 0;
}
