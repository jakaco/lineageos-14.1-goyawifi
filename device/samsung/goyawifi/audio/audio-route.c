#include <tinyalsa/asoundlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc,char **argv) {
    if(argc!=2) return 2;
    FILE *f=fopen(argv[1],"r"); if(!f) return 3;
    struct mixer *m=mixer_open(0); if(!m) return 4;
    char line[1024],kind[32],name[128]; int rc=0;
    while(fgets(line,sizeof(line),f)) {
        if(sscanf(line,"%31s %127s",kind,name)!=2) continue;
        if(!strcmp(kind,"sleep")) {usleep(atoi(name)*1000);continue;}
        struct mixer_ctl *c=mixer_get_ctl_by_name(m,name);
        if(!c) {fprintf(stderr,"missing %s\n",name);rc=5;break;}
        if(!strcmp(kind,"reg")) {
            int mask,value; sscanf(line,"%*s %*s %d %d",&mask,&value);
            int old=mixer_ctl_get_value(c,0);
            if(old<0 || mixer_ctl_set_value(c,0,(old&~mask)|value)) {rc=6;break;}
            printf("%s %02x -> %02x\n",name,old,(old&~mask)|value);
        } else if(!strcmp(kind,"bulk")) {
            long vals[66]={0}; char *s=strchr(strchr(line,' ')+1,' '); int n=0;
            char *v=strtok(s," \n"); while(v && n<64){vals[++n]=strtol(v,0,0);v=strtok(0," \n");}
            vals[0]=n;
            if(mixer_ctl_set_array(c,vals,66)) {fprintf(stderr,"bulk failed\n");rc=7;break;}
        }
    }
    fclose(f);mixer_close(m);return rc;
}
