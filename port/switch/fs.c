/* Directory-relative save storage for libnx's path-based FAT filesystem.
 * Directory descriptors are private to the engine; file descriptors remain real. */
#define NX_FS_IMPLEMENTATION
#include "fs.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/file.h>

#define NX_DIR_BASE 0x40000000 // descriptor base; disjoint from libnx device descriptors
#define NX_DIR_SLOTS 128 // live directories; bounded save-tree traversal depth and scanners
#define NX_PATH_MAX 1024 // bytes; SD root plus bounded script workspace components
static char dirs[NX_DIR_SLOTS][NX_PATH_MAX];

static char const *NX_DirPath(int fd) {
    int slot = fd - NX_DIR_BASE;
    if (slot < 0 || slot >= NX_DIR_SLOTS || !dirs[slot][0]) { errno = EBADF; return NULL; }
    return dirs[slot];
}

/* Resolve relative names without changing global CWD or escaping the anchored directory. */
static bool NX_PathAt(int fd, char const *path, char out[NX_PATH_MAX]) {
    if (!path || !*path) { errno = ENOENT; return false; }
    if (fd == AT_FDCWD || path[0] == '/' || strchr(path, ':')) {
        if (snprintf(out,NX_PATH_MAX,"%s",path) >= NX_PATH_MAX) { errno = ENAMETOOLONG; return false; }
        return true;
    }
    char const *base = NX_DirPath(fd);
    if (!base) return false;
    if (strstr(path,"..")) { errno = EACCES; return false; }
    if (snprintf(out,NX_PATH_MAX,"%s/%s",base,path) >= NX_PATH_MAX) { errno = ENAMETOOLONG; return false; }
    return true;
}

int NX_Open(char const *path, int flags, ...) {
    mode_t mode = 0;
    if (flags & O_CREAT) { va_list ap; va_start(ap,flags); mode = va_arg(ap,int); va_end(ap); }
    struct stat st;
    /* FAT has no symlinks; lstat also enforces the contract in host regression tests. */
    if ((flags & O_NOFOLLOW) && !lstat(path,&st) && S_ISLNK(st.st_mode)) { errno = ELOOP; return -1; }
    if (flags & O_DIRECTORY) {
        if (stat(path,&st)) return -1;
        if (!S_ISDIR(st.st_mode)) { errno = ENOTDIR; return -1; }
        if (strlen(path) >= NX_PATH_MAX) { errno = ENAMETOOLONG; return -1; }
        for (int i = 0; i < NX_DIR_SLOTS; i++) if (!dirs[i][0]) {
            strcpy(dirs[i],path); return NX_DIR_BASE + i;
        }
        errno = EMFILE; return -1;
    }
    return open(path,flags & ~(O_NOFOLLOW | O_CLOEXEC),mode);
}
int NX_Close(int fd) {
    if (fd < NX_DIR_BASE) return close(fd);
    if (!NX_DirPath(fd)) return -1;
    dirs[fd-NX_DIR_BASE][0] = 0; return 0;
}
int NX_Dup(int fd) {
    if (fd < NX_DIR_BASE) return dup(fd);
    char const *path = NX_DirPath(fd);
    return path ? NX_Open(path,O_RDONLY|O_DIRECTORY) : -1;
}
int NX_Fstat(int fd, struct stat *st) {
    if (fd < NX_DIR_BASE) return fstat(fd,st);
    char const *path = NX_DirPath(fd);
    return path ? stat(path,st) : -1;
}
int NX_OpenAt(int fd, char const *path, int flags, ...) {
    mode_t mode = 0; char full[NX_PATH_MAX];
    if (flags & O_CREAT) { va_list ap; va_start(ap,flags); mode = va_arg(ap,int); va_end(ap); }
    return NX_PathAt(fd,path,full) ? NX_Open(full,flags,mode) : -1;
}
int NX_StatAt(int fd, char const *path, struct stat *st, int flags) {
    char full[NX_PATH_MAX];
    if (!NX_PathAt(fd,path,full)) return -1;
    return flags & AT_SYMLINK_NOFOLLOW ? lstat(full,st) : stat(full,st);
}
int NX_MkdirAt(int fd, char const *path, mode_t mode) {
    char full[NX_PATH_MAX]; return NX_PathAt(fd,path,full) ? mkdir(full,mode) : -1;
}
int NX_UnlinkAt(int fd, char const *path, int flags) {
    char full[NX_PATH_MAX];
    if (!NX_PathAt(fd,path,full)) return -1;
    return flags & AT_REMOVEDIR ? rmdir(full) : unlink(full);
}
int NX_RenameAt(int from, char const *src, int to, char const *dst) {
    char a[NX_PATH_MAX], b[NX_PATH_MAX];
    return NX_PathAt(from,src,a) && NX_PathAt(to,dst,b) ? rename(a,b) : -1;
}
DIR *NX_FdOpenDir(int fd) {
    char const *path = NX_DirPath(fd);
    if (!path) return NULL;
    DIR *dir = opendir(path);
    if (dir) NX_Close(fd);
    return dir;
}
/* Save storage runs synchronously on the engine thread, so no reader shares this offset. */
ssize_t NX_Pread(int fd, void *buf, size_t size, off_t offset) {
    off_t old = lseek(fd,0,SEEK_CUR);
    if (old < 0 || lseek(fd,offset,SEEK_SET) < 0) return -1;
    ssize_t count = read(fd,buf,size);
    int saved = errno;
    if (lseek(fd,old,SEEK_SET) < 0) return -1;
    errno = saved; return count;
}
/* TODO: FAT has no advisory process locks. Save APIs run on one engine thread
 * in one foreground application; external concurrent writers are unsupported. */
int NX_Flock(int fd, int op) {
    struct stat st;
    if (fstat(fd,&st)) return -1;
    if (op != LOCK_EX && op != LOCK_SH && op != LOCK_UN) { errno = EINVAL; return -1; }
    return 0;
}
