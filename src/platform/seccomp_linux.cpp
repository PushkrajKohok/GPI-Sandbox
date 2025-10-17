#ifdef __linux__
#include "sandbox.h"
#include <linux/seccomp.h>
#include <linux/filter.h>
#include <linux/audit.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <string>

#ifndef SECCOMP_AUDIT_ARCH
# if defined(__x86_64__)
#  define SECCOMP_AUDIT_ARCH AUDIT_ARCH_X86_64
# else
#  define SECCOMP_AUDIT_ARCH 0
# endif
#endif

static struct sock_filter filter[] = {
  BPF_STMT(BPF_LD  | BPF_W | BPF_ABS, offsetof(struct seccomp_data, arch)),
  BPF_JUMP(BPF_JMP | BPF_JEQ| BPF_K, SECCOMP_AUDIT_ARCH, 0, 9),
  BPF_STMT(BPF_LD  | BPF_W | BPF_ABS, offsetof(struct seccomp_data, nr)),
  BPF_JUMP(BPF_JMP | BPF_JEQ| BPF_K, __NR_execve,  6, 0),
  BPF_JUMP(BPF_JMP | BPF_JEQ| BPF_K, __NR_fork,    5, 0),
  BPF_JUMP(BPF_JMP | BPF_JEQ| BPF_K, __NR_vfork,   4, 0),
  BPF_JUMP(BPF_JMP | BPF_JEQ| BPF_K, __NR_clone,   3, 0),
  BPF_JUMP(BPF_JMP | BPF_JEQ| BPF_K, __NR_socket,  2, 0),
  BPF_JUMP(BPF_JMP | BPF_JEQ| BPF_K, __NR_connect, 1, 0),
  BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | (EPERM & 0xFFFF)),
  BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
};

bool sandbox::install_seccomp_default(bool deny_network, std::string& err) {
  if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) { err="no_new_privs failed"; return false; }
  if (!deny_network) return true;

  struct sock_fprog prog;
  prog.len = sizeof(filter)/sizeof(filter[0]);
  prog.filter = filter;

  if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog) != 0) {
    err = std::string("seccomp failed: ") + strerror(errno);
    return false;
  }
  return true;
}
#else
#include "sandbox.h"
bool sandbox::install_seccomp_default(bool, std::string&) { return true; }
#endif
