/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "goya-audio-v2"
//#define LOG_NDEBUG 0

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <tinyalsa/asoundlib.h>
#include <malloc.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>

#include <cutils/log.h>
#include <cutils/str_parms.h>

#include <hardware/hardware.h>
#include <system/audio.h>
#include <hardware/audio.h>

struct stub_audio_device {
    struct audio_hw_device device;
    pthread_mutex_t lock;
    struct stub_stream_out *out;
    struct stub_stream_in *in;
    bool mic_muted;
};

struct stub_stream_out {
    struct audio_stream_out stream;
    struct stub_audio_device *dev;
    audio_devices_t devices;
    const char *route;
    struct pcm *pcm;
    pthread_mutex_t lock;
    uint64_t frames;
    unsigned errors;
};

struct stub_stream_in {
    struct audio_stream_in stream;
    struct stub_audio_device *dev;
    struct pcm *pcm;
    pthread_mutex_t lock;
    audio_devices_t devices;
    unsigned channels;
    const char *route;
    uint64_t frames;
    unsigned errors;
    short *scratch;
    size_t scratch_bytes;
};

/* Preserve bits still owned by the other direction on the shared SAI. */
static unsigned route_mask(const char *prefix,const char *control) {
    if(!prefix) return 0;
    char path[256],line[1024],name[128];unsigned mask,all=0;
    snprintf(path,sizeof(path),"/system/etc/goya-audio/%s-enable.txt",prefix);
    FILE *f=fopen(path,"r");if(!f)return 0;
    while(fgets(line,sizeof(line),f))
        if(sscanf(line,"reg %127s %u",name,&mask)==2 && !strcmp(name,control))all|=mask;
    fclose(f);return all;
}
static int apply_route(const char *prefix,bool enable,const char *protect) {
    char path[256];snprintf(path,sizeof(path),"/system/etc/goya-audio/%s-%s.txt",prefix,enable?"enable":"disable");
    FILE *f=fopen(path,"r"); if(!f) return -ENOENT;
    struct mixer *m=mixer_open(0); if(!m) {fclose(f);return -ENODEV;}
    char line[1024],kind[32],name[128]; int rc=0;
    while(fgets(line,sizeof(line),f)) {
        if(sscanf(line,"%31s %127s",kind,name)!=2) continue;
        if(!strcmp(kind,"sleep")) {usleep(atoi(name)*1000);continue;}
        struct mixer_ctl *c=mixer_get_ctl_by_name(m,name);
        if(!c) {fprintf(stderr,"missing %s\n",name);rc=5;break;}
        if(!strcmp(kind,"reg")) {
            int mask,value; sscanf(line,"%*s %*s %d %d",&mask,&value);
            if(!enable) mask&=~route_mask(protect,name);
            if(!mask) continue;
            int old=mixer_ctl_get_value(c,0);
            if(old<0 || mixer_ctl_set_value(c,0,(old&~mask)|value)) {rc=6;break;}
            
        } else if(!strcmp(kind,"bulk")) {
            long vals[66]={0}; char *s=strchr(strchr(line,' ')+1,' '); int n=0;
            char *save; char *v=strtok_r(s," \r\n",&save); while(v && n<64){vals[++n]=strtol(v,0,0);v=strtok_r(0," \r\n",&save);}
            vals[0]=n;
            if(mixer_ctl_set_array(c,vals,66)) {fprintf(stderr,"bulk failed\n");rc=7;break;}
        }
    }
    fclose(f);mixer_close(m);return rc;
}

