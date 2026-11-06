#define LOG_TAG "GoyaCodaOMX"
#include <media/hardware/OMXPluginBase.h>
#include <utils/Log.h>
#include <dlfcn.h>
#include <string.h>

namespace android {
namespace {
constexpr char kDecoder[] = "OMX.MARVELL.VIDEO.HW.CODA7542DECODER";
constexpr char kEncoder[] = "OMX.MARVELL.VIDEO.HW.CODA7542ENCODER";
const char* const kComponents[] = {kDecoder, kEncoder};

class CodaPlugin final : public OMXPluginBase {
public:
    bool initialize() {
        library = dlopen("libMrvlOmx.so", RTLD_NOW | RTLD_LOCAL);
        if (!library) {
            ALOGE("Cannot load the stock CODA OMX core: %s", dlerror());
            return false;
        }
        auto init = reinterpret_cast<decltype(&OMX_Init)>(dlsym(library, "OMX_Init"));
        deinit = reinterpret_cast<decltype(&OMX_Deinit)>(dlsym(library, "OMX_Deinit"));
        getHandle = reinterpret_cast<decltype(&OMX_GetHandle)>(dlsym(library, "OMX_GetHandle"));
        freeHandle = reinterpret_cast<decltype(&OMX_FreeHandle)>(dlsym(library, "OMX_FreeHandle"));
        if (!init || !deinit || !getHandle || !freeHandle) return false;
        initialized = init() == OMX_ErrorNone;
        return initialized;
    }

    ~CodaPlugin() override {
        if (initialized) deinit();
        if (library) dlclose(library);
    }

    OMX_ERRORTYPE makeComponentInstance(const char* name, const OMX_CALLBACKTYPE* callbacks,
                                         OMX_PTR appData, OMX_COMPONENTTYPE** component) override {
        if (!roleFor(name)) return OMX_ErrorInvalidComponentName;
        OMX_HANDLETYPE handle = nullptr;
        auto result = getHandle(&handle, const_cast<char*>(name), appData,
                                const_cast<OMX_CALLBACKTYPE*>(callbacks));
        if (result == OMX_ErrorNone) {
            *component = static_cast<OMX_COMPONENTTYPE*>(handle);
            ALOGI("Created %s", name);
        }
        return result;
    }

    OMX_ERRORTYPE destroyComponentInstance(OMX_COMPONENTTYPE* component) override {
        return freeHandle(component);
    }

    OMX_ERRORTYPE enumerateComponents(OMX_STRING name, size_t size, OMX_U32 index) override {
        if (index >= sizeof(kComponents) / sizeof(kComponents[0])) return OMX_ErrorNoMore;
        const char* component = kComponents[index];
        if (size <= strlen(component)) return OMX_ErrorBadParameter;
        strcpy(name, component);
        return OMX_ErrorNone;
    }

    OMX_ERRORTYPE getRolesOfComponent(const char* name, Vector<String8>* roles) override {
        const char* role = roleFor(name);
        if (!role) return OMX_ErrorInvalidComponentName;
        roles->clear();
        roles->push(String8(role));
        return OMX_ErrorNone;
    }

private:
    static const char* roleFor(const char* name) {
        if (!strcmp(name, kDecoder)) return "video_decoder.avc";
        if (!strcmp(name, kEncoder)) return "video_encoder.avc";
        return nullptr;
    }

    void* library = nullptr;
    bool initialized = false;
    decltype(&OMX_Deinit) deinit = nullptr;
    decltype(&OMX_GetHandle) getHandle = nullptr;
    decltype(&OMX_FreeHandle) freeHandle = nullptr;
};
} // namespace
} // namespace android

extern "C" android::OMXPluginBase* createOMXPlugin() {
    auto* plugin = new android::CodaPlugin();
    if (plugin->initialize()) return plugin;
    delete plugin;
    return nullptr;
}
