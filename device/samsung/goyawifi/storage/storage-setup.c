/* Nougat rootdir/init.rc storage propagation, before vold and zygote start. */
#include <sys/mount.h>
#include <unistd.h>
#include <stdio.h>
int main(void) {
 if(getuid()!=0)return 1;
 if(mount(NULL,"/",NULL,MS_SHARED|MS_REC,NULL)){perror("root shared");return 2;}
 if(mount("/mnt/runtime/default","/storage",NULL,MS_BIND|MS_REC,NULL)){perror("storage bind");return 3;}
 if(mount(NULL,"/storage",NULL,MS_SLAVE|MS_REC,NULL)){perror("storage slave");return 4;}
 puts("STORAGE_SETUP_READY");return 0;
}
