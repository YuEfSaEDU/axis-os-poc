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
    /* qemu-user: re-invoke the emulator for the target binary we exec.
     * NOTE: the re-invoked qemu does NOT strip the binary path from argv, so the
     * program path is passed twice (once as qemu's target, once as argv[0]). */
    char **nargv = calloc((size_t)(argc + 3), sizeof(char *));
    nargv[0] = "/usr/bin/qemu-aarch64-static";
    nargv[1] = argv[3];
    nargv[2] = argv[3];
    for (int i = 3; i < argc; i++) nargv[i] = argv[i];
    fprintf(stderr, "RUNAS uid=%d gid=%d exec %s\n", getuid(), getgid(), argv[3]);
    execv("/usr/bin/qemu-aarch64-static", nargv);
    perror("execv");
    return 127;
}