static int codec_power(int on) {
    struct mixer *m=mixer_open(0); if(!m) return -ENODEV;
    struct mixer_ctl *c=mixer_get_ctl_by_name(m,"PM805_CODEC_MAIN_POWERUP");
    int v=c?mixer_ctl_get_value(c,0):-1;
    int rc=v<0?-EIO:mixer_ctl_set_value(c,0,(v&~1)|(on?1:0));
    mixer_close(m); if(on) usleep(5000); return rc;
}
static const char *out_route(audio_devices_t devices) {
    return devices&(AUDIO_DEVICE_OUT_WIRED_HEADSET|AUDIO_DEVICE_OUT_WIRED_HEADPHONE)?"headphone":"speaker";
}
static const char *in_route(audio_devices_t devices) {
    return devices&AUDIO_DEVICE_IN_WIRED_HEADSET&~AUDIO_DEVICE_BIT_IN?"headset-mic":"builtin-mic";
}
/* Caller holds its stream lock; the device lock serializes codec operations. */
static void stop_output(struct stub_stream_out *out) {
    struct stub_audio_device *d=out->dev;
    pthread_mutex_lock(&d->lock);
    if(out->pcm) {
        pcm_close(out->pcm);out->pcm=NULL;
        apply_route(out->route,false,d->in&&d->in->pcm?d->in->route:NULL);
        if(!d->in || !d->in->pcm)codec_power(0);
    }
    pthread_mutex_unlock(&d->lock);
}
static void stop_input(struct stub_stream_in *in) {
    struct stub_audio_device *d=in->dev;
    pthread_mutex_lock(&d->lock);
    if(in->pcm) {
        pcm_close(in->pcm);in->pcm=NULL;
        apply_route(in->route,false,d->out&&d->out->pcm?d->out->route:NULL);
        if(!d->out || !d->out->pcm)codec_power(0);
    }
    pthread_mutex_unlock(&d->lock);
}
static uint32_t out_get_sample_rate(const struct audio_stream *stream)
{
    return 44100;
}

static int out_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    ALOGV("out_set_sample_rate: %d", 0);
    return -ENOSYS;
}

static size_t out_get_buffer_size(const struct audio_stream *stream)
{
    return 1024*4;
}

static audio_channel_mask_t out_get_channels(const struct audio_stream *stream)
{
    ALOGV("out_get_channels");
    return AUDIO_CHANNEL_OUT_STEREO;
}

static audio_format_t out_get_format(const struct audio_stream *stream)
{
    ALOGV("out_get_format");
    return AUDIO_FORMAT_PCM_16_BIT;
}

static int out_set_format(struct audio_stream *stream, audio_format_t format)
{
    ALOGV("out_set_format: %d",format);
    return -ENOSYS;
}

static int out_standby(struct audio_stream *stream)
{
    struct stub_stream_out *out=(struct stub_stream_out *)stream;
    pthread_mutex_lock(&out->lock);stop_output(out);pthread_mutex_unlock(&out->lock);
    return 0;
}

static int out_dump(const struct audio_stream *stream, int fd)
{
    const struct stub_stream_out *out=(const struct stub_stream_out *)stream;
    dprintf(fd,"goya-audio-v2: output route=%s devices=%x pcm0p 44100Hz stereo; frames=%llu errors=%u active=%d\n",out_route(out->devices),out->devices,(unsigned long long)out->frames,out->errors,out->pcm!=NULL);return 0;
}

static int out_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    struct stub_stream_out *out=(struct stub_stream_out *)stream;
    struct str_parms *p=str_parms_create_str(kvpairs);int value;
    if(str_parms_get_int(p,AUDIO_PARAMETER_STREAM_ROUTING,&value)>=0 && value) {
        pthread_mutex_lock(&out->lock);
        if(out->devices!=(audio_devices_t)value) {stop_output(out);out->devices=value;}
        pthread_mutex_unlock(&out->lock);
    }
    str_parms_destroy(p);return 0;
}

static char * out_get_parameters(const struct audio_stream *stream, const char *keys)
{
    ALOGV("out_get_parameters");
    return strdup("");
}

static uint32_t out_get_latency(const struct audio_stream_out *stream)
{
    return 1024*4*1000/44100;
}

static int out_set_volume(struct audio_stream_out *stream, float left,
                          float right)
{
    return -ENOSYS; /* AudioFlinger applies media and mute volume. */
}

