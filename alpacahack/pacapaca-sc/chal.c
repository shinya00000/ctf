#define _GNU_SOURCE 1

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <seccomp.h>

int main(void){
    void *shellcode;
    ssize_t n;
    scmp_filter_ctx ctx;

    shellcode = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE | PROT_EXEC,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (shellcode == MAP_FAILED) _exit(1);
    printf("paca?\n");
    n = read(0, shellcode, 0x1000);
    if (n <= 0) _exit(1);

    ctx = seccomp_init(SCMP_ACT_KILL);
    if (ctx == NULL) _exit(1);

    if (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(read), 0) < 0) _exit(1);
    if (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(write), 0) < 0) _exit(1);
    if (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(open), 0) < 0) _exit(1);
    if (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(openat), 0) < 0) _exit(1);

    if (seccomp_load(ctx) < 0) _exit(1);
    seccomp_release(ctx);
    printf("paca!\n");
    ((void (*)(void))shellcode)();
    
}