#define NX_FS_IMPLEMENTATION
#include "../fs.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
int main(void) {
    char root[]="/tmp/openrealm-fs-XXXXXX"; assert(mkdtemp(root));
    int fd=NX_Open(root,O_RDONLY|O_DIRECTORY);assert(fd>=0);
    assert(NX_MkdirAt(fd,"live",0700)==0);
    int sub=NX_OpenAt(fd,"live",O_RDONLY|O_DIRECTORY|O_NOFOLLOW);assert(sub>=0);
    int file=NX_OpenAt(sub,"slot",O_RDWR|O_CREAT|O_EXCL,0600);assert(file>=0);
    assert(write(file,"abcdef",6)==6);assert(lseek(file,2,SEEK_SET)==2);
    char bytes[4]={0};assert(NX_Pread(file,bytes,3,1)==3);assert(!strcmp(bytes,"bcd"));
    assert(lseek(file,0,SEEK_CUR)==2);assert(NX_Close(file)==0);
    struct stat st;assert(NX_StatAt(sub,"slot",&st,AT_SYMLINK_NOFOLLOW)==0 && st.st_size==6);
    int copy=NX_Dup(sub);assert(copy>=0);DIR *entries=NX_FdOpenDir(copy);assert(entries);assert(closedir(entries)==0);
    errno=0;assert(NX_Fstat(copy,&st)==-1 && errno==EBADF);
    assert(NX_RenameAt(sub,"slot",sub,"renamed")==0);
    errno=0;assert(NX_OpenAt(sub,"../escape",O_CREAT|O_WRONLY,0600)==-1 && errno==EACCES);
    char link[1024];snprintf(link,sizeof(link),"%s/live/link",root);assert(symlink("renamed",link)==0);
    errno=0;assert(NX_OpenAt(sub,"link",O_RDONLY|O_NOFOLLOW)==-1 && errno==ELOOP);
    assert(NX_UnlinkAt(sub,"link",0)==0);assert(NX_UnlinkAt(sub,"renamed",0)==0);
    assert(NX_Close(sub)==0);assert(NX_UnlinkAt(fd,"live",AT_REMOVEDIR)==0);
    assert(NX_Close(fd)==0);assert(rmdir(root)==0);
    puts("Switch filesystem adapter: directory traversal, copy offsets, rename and cleanup passed");
}