static ssize_t out_write(struct audio_stream_out *stream, const void* buffer,
                         size_t bytes)
{
    struct stub_stream_out *out=(struct stub_stream_out *)stream;
    struct stub_audio_device *d=out->dev;
    pthread_mutex_lock(&out->lock);int ret=0;
    if(!out->pcm) {
        struct pcm_config cfg={.channels=2,.rate=44100,.period_size=1024,.period_count=4,.format=PCM_FORMAT_S16_LE,.start_threshold=1024,.stop_threshold=4096,.avail_min=1024};
        pthread_mutex_lock(&d->lock);
        out->route=out_route(out->devices);
        ret=codec_power(1);
        if(!ret)ret=apply_route(out->route,true,NULL);
        if(!ret) {
            out->pcm=pcm_open(0,0,PCM_OUT,&cfg);
            if(!out->pcm || !pcm_is_ready(out->pcm)) {
                ALOGE("output open: %s",out->pcm?pcm_get_error(out->pcm):"null");ret=-EIO;
            }
        }
        if(ret) {
            if(out->pcm){pcm_close(out->pcm);out->pcm=NULL;}
            apply_route(out->route,false,d->in&&d->in->pcm?d->in->route:NULL);
            if(!d->in || !d->in->pcm)codec_power(0);
        }
        pthread_mutex_unlock(&d->lock);
    }
    if(!ret)ret=pcm_write(out->pcm,buffer,bytes);
    if(ret){ALOGE("output write failed %d",ret);out->errors++;stop_output(out);}
    else out->frames+=bytes/4;
    pthread_mutex_unlock(&out->lock);
    if(ret)usleep(bytes*1000000LL/4/44100);
    return ret?-EIO:(ssize_t)bytes;
}

static int out_get_render_position(const struct audio_stream_out *stream,
                                   uint32_t *dsp_frames)
{
    *dsp_frames = 0;
    ALOGV("out_get_render_position: dsp_frames: %p", dsp_frames);
    return -EINVAL;
}

static int out_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    ALOGV("out_add_audio_effect: %p", effect);
    return 0;
}

static int out_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    ALOGV("out_remove_audio_effect: %p", effect);
    return 0;
}

static int out_get_next_write_timestamp(const struct audio_stream_out *stream,
                                        int64_t *timestamp)
{
    *timestamp = 0;
    ALOGV("out_get_next_write_timestamp: %ld", (long int)(*timestamp));
    return -EINVAL;
}

/** audio_stream_in implementation **/
static uint32_t in_get_sample_rate(const struct audio_stream *stream)
{
    return 44100;
}

static int in_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    ALOGV("in_set_sample_rate: %d", rate);
    return -ENOSYS;
}

static size_t in_get_buffer_size(const struct audio_stream *stream)
{
    return 1024*((const struct stub_stream_in *)stream)->channels*2;
}

static audio_channel_mask_t in_get_channels(const struct audio_stream *stream)
{
    return ((const struct stub_stream_in *)stream)->channels==1?AUDIO_CHANNEL_IN_MONO:AUDIO_CHANNEL_IN_STEREO;
}

static audio_format_t in_get_format(const struct audio_stream *stream)
{
    return AUDIO_FORMAT_PCM_16_BIT;
}

static int in_set_format(struct audio_stream *stream, audio_format_t format)
{
    return -ENOSYS;
}

static int in_standby(struct audio_stream *stream)
{
    struct stub_stream_in *in=(struct stub_stream_in *)stream;
    pthread_mutex_lock(&in->lock);stop_input(in);pthread_mutex_unlock(&in->lock);return 0;
}

static int in_dump(const struct audio_stream *stream, int fd)
{
    const struct stub_stream_in *in=(const struct stub_stream_in *)stream;
    dprintf(fd,"goya-audio-v2: input route=%s devices=%x pcm0c 44100Hz channels=%u; frames=%llu errors=%u active=%d\n",in_route(in->devices),in->devices,in->channels,(unsigned long long)in->frames,in->errors,in->pcm!=NULL);return 0;
}

static int in_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    struct stub_stream_in *in=(struct stub_stream_in *)stream;
    struct str_parms *p=str_parms_create_str(kvpairs);int value;
    if(str_parms_get_int(p,AUDIO_PARAMETER_STREAM_ROUTING,&value)>=0 && value) {
        pthread_mutex_lock(&in->lock);
        if(in->devices!=(audio_devices_t)value){stop_input(in);in->devices=value;}
        pthread_mutex_unlock(&in->lock);
    }
    str_parms_destroy(p);return 0;
}

static char * in_get_parameters(const struct audio_stream *stream,
                                const char *keys)
{
    return strdup("");
}

static int in_set_gain(struct audio_stream_in *stream, float gain)
{
    return 0;
}

