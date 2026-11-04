// SPDX-License-Identifier: Apache-2.0
#include <binder/Binder.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/Parcel.h>
#include <utils/String16.h>
#include <stdio.h>
#include <string.h>
using namespace android;
static const char* kName = "goyawifi.bringup.probe";
static const int32_t kChallenge = 0x12345678;
class Probe : public BBinder {
    status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply,
                        uint32_t flags) override {
        if (code != FIRST_CALL_TRANSACTION) return BBinder::onTransact(code, data, reply, flags);
        int32_t value = data.readInt32();
        reply->writeInt32(value ^ 0x5a5a5a5a);
        return NO_ERROR;
    }
};
int main(int argc, char** argv) {
    sp<IServiceManager> sm = defaultServiceManager();
    if (argc == 2 && !strcmp(argv[1], "--server")) {
        status_t rc = sm->addService(String16(kName), new Probe());
        printf("register=%d pid=%d\n", rc, getpid()); fflush(stdout);
        if (rc != NO_ERROR) return 1;
        ProcessState::self()->startThreadPool();
        IPCThreadState::self()->joinThreadPool();
        return 0;
    }
    sp<IBinder> remote = sm->checkService(String16(kName));
    if (remote == nullptr) { fprintf(stderr,"service missing\n"); return 2; }
    Parcel data, reply;
    data.writeInt32(kChallenge);
    status_t rc = remote->transact(IBinder::FIRST_CALL_TRANSACTION, data, &reply);
    int32_t answer = reply.readInt32();
    printf("transaction=%d challenge=%08x reply=%08x expected=%08x\n",
           rc, kChallenge, answer, kChallenge ^ 0x5a5a5a5a);
    return rc == NO_ERROR && answer == (kChallenge ^ 0x5a5a5a5a) ? 0 : 3;
}
