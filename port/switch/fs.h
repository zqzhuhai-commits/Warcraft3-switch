#ifndef OPENREALM_SWITCH_FS_H
#define OPENREALM_SWITCH_FS_H
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
int NX_Open(char const *path, int flags, ...);
int NX_Close(int fd);
int NX_Dup(int fd);
int NX_Fstat(int fd, struct stat *st);
int NX_OpenAt(int fd, char const *path, int flags, ...);
int NX_StatAt(int fd, char const *path, struct stat *st, int flags);
int NX_MkdirAt(int fd, char const *path, mode_t mode);
int NX_UnlinkAt(int fd, char const *path, int flags);
int NX_RenameAt(int from, char const *src, int to, char const *dst);
DIR *NX_FdOpenDir(int fd);
ssize_t NX_Pread(int fd, void *buf, size_t size, off_t offset);
int NX_Flock(int fd, int op);
int NX_Random(void *buf, size_t size);
#ifndef NX_FS_IMPLEMENTATION
#define open NX_Open
#define close NX_Close
#define dup NX_Dup
#define fstat NX_Fstat
#define openat NX_OpenAt
#define fstatat NX_StatAt
#define mkdirat NX_MkdirAt
#define unlinkat NX_UnlinkAt
#define renameat NX_RenameAt
#define fdopendir NX_FdOpenDir
#define pread NX_Pread
#define flock NX_Flock
#endif
#endif