static ssize_t in_read(struct audio_stream_in *stream, void* buffer,
                       size_t bytes)
{
    struct stub_stream_in *in=(struct stub_stream_in *)stream;
    struct stub_audio_device *d=in->dev;
    if(bytes%(in->channels*2))return -EINVAL;
    pthread_mutex_lock(&in->lock);int ret=0;
    if(!in->pcm) {
        struct pcm_config cfg={.channels=2,.rate=44100,.period_size=1024,.period_count=4,.format=PCM_FORMAT_S16_LE,.start_threshold=1,.stop_threshold=4096,.avail_min=1024};
        pthread_mutex_lock(&d->lock);
        in->route=in_route(in->devices);ret=codec_power(1);
        if(!ret)ret=apply_route(in->route,true,NULL);
        if(!ret) {
            in->pcm=pcm_open(0,0,PCM_IN,&cfg);
            if(!in->pcm || !pcm_is_ready(in->pcm)) {
                ALOGE("capture open: %s",in->pcm?pcm_get_error(in->pcm):"null");ret=-EIO;
            }
        }
        if(ret) {
            if(in->pcm){pcm_close(in->pcm);in->pcm=NULL;}
            apply_route(in->route,false,d->out&&d->out->pcm?d->out->route:NULL);
            if(!d->out || !d->out->pcm)codec_power(0);
        }
        pthread_mutex_unlock(&d->lock);
    }
    size_t frames=bytes/(in->channels*2),hwbytes=frames*4;
    if(!ret && in->channels==1 && hwbytes>in->scratch_bytes) {
        void *b=realloc(in->scratch,hwbytes);
        if(!b)ret=-ENOMEM;else{in->scratch=b;in->scratch_bytes=hwbytes;}
    }
    if(!ret)ret=pcm_read(in->pcm,in->channels==1?in->scratch:buffer,hwbytes);
    if(ret){ALOGE("capture read failed %d",ret);in->errors++;stop_input(in);}
    else {
        if(in->channels==1)for(size_t i=0;i<frames;i++)((short *)buffer)[i]=in->scratch[i*2];
        pthread_mutex_lock(&d->lock);bool muted=d->mic_muted;pthread_mutex_unlock(&d->lock);
        if(muted)memset(buffer,0,bytes);
        in->frames+=frames;
    }
    pthread_mutex_unlock(&in->lock);
    if(ret)usleep(frames*1000000LL/44100);
    return ret?-EIO:(ssize_t)bytes;
}

static uint32_t in_get_input_frames_lost(struct audio_stream_in *stream)
{
    return 0;
}

static int in_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    return 0;
}

static int in_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    return 0;
}

static int adev_open_output_stream(struct audio_hw_device *dev,
                                   audio_io_handle_t handle,
                                   audio_devices_t devices,
                                   audio_output_flags_t flags,
                                   struct audio_config *config,
                                   struct audio_stream_out **stream_out,
                                   const char *address __unused)
{
    ALOGV("adev_open_output_stream...");

    struct stub_audio_device *ladev = (struct stub_audio_device *)dev;
    struct stub_stream_out *out;
    int ret;

    out = (struct stub_stream_out *)calloc(1, sizeof(struct stub_stream_out));
    if (!out)
        return -ENOMEM;

    out->stream.common.get_sample_rate = out_get_sample_rate;
    out->stream.common.set_sample_rate = out_set_sample_rate;
    out->stream.common.get_buffer_size = out_get_buffer_size;
    out->stream.common.get_channels = out_get_channels;
    out->stream.common.get_format = out_get_format;
    out->stream.common.set_format = out_set_format;
    out->stream.common.standby = out_standby;
    out->stream.common.dump = out_dump;
    out->stream.common.set_parameters = out_set_parameters;
    out->stream.common.get_parameters = out_get_parameters;
    out->stream.common.add_audio_effect = out_add_audio_effect;
    out->stream.common.remove_audio_effect = out_remove_audio_effect;
    out->stream.get_latency = out_get_latency;
    out->stream.set_volume = out_set_volume;
    out->stream.write = out_write;
    out->stream.get_render_position = out_get_render_position;
    out->stream.get_next_write_timestamp = out_get_next_write_timestamp;

    out->dev=ladev;out->devices=devices;
    pthread_mutex_init(&out->lock,NULL);
    pthread_mutex_lock(&ladev->lock);
    if(ladev->out){pthread_mutex_unlock(&ladev->lock);pthread_mutex_destroy(&out->lock);free(out);return -EBUSY;}
    ladev->out=out;pthread_mutex_unlock(&ladev->lock);
    config->sample_rate=44100;
    config->channel_mask=AUDIO_CHANNEL_OUT_STEREO;
    config->format=AUDIO_FORMAT_PCM_16_BIT;
    *stream_out = &out->stream;
    return 0;

err_open:
    free(out);
    *stream_out = NULL;
    return ret;
}

