// SPDX-License-Identifier: Apache-2.0
#include <hardware/camera.h>
#include <dlfcn.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <string.h>
#include <sys/mman.h>
#include <poll.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
struct IonAlloc { size_t len,align; unsigned mask,flags; void *handle; };
struct IonFd {void *handle;int fd;};
struct Mem {camera_memory_t m; bool mapped;};
static volatile int frames=0,jpegs=0;static volatile size_t jpegsize=0;
static void release_mem(camera_memory_t* p){Mem*q=(Mem*)p;if(q->mapped)munmap(p->data,p->size);else free(p->data);free(q);}
static camera_memory_t* alloc_mem(int fd,size_t size,unsigned count,void*){Mem*q=(Mem*)calloc(1,sizeof(Mem));q->m.size=size*count;q->mapped=fd>=0;q->m.data=fd>=0?mmap(0,q->m.size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0):calloc(1,q->m.size);q->m.handle=q;q->m.release=release_mem;if(!q->m.data||q->m.data==MAP_FAILED){free(q);return 0;}return &q->m;}
static void notify(int type,int ext1,int ext2,void*){printf("NOTIFY %x %d %d\n",type,ext1,ext2);}
static void data_cb(int type,const camera_memory_t* mem,unsigned index,camera_frame_metadata_t*,void*){if(type==CAMERA_MSG_PREVIEW_FRAME){frames++;if(frames==1)printf("PREVIEW_CALLBACK bytes=%u index=%u\n",(unsigned)mem->size,index);}if(type==CAMERA_MSG_COMPRESSED_IMAGE){FILE*f=fopen("/tmp/goya-camera-stock.jpg","wb");if(f){fwrite(mem->data,1,mem->size,f);fclose(f);}jpegsize=mem->size;jpegs++;printf("JPEG_CALLBACK bytes=%u\n",(unsigned)mem->size);}}
static void timestamp_cb(int64_t,int,const camera_memory_t*,unsigned,void*){}
int main(int argc,char **argv) {
 if(argc==1 || !strcmp(argv[1],"capture")) {
  int fd=open("/dev/video0",O_RDWR|O_NONBLOCK);if(fd<0){perror("open video");return 1;}
  v4l2_capability cap={};if(ioctl(fd,VIDIOC_QUERYCAP,&cap)){perror("querycap");return 2;}
  printf("driver=%s card=%s capabilities=%x\n",cap.driver,cap.card,cap.capabilities);
  for(unsigned i=0;i<32;i++){v4l2_fmtdesc f={};f.index=i;f.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;if(ioctl(fd,VIDIOC_ENUM_FMT,&f))break;printf("format=%c%c%c%c description=%s\n",f.pixelformat&255,f.pixelformat>>8&255,f.pixelformat>>16&255,f.pixelformat>>24&255,f.description);}
  for(unsigned i=0;i<32;i++){v4l2_frmsizeenum f={};f.index=i;f.pixel_format=V4L2_PIX_FMT_UYVY;if(ioctl(fd,VIDIOC_ENUM_FRAMESIZES,&f))break;printf("size=%ux%u type=%u\n",f.discrete.width,f.discrete.height,f.type);}
  if(argc>1) {
   v4l2_format f={};f.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;f.fmt.pix.width=640;f.fmt.pix.height=480;f.fmt.pix.pixelformat=V4L2_PIX_FMT_UYVY;f.fmt.pix.field=V4L2_FIELD_NONE;
   if(ioctl(fd,VIDIOC_S_FMT,&f)){perror("S_FMT");return 10;}
   printf("capture %ux%u stride=%u bytes=%u\n",f.fmt.pix.width,f.fmt.pix.height,f.fmt.pix.bytesperline,f.fmt.pix.sizeimage);
   v4l2_requestbuffers req={};req.count=4;req.type=f.type;req.memory=V4L2_MEMORY_USERPTR;
   if(ioctl(fd,VIDIOC_REQBUFS,&req)){perror("REQBUFS mmap");return 11;}
   void *ptr[8]={};size_t len[8]={};if(req.count>8)return 12;
   int ion=open("/dev/ion",O_RDWR);if(ion<0){perror("ion open");return 20;}
   for(unsigned i=0;i<req.count;i++){
    IonAlloc ia={((f.fmt.pix.sizeimage+4095)/4096)*4096,4096,4,0,0};
    if(ioctl(ion,_IOWR('I',0,IonAlloc),&ia)){perror("ION_ALLOC");return 21;}
    IonFd ifd={ia.handle,-1};if(ioctl(ion,_IOWR('I',4,IonFd),&ifd)){perror("ION_MAP");return 22;}
    len[i]=ia.len;ptr[i]=mmap(0,ia.len,PROT_READ|PROT_WRITE,MAP_SHARED,ifd.fd,0);close(ifd.fd);
    if(ptr[i]==MAP_FAILED){perror("ion mmap");return 23;}
    ioctl(ion,_IOWR('I',1,void *),&ia.handle);
    v4l2_buffer b={};b.type=f.type;b.memory=V4L2_MEMORY_USERPTR;b.index=i;b.m.userptr=(unsigned long)ptr[i];b.length=len[i];
    if(ioctl(fd,VIDIOC_QBUF,&b)){perror("QBUF");return 15;}
   }
   int type=f.type;if(ioctl(fd,VIDIOC_STREAMON,&type)){perror("STREAMON");return 16;}
   for(int i=0;i<20;i++){pollfd p={fd,POLLIN,0};if(poll(&p,1,3000)<=0){perror("poll frame");return 17;}v4l2_buffer b={};b.type=f.type;b.memory=V4L2_MEMORY_USERPTR;if(ioctl(fd,VIDIOC_DQBUF,&b)){perror("DQBUF");return 18;}unsigned hash=2166136261u;for(unsigned j=0;j<b.bytesused;j++)hash=(hash^((unsigned char *)ptr[b.index])[j])*16777619u;if(i==19){FILE *out=fopen("/tmp/goya-camera-frame.uyvy","wb");if(out){fwrite(ptr[b.index],1,b.bytesused,out);fclose(out);}}printf("frame=%d seq=%u bytes=%u hash=%08x timestamp=%ld.%06ld\n",i,b.sequence,b.bytesused,hash,b.timestamp.tv_sec,b.timestamp.tv_usec);if(ioctl(fd,VIDIOC_QBUF,&b)){perror("requeue");return 19;}}
   ioctl(fd,VIDIOC_STREAMOFF,&type);for(unsigned i=0;i<req.count;i++)munmap(ptr[i],len[i]);puts("CAPTURE_CLOSED");
  }
  close(fd);return 0;
 }
 void *shim=dlopen("/tmp/goya-camera/libgoya_camera_legacy.so",RTLD_NOW|RTLD_GLOBAL);if(!shim){fprintf(stderr,"SHIM_ERROR %s\n",dlerror());return 24;}
 void *lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);if(!lib){fprintf(stderr,"LOAD_ERROR: %s\n",dlerror());return 3;}
 camera_module_t *m=(camera_module_t *)dlsym(lib,HAL_MODULE_INFO_SYM_AS_STR);if(!m)return 4;
 printf("HAL name=%s api=%x\n",m->common.name,m->common.module_api_version);
 int n=m->get_number_of_cameras();printf("cameras=%d\n",n);
 for(int i=0;i<n;i++){camera_info info={};int rc=m->get_camera_info(i,&info);printf("camera=%d info_rc=%d facing=%d orientation=%d\n",i,rc,info.facing,info.orientation);}
 if(argc>2 && (!strcmp(argv[2],"open")||!strcmp(argv[2],"preview"))) {
  hw_device_t *device=nullptr;int rc=m->common.methods->open(&m->common,"0",&device);printf("open_rc=%d device=%p\n",rc,device);
  if(device){camera_device_t *c=(camera_device_t *)device;char *p=c->ops->get_parameters(c);if(p){printf("PARAMETERS %s\n",p);c->ops->put_parameters(c,p);}if(!strcmp(argv[2],"preview")){c->ops->set_callbacks(c,notify,data_cb,timestamp_cb,alloc_mem,0);c->ops->enable_msg_type(c,CAMERA_MSG_PREVIEW_FRAME|CAMERA_MSG_COMPRESSED_IMAGE|CAMERA_MSG_SHUTTER|CAMERA_MSG_ERROR);int r=c->ops->start_preview(c);printf("START_PREVIEW %d\n",r);sleep(5);printf("PREVIEW_FRAMES %d\n",frames);if(!r){r=c->ops->take_picture(c);printf("TAKE_PICTURE %d\n",r);for(int i=0;i<15&&!jpegs;i++)sleep(1);}c->ops->stop_preview(c);printf("STOP_PREVIEW frames=%d jpegs=%d bytes=%u\n",frames,jpegs,(unsigned)jpegsize);}device->close(device);puts("CAMERA_CLOSED");}
  if(rc)return 5;
 }
 return 0;
}
