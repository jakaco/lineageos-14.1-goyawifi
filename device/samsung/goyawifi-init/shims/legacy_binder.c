/* SPDX-License-Identifier: Apache-2.0
 * ARM32 compatibility entry point for the stock SM-T110 gralloc module.
 * Verified against LineageOS frameworks/native cm-14.1 a2efd4f088d7e9fb870b56a08b0f831e0b05321a.
 * getCallingPid changed from non-const to const; no object layout is accessed.
 */
extern int nougat_get_calling_pid(const void *state)
    __asm__("_ZNK7android14IPCThreadState13getCallingPidEv");

int legacy_get_calling_pid(const void *state)
    __asm__("_ZN7android14IPCThreadState13getCallingPidEv");

int legacy_get_calling_pid(const void *state)
{
    return nougat_get_calling_pid(state);
}