static void adev_close_output_stream(struct audio_hw_device *dev,
                                     struct audio_stream_out *stream)
{
    out_standby(&stream->common);
    pthread_mutex_destroy(&((struct stub_stream_out *)stream)->lock);
    struct stub_audio_device *d=((struct stub_stream_out *)stream)->dev;
    pthread_mutex_lock(&d->lock);d->out=NULL;pthread_mutex_unlock(&d->lock);
    free(stream);
}

static int adev_set_parameters(struct audio_hw_device *dev, const char *kvpairs)
{
    return 0;
}

static char * adev_get_parameters(const struct audio_hw_device *dev,
                                  const char *keys)
{
    ALOGV("adev_get_parameters");
    return strdup("");
}

static int adev_init_check(const struct audio_hw_device *dev)
{
    ALOGV("adev_init_check");
    return 0;
}

static int adev_set_voice_volume(struct audio_hw_device *dev, float volume)
{
    ALOGV("adev_set_voice_volume: %f", volume);
    return -ENOSYS;
}

static int adev_set_master_volume(struct audio_hw_device *dev, float volume)
{
    ALOGV("adev_set_master_volume: %f", volume);
    return -ENOSYS;
}

static int adev_get_master_volume(struct audio_hw_device *dev, float *volume)
{
    ALOGV("adev_get_master_volume: %f", *volume);
    return -ENOSYS;
}

static int adev_set_master_mute(struct audio_hw_device *dev, bool muted)
{
    ALOGV("adev_set_master_mute: %d", muted);
    return -ENOSYS;
}

static int adev_get_master_mute(struct audio_hw_device *dev, bool *muted)
{
    ALOGV("adev_get_master_mute: %d", *muted);
    return -ENOSYS;
}

static int adev_set_mode(struct audio_hw_device *dev, audio_mode_t mode)
{
    ALOGV("adev_set_mode: %d", mode);
    return 0;
}

static int adev_set_mic_mute(struct audio_hw_device *dev, bool state)
{
    struct stub_audio_device *d=(struct stub_audio_device *)dev;
    pthread_mutex_lock(&d->lock);d->mic_muted=state;pthread_mutex_unlock(&d->lock);return 0;
}

static int adev_get_mic_mute(const struct audio_hw_device *dev, bool *state)
{
    struct stub_audio_device *d=(struct stub_audio_device *)dev;
    pthread_mutex_lock(&d->lock);*state=d->mic_muted;pthread_mutex_unlock(&d->lock);return 0;
}

static size_t adev_get_input_buffer_size(const struct audio_hw_device *dev,
                                         const struct audio_config *config)
{
    return 1024*audio_channel_count_from_in_mask(config->channel_mask)*2;
}

