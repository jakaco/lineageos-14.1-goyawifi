/* SD8777 H4 transport for Nougat. Firmware and power-save are owned by mbt8xxx. */
#define LOG_TAG "goya-bt-vendor"
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <cutils/log.h>
#include "hci/include/bt_vendor_lib.h"
static const bt_vendor_callbacks_t *callbacks;
static int transport=-1;
static int radio_power_unlocked(int on) {
 DIR *dir=opendir("/sys/class/rfkill");if(!dir)return -ENODEV;
 struct dirent *entry;
 int ret=-ENODEV;
 while((entry=readdir(dir))) {
  if(strncmp(entry->d_name,"rfkill",6))continue;
  char type[32]={0},path[512];snprintf(path,sizeof(path),"/sys/class/rfkill/%s/type",entry->d_name);
  int fd=open(path,O_RDONLY|O_CLOEXEC);
  if(fd<0)continue;int n=read(fd,type,sizeof(type)-1);close(fd);
  if(n<9 || strncmp(type,"bluetooth",9))continue;
  snprintf(path,sizeof(path),"/sys/class/rfkill/%s/state",entry->d_name);
  fd=open(path,O_WRONLY|O_CLOEXEC);if(fd<0){ret=-errno;break;}
  ret=write(fd,on?"1":"0",1)==1?0:-errno;close(fd);break;
 }
 closedir(dir);
 if(!ret && on) {
  /* mmc_detect_change_sync can time out before the async SDIO probes finish. */
  for(int retry=0;retry<100;retry++) {
   if(access("/dev/mbtchar0",F_OK)==0)break;
   usleep(20000);
  }
  if(access("/dev/mbtchar0",F_OK))return -ENODEV;
  /* If Wi-Fi's power vote is off, keep the freshly enumerated WLAN blocked. */
  char state=0;int fd=open("/sys/class/rfkill/rfkill0/soft",O_RDONLY|O_CLOEXEC);
  if(fd<0){ALOGE("Cannot read WLAN rfkill soft state: %d",errno);return -errno;}
  int n=read(fd,&state,1);close(fd);if(n!=1)return -EIO;
  if(state=='1' && (dir=opendir("/sys/class/ieee80211/mwiphy0"))) {
   while((entry=readdir(dir))) {
    if(strncmp(entry->d_name,"rfkill",6))continue;
    char path[512];snprintf(path,sizeof(path),"/sys/class/ieee80211/mwiphy0/%s/state",entry->d_name);
    for(int retry=0;retry<20;retry++) {fd=open(path,O_WRONLY|O_CLOEXEC);if(fd>=0)break;usleep(10000);}
    if(fd>=0){write(fd,"0",1);close(fd);}
   }
   closedir(dir);
  }
 }
 return ret;
}
static int radio_power(int on) {
 int lock=open("/data/misc/goya-radio.lock",O_RDWR|O_CLOEXEC);
 if(lock<0)return -errno;
 if(flock(lock,LOCK_EX)){int error=-errno;close(lock);return error;}
 int ret=radio_power_unlocked(on);flock(lock,LOCK_UN);close(lock);return ret;
}
static int init(const bt_vendor_callbacks_t *cb,unsigned char *address) {
 (void)address;if(!cb)return -EINVAL;callbacks=cb;return 0;
}
static void close_transport(void) {if(transport>=0){close(transport);transport=-1;}}
static int op(bt_vendor_opcode_t opcode,void *param) {
 if(!callbacks)return -EINVAL;
 switch(opcode) {
 case BT_VND_OP_POWER_CTRL:
  /* Kernel arbitration only cuts the shared supply after both votes are off. */
  return radio_power(*(int *)param==BT_VND_PWR_ON);
 case BT_VND_OP_USERIAL_OPEN:
  if(transport>=0)return -EBUSY;
  for(int retry=0;retry<50;retry++) {
   transport=open("/dev/mbtchar0",O_RDWR|O_NOCTTY|O_NONBLOCK|O_CLOEXEC);
   if(transport>=0)break;usleep(20000);
  }
  if(transport<0){ALOGE("open mbtchar0 failed errno=%d",errno);return -1;}
  for(int i=0;i<CH_MAX;i++)((int *)param)[i]=transport;
  ALOGI("Opened SD8777 H4 transport");return 1;
 case BT_VND_OP_USERIAL_CLOSE:close_transport();return 0;
 case BT_VND_OP_FW_CFG:
  /* Module probe completed the combo firmware setup before creating this node. */
  callbacks->fwcfg_cb(transport>=0?BT_VND_OP_RESULT_SUCCESS:BT_VND_OP_RESULT_FAIL);return 0;
 case BT_VND_OP_SCO_CFG:callbacks->scocfg_cb(BT_VND_OP_RESULT_SUCCESS);return 0;
 case BT_VND_OP_GET_LPM_IDLE_TIMEOUT:*(uint32_t *)param=1000;return 0;
 case BT_VND_OP_LPM_SET_MODE:callbacks->lpm_cb(BT_VND_OP_RESULT_SUCCESS);return 0;
 case BT_VND_OP_LPM_WAKE_SET_STATE:return 0;
 case BT_VND_OP_SET_AUDIO_STATE:
  if(callbacks->audio_state_cb)callbacks->audio_state_cb(BT_VND_OP_RESULT_SUCCESS);return 0;
 case BT_VND_OP_EPILOG:callbacks->epilog_cb(BT_VND_OP_RESULT_SUCCESS);return 0;
 default:return -ENOSYS;
 }
}
static void cleanup(void){close_transport();callbacks=NULL;}
const bt_vendor_interface_t BLUETOOTH_VENDOR_LIB_INTERFACE={
 .size=sizeof(bt_vendor_interface_t),.init=init,.op=op,.cleanup=cleanup
};
