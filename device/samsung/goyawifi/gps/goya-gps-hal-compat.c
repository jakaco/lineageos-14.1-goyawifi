/* Android 7 adapter for the stock SM-T110 Android 4.2 Marvell GPS HAL. */
#define LOG_TAG "GoyaGpsCompat"
#include <android/log.h>
#include <errno.h>
#include <dlfcn.h>
#include <hardware/gps.h>
#include <hardware/hardware.h>
#include <string.h>
#include <time.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/* Android 4.2 callback layout used by the proprietary gps.default module. */
typedef struct {
    size_t size;
    gps_location_callback location_cb;
    gps_status_callback status_cb;
    gps_sv_status_callback sv_status_cb;
    gps_nmea_callback nmea_cb;
    gps_set_capabilities set_capabilities_cb;
    gps_acquire_wakelock acquire_wakelock_cb;
    gps_release_wakelock release_wakelock_cb;
    gps_create_thread create_thread_cb;
    gps_request_utc_time request_utc_time_cb;
} LegacyGpsCallbacks;

typedef struct {
    hw_device_t common;
    const GpsInterface *(*get_gps_interface)(struct gps_device_t *device);
} CompatGpsDevice;

static void *vendor_module_handle;
static const hw_module_t *vendor_module;
static struct gps_device_t *vendor_device;
static const GpsInterface *vendor_gps;
static GpsInterface compat_gps;
static CompatGpsDevice compat_device;

/* The receiver firmware predates a GPS week rollover and reports dates 1024 weeks early. */
#define GPS_ROLLOVER_MS 619315200000LL
#define GPS_PLAUSIBLE_MS 1546300800000LL /* 2019-01-01 UTC */
/*
 * The firmware also reports GPS time, which runs ahead of UTC by the leap seconds since 1980
 * (18 s since 2017), without subtracting them. Correct that when the first fix of a session is
 * that far ahead of the system clock, so firmware that already applies them isn't shifted twice.
 */
#define GPS_UTC_LEAP_MS 18000LL
#define GPS_UTC_TOLERANCE_MS 2000LL
static gps_location_callback framework_location_cb;
static int rollover_logged;
static long long leap_correction_ms;

static void compat_location_cb(GpsLocation *location) {
    if (framework_location_cb == NULL || location == NULL) return;
    GpsLocation fixed = *location;
    int weeks = 0;
    while (fixed.timestamp > 0 && fixed.timestamp < GPS_PLAUSIBLE_MS && weeks < 4) {
        fixed.timestamp += GPS_ROLLOVER_MS;
        weeks++;
    }
    if (!rollover_logged) {
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        const long long wall = (long long)now.tv_sec * 1000LL + now.tv_nsec / 1000000LL;
        const long long offset = (long long)fixed.timestamp - wall;
        const long long leap_error = offset - GPS_UTC_LEAP_MS;
        leap_correction_ms = (leap_error > -GPS_UTC_TOLERANCE_MS && leap_error < GPS_UTC_TOLERANCE_MS)
                ? GPS_UTC_LEAP_MS : 0;
        LOGI("first fix: rollovers corrected %d, offset from system clock %lld ms, "
             "leap seconds subtracted %lld ms", weeks, offset, leap_correction_ms);
        rollover_logged = 1;
    }
    fixed.timestamp -= leap_correction_ms;
    framework_location_cb(&fixed);
}

static int compat_init(GpsCallbacks *callbacks) {
    if (callbacks == NULL || vendor_gps == NULL || vendor_gps->init == NULL) return -EINVAL;
    LegacyGpsCallbacks legacy;
    memset(&legacy, 0, sizeof(legacy));
    legacy.size = sizeof(legacy);
    framework_location_cb = callbacks->location_cb;
    rollover_logged = 0;
    legacy.location_cb = compat_location_cb;
    legacy.status_cb = callbacks->status_cb;
    legacy.sv_status_cb = callbacks->sv_status_cb;
    legacy.nmea_cb = callbacks->nmea_cb;
    legacy.set_capabilities_cb = callbacks->set_capabilities_cb;
    legacy.acquire_wakelock_cb = callbacks->acquire_wakelock_cb;
    legacy.release_wakelock_cb = callbacks->release_wakelock_cb;
    legacy.create_thread_cb = callbacks->create_thread_cb;
    legacy.request_utc_time_cb = callbacks->request_utc_time_cb;

    void *engine = dlopen("/system/lib/libagps_hal.so", RTLD_NOW | RTLD_GLOBAL);
    if (engine != NULL) {
        void (*download_timer)(unsigned int) =
            (void (*)(unsigned int))dlsym(engine, "dl_host_download_fresh_timer");
        if (download_timer != NULL) {
            download_timer(300); /* 30 seconds; receiver firmware takes about 11 seconds. */
            LOGI("Marvell firmware download watchdog set to 30 seconds");
        } else {
            LOGE("Marvell firmware watchdog API not found");
        }
    } else {
        LOGE("Could not open Marvell engine: %s", dlerror());
    }
    const int rc = vendor_gps->init((GpsCallbacks *)&legacy);
    LOGI("legacy GPS init returned %d (callback size %u)", rc, (unsigned)legacy.size);
    return rc;
}

