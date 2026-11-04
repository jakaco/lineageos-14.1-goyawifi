// SPDX-License-Identifier: Apache-2.0
#include <gui/SurfaceComposerClient.h>
#include <gui/SurfaceControl.h>
#include <gui/Surface.h>
#include <binder/ProcessState.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <stdio.h>
#include <unistd.h>
using namespace android;
static bool capture(const char* path) {
    ScreenshotClient shot;
    status_t rc=shot.update(SurfaceComposerClient::getBuiltInDisplay(0),Rect(),true);
    printf("capture=%d width=%u height=%u stride=%u format=%d\n",rc,shot.getWidth(),shot.getHeight(),shot.getStride(),shot.getFormat());
    if (rc || shot.getFormat()!=PIXEL_FORMAT_RGBA_8888) return false;
    FILE* f=fopen(path,"wb"); if(!f) return false;
    fprintf(f,"P6\n%u %u\n255\n",shot.getWidth(),shot.getHeight());
    auto pixels=static_cast<const unsigned char*>(shot.getPixels());
    for(uint32_t y=0;y<shot.getHeight();++y) for(uint32_t x=0;x<shot.getWidth();++x)
        fwrite(pixels+4*(y*shot.getStride()+x),1,3,f);
    fclose(f); return true;
}
int main() {
    setbuf(stdout,nullptr); ProcessState::self()->startThreadPool();
    sp<SurfaceComposerClient> client=new SurfaceComposerClient();
    if(client->initCheck()!=NO_ERROR) return 1;
    sp<SurfaceControl> control=client->createSurface(String8("Nougat RAM graphics test"),1024,600,PIXEL_FORMAT_RGBA_8888);
    if(control==nullptr || !control->isValid()) return 2;
    SurfaceComposerClient::openGlobalTransaction(); control->setLayer(100000);control->show();
    SurfaceComposerClient::closeGlobalTransaction();
    sp<Surface> surface=control->getSurface();
    EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY); if(!eglInitialize(d,nullptr,nullptr)) return 3;
    EGLint attrs[]={EGL_SURFACE_TYPE,EGL_WINDOW_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};
    EGLConfig cfg;EGLint n=0;if(!eglChooseConfig(d,attrs,&cfg,1,&n)||!n)return 4;
    EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,2,EGL_NONE};
    EGLContext c=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);
    EGLSurface s=eglCreateWindowSurface(d,cfg,surface.get(),nullptr);
    if(!eglMakeCurrent(d,s,s,c)){printf("makeCurrent error=%x\n",eglGetError());return 5;}
    bool captured1=false,captured2=false;
    for(int frame=0;frame<1200;++frame){
        glDisable(GL_SCISSOR_TEST);glClearColor(0,0,1,1);glClear(GL_COLOR_BUFFER_BIT);
        glEnable(GL_SCISSOR_TEST);glScissor((frame*7)%824,200,200,200);glClearColor(0,1,0,1);glClear(GL_COLOR_BUFFER_BIT);
        if(frame==0||frame==30||frame==90){
            unsigned char bg[4]={0},fg[4]={0};
            glReadPixels(10,10,1,1,GL_RGBA,GL_UNSIGNED_BYTE,bg);
            glReadPixels((frame*7)%824+100,300,1,1,GL_RGBA,GL_UNSIGNED_BYTE,fg);
            printf("window frame=%d bg=%u,%u,%u,%u fg=%u,%u,%u,%u error=%x\n",frame,bg[0],bg[1],bg[2],bg[3],fg[0],fg[1],fg[2],fg[3],glGetError());
        }
        if(!eglSwapBuffers(d,s)){printf("swap failed frame=%d error=%x\n",frame,eglGetError());return 6;}
        if(frame==30)captured1=capture("/tmp/surface-frame1.ppm");
        if(frame==90)captured2=capture("/tmp/surface-frame2.ppm");
        if(frame%120==0)printf("frames=%d\n",frame+1);
        usleep(25000);
    }
    printf("completed=1200 captures=%d,%d\n",captured1,captured2);
    eglMakeCurrent(d,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroySurface(d,s);eglDestroyContext(d,c);eglTerminate(d);
    return captured1&&captured2?0:7;
}
