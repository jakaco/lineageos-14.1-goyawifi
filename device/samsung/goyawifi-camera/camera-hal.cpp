// SPDX-License-Identifier: Apache-2.0
#define LOG_TAG "GoyaCamera"
#include <hardware/camera.h>
#include <hardware/gralloc.h>
#include <camera/CameraParameters.h>
#include <ui/GraphicBufferMapper.h>
#include <ui/Rect.h>
#include <utils/Log.h>
#include <libyuv.h>
#include <stdio.h>
#include <jpeglib.h>
#include <setjmp.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <linux/videodev2.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <string>
#include <errno.h>
using namespace android;
struct IonAlloc {size_t len,align;unsigned mask,flags;void*handle;};
struct IonFd {void*handle;int fd;};
class Sensor {
 int fd=-1,ion=-1; bool streaming=false;void*ptr[4]={};size_t len[4]={};
public:
 int width=0,height=0,stride=0;
 ~Sensor(){stop();}
 void stop(bool closeDevice=true){if(fd>=0&&streaming){int type=V4L2_BUF_TYPE_VIDEO_CAPTURE;ioctl(fd,VIDIOC_STREAMOFF,&type);}streaming=false;if(closeDevice&&fd>=0){close(fd);fd=-1;}for(int i=0;i<4;i++){if(ptr[i]&&ptr[i]!=MAP_FAILED)munmap(ptr[i],len[i]);ptr[i]=0;}if(ion>=0)close(ion);ion=-1;}
 int start(int w,int h,bool still=false){stop(false);if(fd<0)fd=open("/dev/video0",O_RDWR|O_NONBLOCK|O_CLOEXEC);if(fd<0)return -errno;
  v4l2_dbg_register ctrl={};ctrl.reg=V4L2_CID_BASE+((still&&w==1600)?68:69);if(ioctl(fd,VIDIOC_DBG_S_REGISTER,&ctrl))return -errno;ctrl.reg=V4L2_CID_BASE+49;ctrl.val=24;if(!still&&ioctl(fd,VIDIOC_DBG_S_REGISTER,&ctrl))return -errno;
  v4l2_format f={};f.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;f.fmt.pix.width=w;f.fmt.pix.height=h;f.fmt.pix.pixelformat=V4L2_PIX_FMT_UYVY;f.fmt.pix.field=V4L2_FIELD_NONE;
  if(ioctl(fd,VIDIOC_S_FMT,&f))return -errno;if(f.fmt.pix.width!=(unsigned)w||f.fmt.pix.height!=(unsigned)h)return -EINVAL;
  width=w;height=h;stride=f.fmt.pix.bytesperline?f.fmt.pix.bytesperline:w*2;
  v4l2_requestbuffers req={};req.count=4;req.type=f.type;req.memory=V4L2_MEMORY_USERPTR;if(ioctl(fd,VIDIOC_REQBUFS,&req))return -errno;if(req.count>4||req.count<2)return -EINVAL;
  ion=open("/dev/ion",O_RDWR|O_CLOEXEC);if(ion<0)return -errno;
  for(unsigned i=0;i<req.count;i++){
   IonAlloc a={(f.fmt.pix.sizeimage+4095u)&~4095u,4096,4,0,0};if(ioctl(ion,_IOWR('I',0,IonAlloc),&a))return -errno;
   IonFd x={a.handle,-1};int rc=ioctl(ion,_IOWR('I',4,IonFd),&x);int saved=errno;ioctl(ion,_IOWR('I',1,void*),&a.handle);if(rc){errno=saved;return -errno;}
   len[i]=a.len;ptr[i]=mmap(0,a.len,PROT_READ|PROT_WRITE,MAP_SHARED,x.fd,0);close(x.fd);if(ptr[i]==MAP_FAILED)return -errno;
   v4l2_buffer b={};b.type=f.type;b.memory=req.memory;b.index=i;b.m.userptr=(unsigned long)ptr[i];b.length=len[i];if(ioctl(fd,VIDIOC_QBUF,&b))return -errno;
  }
  int type=f.type;if(ioctl(fd,VIDIOC_STREAMON,&type))return -errno;streaming=true;ALOGI("sensor started %dx%d",w,h);return 0;
 }
 int next(v4l2_buffer& b){pollfd p={fd,POLLIN,0};int rc=poll(&p,1,700);if(rc<=0)return rc==0?-EAGAIN:-errno;b={};b.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;b.memory=V4L2_MEMORY_USERPTR;if(ioctl(fd,VIDIOC_DQBUF,&b))return -errno;if(b.index>=4||b.bytesused>len[b.index])return -EIO;return 0;}
 const uint8_t*data(const v4l2_buffer&b){return (const uint8_t*)ptr[b.index];}
 int queue(v4l2_buffer&b){return ioctl(fd,VIDIOC_QBUF,&b)?-errno:0;}
};
struct JpegError {jpeg_error_mgr base;jmp_buf jump;};
static void jpeg_error(j_common_ptr c){longjmp(((JpegError*)c->err)->jump,1);}
static bool makeJpeg(const uint8_t*uyvy,int stride,int w,int h,int quality,int rotation,std::vector<uint8_t>&out){
 std::vector<uint8_t> bgra(w*h*4),rgb(w*h*3);libyuv::UYVYToARGB(uyvy,stride,bgra.data(),w*4,w,h);libyuv::ARGBToRAW(bgra.data(),w*4,rgb.data(),w*3,w,h);
 jpeg_compress_struct c={};JpegError e={};c.err=jpeg_std_error(&e.base);e.base.error_exit=jpeg_error;unsigned char*mem=0;unsigned long size=0;
 if(setjmp(e.jump)){jpeg_destroy_compress(&c);free(mem);return false;}jpeg_create_compress(&c);jpeg_mem_dest(&c,&mem,&size);c.image_width=w;c.image_height=h;c.input_components=3;c.in_color_space=JCS_RGB;jpeg_set_defaults(&c);jpeg_set_quality(&c,quality,TRUE);jpeg_start_compress(&c,TRUE);
 unsigned orientation=rotation==90?6:rotation==180?3:rotation==270?8:1;
 const unsigned char exif[]={ 'E','x','i','f',0,0,'I','I',42,0,8,0,0,0,1,0,0x12,1,3,0,1,0,0,0,(unsigned char)orientation,0,0,0,0,0,0,0};jpeg_write_marker(&c,JPEG_APP0+1,exif,sizeof(exif));
 while(c.next_scanline<c.image_height){JSAMPROW row=rgb.data()+c.next_scanline*w*3;jpeg_write_scanlines(&c,&row,1);}jpeg_finish_compress(&c);out.assign(mem,mem+size);jpeg_destroy_compress(&c);free(mem);return true;
}
struct Camera {
 camera_device_t device={};std::mutex lock;std::thread worker;std::atomic<bool>running{false},picture{false},cancelled{false},focusRequested{false};std::atomic<int>messages{0};
 preview_stream_ops_t*window=0;camera_notify_callback notify=0;camera_data_callback data=0;camera_request_memory allocator=0;void*cookie=0;CameraParameters params;int displayOrientation=0;std::atomic<unsigned>frames{0},errors{0};
 Camera(){
  params.setPreviewSize(640,480);params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES,"640x480,320x240");params.setPreviewFormat(CameraParameters::PIXEL_FORMAT_YUV420SP);params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS,"yuv420sp");
  params.setPreviewFrameRate(15);params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES,"15");params.set(CameraParameters::KEY_PREVIEW_FPS_RANGE,"7000,24000");params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE,"(7000,24000)");
  params.setPictureSize(1600,1200);params.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES,"640x480,1600x1200");params.setPictureFormat(CameraParameters::PIXEL_FORMAT_JPEG);params.set(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS,"jpeg");params.set(CameraParameters::KEY_JPEG_QUALITY,90);
  params.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH,0);params.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT,0);params.set(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES,"0x0");params.set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY,80);
  params.set(CameraParameters::KEY_FOCUS_MODE,CameraParameters::FOCUS_MODE_FIXED);params.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES,"fixed");params.set(CameraParameters::KEY_FOCUS_DISTANCES,"0.05,2,Infinity");params.set(CameraParameters::KEY_FOCAL_LENGTH,"2.79");params.set(CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE,"50");params.set(CameraParameters::KEY_VERTICAL_VIEW_ANGLE,"50");
  params.set(CameraParameters::KEY_WHITE_BALANCE,"auto");params.set(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE,"auto");params.set(CameraParameters::KEY_EFFECT,"none");params.set(CameraParameters::KEY_SUPPORTED_EFFECTS,"none");params.set(CameraParameters::KEY_ANTIBANDING,"50hz");params.set(CameraParameters::KEY_SUPPORTED_ANTIBANDING,"50hz");params.set(CameraParameters::KEY_SCENE_MODE,"auto");params.set(CameraParameters::KEY_SUPPORTED_SCENE_MODES,"auto");
  params.set(CameraParameters::KEY_ZOOM,0);params.set(CameraParameters::KEY_MAX_ZOOM,0);params.set(CameraParameters::KEY_ZOOM_SUPPORTED,"false");params.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED,"false");params.set(CameraParameters::KEY_ZOOM_RATIOS,"100");params.set(CameraParameters::KEY_EXPOSURE_COMPENSATION,0);params.set(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION,0);params.set(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION,0);params.set(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP,"1");
  params.set(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS,0);params.set(CameraParameters::KEY_MAX_NUM_METERING_AREAS,0);params.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED,"false");params.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED,"false");params.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW,0);params.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_SW,0);params.set(CameraParameters::KEY_VIDEO_SNAPSHOT_SUPPORTED,"false");params.set(CameraParameters::KEY_RECORDING_HINT,"false");params.set(CameraParameters::KEY_ROTATION,0);
 }
 ~Camera(){stop();}
 void stop(){running=false;if(worker.joinable())worker.join();}
 void error(int code){errors++;ALOGE("capture error %d",code);camera_notify_callback n;void*u;{std::lock_guard<std::mutex>g(lock);n=notify;u=cookie;}if(n&&(messages&CAMERA_MSG_ERROR))n(CAMERA_MSG_ERROR,CAMERA_ERROR_UNKNOWN,0,u);}
 int configureWindow(){if(!window)return 0;int min=0,rc=window->get_min_undequeued_buffer_count(window,&min);if(rc)return rc;if((rc=window->set_usage(window,GRALLOC_USAGE_SW_WRITE_OFTEN|GRALLOC_USAGE_HW_TEXTURE)))return rc;if((rc=window->set_buffer_count(window,min+3)))return rc;int w,h;params.getPreviewSize(&w,&h);return window->set_buffers_geometry(window,w,h,HAL_PIXEL_FORMAT_BGRA_8888);}
 void render(const uint8_t*src,int stride,int w,int h){std::lock_guard<std::mutex>g(lock);if(window){buffer_handle_t*buf=0;int outStride=0;int rc=window->dequeue_buffer(window,&buf,&outStride);if(rc){ALOGW("dequeue %d",rc);return;}void*dst=0;rc=GraphicBufferMapper::get().lock(*buf,GRALLOC_USAGE_SW_WRITE_OFTEN,Rect(w,h),&dst);if(!rc&&dst){libyuv::UYVYToARGB(src,stride,(uint8_t*)dst,outStride*4,w,h);GraphicBufferMapper::get().unlock(*buf);if(window->set_timestamp)window->set_timestamp(window,systemTime(SYSTEM_TIME_MONOTONIC));window->enqueue_buffer(window,buf);}else{ALOGW("gralloc lock %d",rc);window->cancel_buffer(window,buf);}}}
 void previewCallback(const uint8_t*src,int stride,int w,int h){camera_data_callback d;camera_request_memory a;void*u;{std::lock_guard<std::mutex>g(lock);d=data;a=allocator;u=cookie;}if(!(messages&CAMERA_MSG_PREVIEW_FRAME)||!d||!a)return;camera_memory_t*m=a(-1,w*h*3/2,1,u);if(!m)return;std::vector<uint8_t>i420(w*h*3/2);uint8_t*y=i420.data(),*cb=y+w*h,*cr=cb+w*h/4;libyuv::UYVYToI420(src,stride,y,w,cb,w/2,cr,w/2,w,h);libyuv::I420ToNV21(y,w,cb,w/2,cr,w/2,(uint8_t*)m->data,w,(uint8_t*)m->data+w*h,w,w,h);d(CAMERA_MSG_PREVIEW_FRAME,m,0,0,u);m->release(m);}
 void photograph(Sensor&sensor){int w,h,q,r;camera_data_callback d;camera_notify_callback n;camera_request_memory a;void*u;{std::lock_guard<std::mutex>g(lock);params.getPictureSize(&w,&h);q=params.getInt(CameraParameters::KEY_JPEG_QUALITY);r=params.getInt(CameraParameters::KEY_ROTATION);d=data;n=notify;a=allocator;u=cookie;}
  int rc=sensor.start(w,h,true);if(rc){error(rc);return;}v4l2_buffer b={};int good=0;for(int attempt=0;attempt<30&&running&&!cancelled;attempt++){rc=sensor.next(b);if(rc==-EAGAIN)continue;if(rc){error(rc);return;}if(++good<4){sensor.queue(b);continue;}break;}
  if(!running||cancelled||good<4)return;if(n&&(messages&CAMERA_MSG_SHUTTER))n(CAMERA_MSG_SHUTTER,0,0,u);std::vector<uint8_t>jpeg;bool ok=makeJpeg(sensor.data(b),sensor.stride,w,h,q,r,jpeg);sensor.queue(b);sensor.stop();if(!ok){error(-EIO);return;}
  running=false;
  if(!cancelled&&d&&a&&(messages&CAMERA_MSG_COMPRESSED_IMAGE)){camera_memory_t*m=a(-1,jpeg.size(),1,u);if(m){memcpy(m->data,jpeg.data(),jpeg.size());d(CAMERA_MSG_COMPRESSED_IMAGE,m,0,0,u);m->release(m);ALOGI("JPEG %dx%d %u bytes",w,h,(unsigned)jpeg.size());}else error(-ENOMEM);}
 }
 void loop(){Sensor sensor;int w,h;{std::lock_guard<std::mutex>g(lock);params.getPreviewSize(&w,&h);}int rc=sensor.start(w,h);if(rc){error(rc);running=false;return;}while(running){if(focusRequested.exchange(false)){camera_notify_callback n;void*u;{std::lock_guard<std::mutex>g(lock);n=notify;u=cookie;}if(n&&(messages&CAMERA_MSG_FOCUS))n(CAMERA_MSG_FOCUS,1,0,u);}if(picture.exchange(false)){photograph(sensor);break;}v4l2_buffer b={};rc=sensor.next(b);if(rc==-EAGAIN)continue;if(rc){error(rc);break;}render(sensor.data(b),sensor.stride,w,h);previewCallback(sensor.data(b),sensor.stride,w,h);frames++;if(sensor.queue(b)){error(-EIO);break;}}sensor.stop();running=false;ALOGI("preview ended frames=%u errors=%u",frames.load(),errors.load());}
};
static Camera*get(camera_device_t*d){return (Camera*)d->priv;}
static int setWindow(camera_device_t*d,preview_stream_ops_t*w){Camera*c=get(d);std::lock_guard<std::mutex>g(c->lock);c->window=w;return c->configureWindow();}
static void callbacks(camera_device_t*d,camera_notify_callback n,camera_data_callback cb,camera_data_timestamp_callback,camera_request_memory a,void*u){Camera*c=get(d);std::lock_guard<std::mutex>g(c->lock);c->notify=n;c->data=cb;c->allocator=a;c->cookie=u;}
static void enable(camera_device_t*d,int32_t m){get(d)->messages.fetch_or(m);}
static void disable(camera_device_t*d,int32_t m){get(d)->messages.fetch_and(~m);}
static int enabled(camera_device_t*d,int32_t m){return (get(d)->messages&m)==m;}
static int start(camera_device_t*d){Camera*c=get(d);if(c->running)return 0;c->stop();{std::lock_guard<std::mutex>g(c->lock);int rc=c->configureWindow();if(rc)return rc;}c->cancelled=false;c->picture=false;c->running=true;c->worker=std::thread(&Camera::loop,c);return 0;}
static void stop(camera_device_t*d){get(d)->stop();}
static int previewing(camera_device_t*d){return get(d)->running;}
static int metadata(camera_device_t*,int){return -ENOSYS;}
static int record(camera_device_t*){return -ENOSYS;}
static void stopRecord(camera_device_t*){}
static int recording(camera_device_t*){return 0;}
static void releaseFrame(camera_device_t*,const void*){}
static int focus(camera_device_t*d){Camera*c=get(d);if(!c->running)return -EINVAL;c->focusRequested=true;return 0;}
static int cancelFocus(camera_device_t*d){get(d)->focusRequested=false;return 0;}
static int take(camera_device_t*d){Camera*c=get(d);if(!c->running)return -EINVAL;c->cancelled=false;c->picture=true;return 0;}
static int cancel(camera_device_t*d){get(d)->cancelled=true;return 0;}
static int setParams(camera_device_t*d,const char*s){Camera*c=get(d);CameraParameters p{String8(s)};int w,h,pw,ph;p.getPreviewSize(&w,&h);p.getPictureSize(&pw,&ph);if(!((w==640&&h==480)||(w==320&&h==240))||!((pw==1600&&ph==1200)||(pw==640&&ph==480)))return -EINVAL;const char*f=p.getPreviewFormat();const char*j=p.getPictureFormat();if(!f||strcmp(f,"yuv420sp")||!j||strcmp(j,"jpeg"))return -EINVAL;int q=p.getInt(CameraParameters::KEY_JPEG_QUALITY),r=p.getInt(CameraParameters::KEY_ROTATION);if(q<1||q>100||(r!=0&&r!=90&&r!=180&&r!=270))return -EINVAL;std::lock_guard<std::mutex>g(c->lock);int oldw,oldh;c->params.getPreviewSize(&oldw,&oldh);if(c->running&&(oldw!=w||oldh!=h))return -EBUSY;c->params=p;return 0;}
static char*params(camera_device_t*d){Camera*c=get(d);std::lock_guard<std::mutex>g(c->lock);return strdup(c->params.flatten().string());}
static void putParams(camera_device_t*,char*p){free(p);}
static int command(camera_device_t*d,int32_t cmd,int32_t a,int32_t){if(cmd==CAMERA_CMD_SET_DISPLAY_ORIENTATION){get(d)->displayOrientation=a;return 0;}if(cmd==CAMERA_CMD_ENABLE_SHUTTER_SOUND)return 0;return -EINVAL;}
static void release(camera_device_t*d){get(d)->stop();}
static int dump(camera_device_t*d,int fd){Camera*c=get(d);dprintf(fd,"Goya source HAL1: rear SR200PC20M, ION USERPTR, BGRA preview\nframes=%u errors=%u running=%d\n",c->frames.load(),c->errors.load(),(int)c->running.load());return 0;}
static camera_device_ops_t ops={setWindow,callbacks,enable,disable,enabled,start,stop,previewing,metadata,record,stopRecord,recording,releaseFrame,focus,cancelFocus,take,cancel,setParams,params,putParams,command,release,dump};
static std::mutex openLock;static bool opened=false;
static int closeCam(hw_device_t*d){Camera*c=(Camera*)((camera_device_t*)d)->priv;delete c;std::lock_guard<std::mutex>g(openLock);opened=false;return 0;}
static int openCam(const hw_module_t*m,const char*name,hw_device_t**out){if(!name||strcmp(name,"0")||!out)return -EINVAL;std::lock_guard<std::mutex>g(openLock);if(opened)return -EBUSY;Camera*c=new Camera;c->device.common.tag=HARDWARE_DEVICE_TAG;c->device.common.version=CAMERA_DEVICE_API_VERSION_1_0;c->device.common.module=const_cast<hw_module_t*>(m);c->device.common.close=closeCam;c->device.ops=&ops;c->device.priv=c;*out=&c->device.common;opened=true;return 0;}
static int count(){return 1;}
static int info(int id,camera_info*i){if(id!=0||!i)return -EINVAL;memset(i,0,sizeof(*i));i->facing=CAMERA_FACING_BACK;i->orientation=90;i->device_version=CAMERA_DEVICE_API_VERSION_1_0;return 0;}
static hw_module_methods_t methods={openCam};
extern "C" camera_module_t HAL_MODULE_INFO_SYM={{HARDWARE_MODULE_TAG,CAMERA_MODULE_API_VERSION_1_0,HARDWARE_HAL_API_VERSION,CAMERA_HARDWARE_MODULE_ID,"Goya SR200PC20M Camera","Goya port",&methods,0,{0}},count,info,0,0,0,0,0,0,{0}};
