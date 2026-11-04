// SPDX-License-Identifier: Apache-2.0
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/eglext.h>
#include <ui/GraphicBuffer.h>
#include <hardware/hardware.h>
#include <hardware/gralloc.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
int main(int argc, char** argv) {
    setbuf(stdout, nullptr);
    if (argc == 2 && !strcmp(argv[1], "--load")) {
        const char* paths[] = {"/system/lib/egl/libEGL_MRVL.so", "/system/lib/egl/libGLESv2_MRVL.so", "/system/lib/hw/gralloc.mrvl.so", "/system/lib/hw/hwcomposer.mrvl.so"};
        int failures = 0;
        for (const char* path : paths) {
            void* h = dlopen(path, RTLD_NOW | RTLD_LOCAL);
            printf("dlopen %s: %s\n", path, h ? "OK" : dlerror());
            if (!h) ++failures;
        }
        return failures ? 1 : 0;
    }
    EGLDisplay d = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint major = 0, minor = 0;
    if (!eglInitialize(d, &major, &minor)) { printf("eglInitialize failed %x\n", eglGetError()); return 2; }
    printf("EGL %d.%d vendor=%s version=%s\n", major, minor, eglQueryString(d,EGL_VENDOR), eglQueryString(d,EGL_VERSION));
    EGLint attrs[] = {EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};
    EGLConfig cfg; EGLint count=0;
    if (!eglChooseConfig(d,attrs,&cfg,1,&count) || !count) { printf("no config %x\n",eglGetError()); return 3; }
    EGLint pbuf[] = {EGL_WIDTH,16,EGL_HEIGHT,16,EGL_NONE};
    EGLSurface s = eglCreatePbufferSurface(d,cfg,pbuf);
    EGLint ctxattrs[] = {EGL_CONTEXT_CLIENT_VERSION,2,EGL_NONE};
    EGLContext c = eglCreateContext(d,cfg,EGL_NO_CONTEXT,ctxattrs);
    if (!eglMakeCurrent(d,s,s,c)) { printf("make current failed %x\n",eglGetError()); return 4; }
    printf("GL vendor=%s renderer=%s version=%s\n",glGetString(GL_VENDOR),glGetString(GL_RENDERER),glGetString(GL_VERSION));
    const char* vs="attribute vec2 position; void main(){gl_Position=vec4(position,0.0,1.0);}";
    const char* fs="precision mediump float; void main(){gl_FragColor=vec4(0.0,1.0,0.0,1.0);}";
    GLuint v=glCreateShader(GL_VERTEX_SHADER), f=glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(v,1,&vs,nullptr); glCompileShader(v);
    glShaderSource(f,1,&fs,nullptr); glCompileShader(f);
    GLint vok=0,fok=0; glGetShaderiv(v,GL_COMPILE_STATUS,&vok); glGetShaderiv(f,GL_COMPILE_STATUS,&fok);
    GLuint program=glCreateProgram(); glAttachShader(program,v); glAttachShader(program,f);
    glBindAttribLocation(program,0,"position"); glLinkProgram(program);
    GLint linked=0; glGetProgramiv(program,GL_LINK_STATUS,&linked);
    printf("vertex_compile=%d fragment_compile=%d program_link=%d\n",vok,fok,linked);
    if (!vok || !fok || !linked) return 6;
    glUseProgram(program); glViewport(0,0,16,16);
    const GLfloat vertices[]={-0.8f,-0.8f,0.8f,-0.8f,0.0f,0.8f};
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,vertices); glEnableVertexAttribArray(0);
    glClearColor(1,0,0,1); glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES,0,3); glFinish();
    unsigned char p[4]={0}; glReadPixels(8,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,p);
    GLenum err=glGetError();
    printf("pixel=%u,%u,%u,%u glError=%x\n",p[0],p[1],p[2],p[3],err);
    unsigned char corner[4]={0}; glReadPixels(0,15,1,1,GL_RGBA,GL_UNSIGNED_BYTE,corner);
    printf("corner=%u,%u,%u,%u\n",corner[0],corner[1],corner[2],corner[3]);
    bool ok = p[0] == 0 && p[1] == 255 && p[2] == 0 && p[3] == 255 && err == GL_NO_ERROR
        && corner[0] == 255 && corner[1] == 0 && corner[2] == 0 && corner[3] == 255;
    glDeleteProgram(program); glDeleteShader(v); glDeleteShader(f);
    android::sp<android::GraphicBuffer> buffer=new android::GraphicBuffer(16,16,HAL_PIXEL_FORMAT_RGBA_8888,
        GRALLOC_USAGE_SW_WRITE_OFTEN|GRALLOC_USAGE_HW_TEXTURE);
    void* mapped=nullptr;
    if(buffer->initCheck()!=0 || buffer->lock(GRALLOC_USAGE_SW_WRITE_OFTEN,&mapped)!=0) return 7;
    for(int y=0;y<16;++y)for(int x=0;x<16;++x)static_cast<uint32_t*>(mapped)[y*buffer->getStride()+x]=0xff00ffff;
    buffer->unlock();
    auto createImage=reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(eglGetProcAddress("eglCreateImageKHR"));
    auto bindImage=reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(eglGetProcAddress("glEGLImageTargetTexture2DOES"));
    auto destroyImage=reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(eglGetProcAddress("eglDestroyImageKHR"));
    EGLint ia[]={EGL_IMAGE_PRESERVED_KHR,EGL_TRUE,EGL_NONE};
    EGLImageKHR image=createImage(d,EGL_NO_CONTEXT,EGL_NATIVE_BUFFER_ANDROID,buffer->getNativeBuffer(),ia);
    if(image==EGL_NO_IMAGE_KHR){printf("native image failed %x\n",eglGetError());return 8;}
    GLuint tex=0;glGenTextures(1,&tex);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_EXTERNAL_OES,tex);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    bindImage(GL_TEXTURE_EXTERNAL_OES,image);
    const char* ivs="attribute vec2 position; varying vec2 uv; void main(){gl_Position=vec4(position,0.0,1.0);uv=(position+1.0)*0.5;}";
    const char* ifs="#extension GL_OES_EGL_image_external : require\nprecision mediump float; uniform samplerExternalOES image; varying vec2 uv; void main(){gl_FragColor=texture2D(image,uv);}";
    v=glCreateShader(GL_VERTEX_SHADER);f=glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(v,1,&ivs,nullptr);glCompileShader(v);glShaderSource(f,1,&ifs,nullptr);glCompileShader(f);
    program=glCreateProgram();glAttachShader(program,v);glAttachShader(program,f);glBindAttribLocation(program,0,"position");glLinkProgram(program);
    glGetProgramiv(program,GL_LINK_STATUS,&linked);glUseProgram(program);glUniform1i(glGetUniformLocation(program,"image"),0);
    glClearColor(1,0,0,1);glClear(GL_COLOR_BUFFER_BIT);glDrawArrays(GL_TRIANGLES,0,3);glFinish();
    glReadPixels(8,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,p);err=glGetError();
    printf("native_texture program_link=%d pixel=%u,%u,%u,%u glError=%x\n",linked,p[0],p[1],p[2],p[3],err);
    ok=ok&&linked&&p[0]==255&&p[1]==255&&p[2]==0&&p[3]==255&&err==GL_NO_ERROR;
    glDeleteTextures(1,&tex);destroyImage(d,image);glDeleteProgram(program);glDeleteShader(v);glDeleteShader(f);
    eglMakeCurrent(d,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
    eglDestroyContext(d,c); eglDestroySurface(d,s); eglTerminate(d);
    return ok ? 0 : 5;
}