static int adev_open_input_stream(struct audio_hw_device *dev,
                                  audio_io_handle_t handle,
                                  audio_devices_t devices,
                                  struct audio_config *config,
                                  struct audio_stream_in **stream_in,
                                  audio_input_flags_t flags __unused,
                                  const char *address __unused,
                                  audio_source_t source __unused)
{
    struct stub_audio_device *d=(struct stub_audio_device *)dev;
    *stream_in=NULL;
    if(config->sample_rate!=44100 || config->format!=AUDIO_FORMAT_PCM_16_BIT ||
       (config->channel_mask!=AUDIO_CHANNEL_IN_MONO && config->channel_mask!=AUDIO_CHANNEL_IN_STEREO)) {
        config->sample_rate=44100;config->format=AUDIO_FORMAT_PCM_16_BIT;config->channel_mask=AUDIO_CHANNEL_IN_MONO;return -EINVAL;
    }
    struct stub_stream_in *in=calloc(1,sizeof(*in));if(!in)return -ENOMEM;
    in->dev=d;in->devices=devices;in->channels=audio_channel_count_from_in_mask(config->channel_mask);
    pthread_mutex_init(&in->lock,NULL);
    in->stream.common.get_sample_rate=in_get_sample_rate;
    in->stream.common.set_sample_rate=in_set_sample_rate;
    in->stream.common.get_buffer_size=in_get_buffer_size;
    in->stream.common.get_channels=in_get_channels;
    in->stream.common.get_format=in_get_format;
    in->stream.common.set_format=in_set_format;
    in->stream.common.standby=in_standby;
    in->stream.common.dump=in_dump;
    in->stream.common.set_parameters=in_set_parameters;
    in->stream.common.get_parameters=in_get_parameters;
    in->stream.common.add_audio_effect=in_add_audio_effect;
    in->stream.common.remove_audio_effect=in_remove_audio_effect;
    in->stream.set_gain=in_set_gain;
    in->stream.read=in_read;
    in->stream.get_input_frames_lost=in_get_input_frames_lost;
    pthread_mutex_lock(&d->lock);
    if(d->in){pthread_mutex_unlock(&d->lock);pthread_mutex_destroy(&in->lock);free(in);return -EBUSY;}
    d->in=in;pthread_mutex_unlock(&d->lock);*stream_in=&in->stream;return 0;
}

static void adev_close_input_stream(struct audio_hw_device *dev,
                                   struct audio_stream_in *stream)
{
    struct stub_stream_in *in=(struct stub_stream_in *)stream;
    in_standby(&stream->common);
    pthread_mutex_lock(&in->dev->lock);in->dev->in=NULL;pthread_mutex_unlock(&in->dev->lock);
    pthread_mutex_destroy(&in->lock);free(in->scratch);free(in);
}

static int adev_dump(const audio_hw_device_t *device, int fd)
{
    dprintf(fd,"goya-audio-v2: PM805/PM822 speaker/wired output and microphone; shared SAI1 at 44100Hz\n");return 0;
}

static int adev_close(hw_device_t *device)
{
    ALOGV("adev_close");
    pthread_mutex_destroy(&((struct stub_audio_device *)device)->lock);
    free(device);
    return 0;
}

static int adev_open(const hw_module_t* module, const char* name,
                     hw_device_t** device)
{
    ALOGV("adev_open: %s", name);

    struct stub_audio_device *adev;
    int ret;

    if (strcmp(name, AUDIO_HARDWARE_INTERFACE) != 0)
        return -EINVAL;

    adev = calloc(1, sizeof(struct stub_audio_device));
    if (!adev)
        return -ENOMEM;

    pthread_mutex_init(&adev->lock,NULL);
    adev->device.common.tag = HARDWARE_DEVICE_TAG;
    adev->device.common.version = AUDIO_DEVICE_API_VERSION_2_0;
    adev->device.common.module = (struct hw_module_t *) module;
    adev->device.common.close = adev_close;

    adev->device.init_check = adev_init_check;
    adev->device.set_voice_volume = adev_set_voice_volume;
    adev->device.set_master_volume = adev_set_master_volume;
    adev->device.get_master_volume = adev_get_master_volume;
    adev->device.set_master_mute = adev_set_master_mute;
    adev->device.get_master_mute = adev_get_master_mute;
    adev->device.set_mode = adev_set_mode;
    adev->device.set_mic_mute = adev_set_mic_mute;
    adev->device.get_mic_mute = adev_get_mic_mute;
    adev->device.set_parameters = adev_set_parameters;
    adev->device.get_parameters = adev_get_parameters;
    adev->device.get_input_buffer_size = adev_get_input_buffer_size;
    adev->device.open_output_stream = adev_open_output_stream;
    adev->device.close_output_stream = adev_close_output_stream;
    adev->device.open_input_stream = adev_open_input_stream;
    adev->device.close_input_stream = adev_close_input_stream;
    adev->device.dump = adev_dump;

    *device = &adev->device.common;

    return 0;
}

static struct hw_module_methods_t hal_module_methods = {
    .open = adev_open,
};

struct audio_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = AUDIO_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = AUDIO_HARDWARE_MODULE_ID,
        .name = "Goya PM805/PM822 TinyALSA audio",
        .author = "The Android Open Source Project",
        .methods = &hal_module_methods,
    },
};
