/* runas.c — drop to <uid>/<gid> inside the chroot, then exec the requested
 * aarch64 binary through qemu-user (QEMU_EXECVE re-invoke), so the dbus peer
 * credentials observed by dbus-daemon/actionengined are the DROPPED uid.
 * Usage: runas <uid> <gid> <prog> [args...]
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: %s <uid> <gid> <prog> [args...]\n", argv[0]); return 2; }
    uid_t uid = (uid_t)atoi(argv[1]);
    gid_t gid = (gid_t)atoi(argv[2]);
    if (setresgid(gid, gid, gid) != 0) { perror("setresgid"); return 2; }
    if (setresuid(uid, uid, uid) != 0) { perror("setresuid"); return 2; }
    /* relies on binfmt_misc (registered by qemu-user-static) for the aarch64 exec */
    char **nargv = calloc((size_t)(argc - 2), sizeof(char *));
    for (int i = 3; i < argc; i++) nargv[i - 3] = argv[i];
    fprintf(stderr, "RUNAS uid=%d gid=%d exec %s\n", getuid(), getgid(), argv[3]);
    execv(argv[3], nargv);
    perror("execv");
    return 127;
}