static int compat_start(void) { return vendor_gps && vendor_gps->start ? vendor_gps->start() : -ENODEV; }
static int compat_stop(void) { return vendor_gps && vendor_gps->stop ? vendor_gps->stop() : -ENODEV; }
static void compat_cleanup(void) { if (vendor_gps && vendor_gps->cleanup) vendor_gps->cleanup(); }
static int compat_inject_time(GpsUtcTime t, int64_t ref, int uncertainty) {
    return vendor_gps && vendor_gps->inject_time ? vendor_gps->inject_time(t, ref, uncertainty) : -ENODEV;
}
static int compat_inject_location(double lat, double lon, float accuracy) {
    return vendor_gps && vendor_gps->inject_location ? vendor_gps->inject_location(lat, lon, accuracy) : -ENODEV;
}
static void compat_delete_aiding_data(GpsAidingData flags) {
    if (vendor_gps && vendor_gps->delete_aiding_data) vendor_gps->delete_aiding_data(flags);
}
static int compat_set_position_mode(GpsPositionMode mode, GpsPositionRecurrence recurrence,
                                    uint32_t interval, uint32_t accuracy, uint32_t time) {
    return vendor_gps && vendor_gps->set_position_mode
        ? vendor_gps->set_position_mode(mode, recurrence, interval, accuracy, time) : -ENODEV;
}
static const void *compat_get_extension(const char *name) {
    return vendor_gps && vendor_gps->get_extension ? vendor_gps->get_extension(name) : NULL;
}

static const GpsInterface *compat_get_gps_interface(struct gps_device_t *unused) {
    (void)unused;
    return vendor_gps ? &compat_gps : NULL;
}

static int compat_close(hw_device_t *device) {
    (void)device;
    if (vendor_device != NULL) {
        vendor_device->common.close(&vendor_device->common);
        vendor_device = NULL;
        vendor_gps = NULL;
    }
    if (vendor_module_handle != NULL) {
        dlclose(vendor_module_handle);
        vendor_module_handle = NULL;
        vendor_module = NULL;
    }
    return 0;
}

static int compat_open(const hw_module_t *module, const char *id, hw_device_t **device) {
    if (module == NULL || id == NULL || device == NULL || strcmp(id, GPS_HARDWARE_MODULE_ID) != 0) return -EINVAL;
    if (vendor_device == NULL) {
        vendor_module_handle = dlopen("/system/lib/hw/gps.vendor.so", RTLD_NOW | RTLD_GLOBAL);
        if (vendor_module_handle == NULL) {
            LOGE("Could not load preserved stock GPS HAL: %s", dlerror());
            return -ENOENT;
        }
        vendor_module = (const hw_module_t *)dlsym(vendor_module_handle, "HMI");
        if (vendor_module == NULL || vendor_module->methods == NULL || vendor_module->methods->open == NULL) {
            LOGE("Preserved stock GPS HAL has no HMI/open method");
            return -EINVAL;
        }
        int rc = vendor_module->methods->open(vendor_module, GPS_HARDWARE_MODULE_ID,
                                               (hw_device_t **)&vendor_device);
        if (rc != 0 || vendor_device == NULL) {
            LOGE("Preserved stock GPS HAL open failed: %d", rc);
            return rc != 0 ? rc : -ENODEV;
        }
        vendor_gps = vendor_device->get_gps_interface(vendor_device);
        if (vendor_gps == NULL || vendor_gps->size < sizeof(GpsInterface)) {
            LOGE("Preserved stock GPS interface is missing or too old");
            return -ENODEV;
        }
        memset(&compat_gps, 0, sizeof(compat_gps));
        compat_gps.size = sizeof(compat_gps);
        compat_gps.init = compat_init;
        compat_gps.start = compat_start;
        compat_gps.stop = compat_stop;
        compat_gps.cleanup = compat_cleanup;
        compat_gps.inject_time = compat_inject_time;
        compat_gps.inject_location = compat_inject_location;
        compat_gps.delete_aiding_data = compat_delete_aiding_data;
        compat_gps.set_position_mode = compat_set_position_mode;
        compat_gps.get_extension = compat_get_extension;
        LOGI("Loaded stock Marvell GPS HAL interface size %u", (unsigned)vendor_gps->size);
    }

    memset(&compat_device, 0, sizeof(compat_device));
    compat_device.common.tag = HARDWARE_DEVICE_TAG;
    compat_device.common.version = 0;
    compat_device.common.module = (hw_module_t *)module;
    compat_device.common.close = compat_close;
    compat_device.get_gps_interface = compat_get_gps_interface;
    *device = &compat_device.common;
    return 0;
}

static hw_module_methods_t gps_methods = { .open = compat_open };

hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .module_api_version = HARDWARE_MODULE_API_VERSION(0, 1),
    .hal_api_version = HARDWARE_HAL_API_VERSION,
    .id = GPS_HARDWARE_MODULE_ID,
    .name = "SM-T110 GPS HAL ABI adapter",
    .author = "Codex downstream bring-up",
    .methods = &gps_methods,
};
