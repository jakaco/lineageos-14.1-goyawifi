/* Minimal Android lights HAL for the SM-T110 panel backlight.
 * The PXA/VX5B3D kernel backlight driver owns the hardware mapping and PWM.
 * Android sends its normal 0..255 light state here, including zero on display
 * suspend.  Other light IDs are not present on this board and are no-ops.
 */
#define LOG_TAG "GoyaLights"
#include <cutils/log.h>
#include <hardware/hardware.h>
#include <hardware/lights.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define GOYA_BACKLIGHT_PATH "/sys/class/backlight/panel/brightness"

struct goya_light_device {
    struct light_device_t device;
};

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

static int goya_set_light(struct light_device_t *device,
                          const struct light_state_t *state) {
    (void)device;
    if (state == NULL) return -EINVAL;

    /* LightsService encodes scalar backlight brightness in RGB equally. */
    const unsigned int red = (state->color >> 16) & 0xff;
    const unsigned int green = (state->color >> 8) & 0xff;
    const unsigned int blue = state->color & 0xff;
    const unsigned int brightness = (77 * red + 150 * green + 29 * blue) >> 8;

    char value[5];
    const int length = snprintf(value, sizeof(value), "%u\n", brightness);
    if (length <= 0 || (size_t)length >= sizeof(value)) return -EINVAL;

    pthread_mutex_lock(&g_lock);
    const int fd = open(GOYA_BACKLIGHT_PATH, O_WRONLY | O_CLOEXEC);
    if (fd < 0) {
        const int error = errno;
        ALOGE("open %s failed: %s", GOYA_BACKLIGHT_PATH, strerror(error));
        pthread_mutex_unlock(&g_lock);
        return -error;
    }
    const ssize_t written = write(fd, value, (size_t)length);
    const int error = written < 0 ? errno : (written == length ? 0 : EIO);
    close(fd);
    pthread_mutex_unlock(&g_lock);
    if (error != 0) {
        ALOGE("write brightness %u failed: %s", brightness, strerror(error));
        return -error;
    }
    ALOGV("panel backlight=%u", brightness);
    return 0;
}

static int goya_close(hw_device_t *device) {
    free(device);
    return 0;
}

static int goya_open(const hw_module_t *module, const char *name,
                     hw_device_t **device) {
    if (name == NULL || device == NULL) return -EINVAL;
    if (strcmp(name, LIGHT_ID_BACKLIGHT) != 0) return -EINVAL;

    struct goya_light_device *light = calloc(1, sizeof(*light));
    if (light == NULL) return -ENOMEM;
    light->device.common.tag = HARDWARE_DEVICE_TAG;
    light->device.common.version = LIGHTS_DEVICE_API_VERSION_1_0;
    light->device.common.module = (struct hw_module_t *)module;
    light->device.common.close = goya_close;
    light->device.set_light = goya_set_light;
    *device = &light->device.common;
    return 0;
}

static struct hw_module_methods_t goya_methods = {
    .open = goya_open,
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .module_api_version = HARDWARE_MODULE_API_VERSION(1, 0),
    .hal_api_version = HARDWARE_HAL_API_VERSION,
    .id = LIGHTS_HARDWARE_MODULE_ID,
    .name = "Goya panel backlight",
    .author = "LineageOS goyawifi",
    .methods = &goya_methods,
};
