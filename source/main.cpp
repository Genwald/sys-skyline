#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <switch.h>

// Sysmodules should not use applet*.
u32 __nx_applet_type = AppletType_None;
u32 __nx_fs_num_sessions = 1;
u32 __nx_fsdev_direntry_cache_size = 1;

// Adjust size as needed.
#define INNER_HEAP_SIZE 0x80000
size_t nx_inner_heap_size = INNER_HEAP_SIZE;
char   nx_inner_heap[INNER_HEAP_SIZE];

void __libnx_initheap(void) {
    void*  addr = nx_inner_heap;
    size_t size = nx_inner_heap_size;

    // Newlib
    extern char* fake_heap_start;
    extern char* fake_heap_end;

    fake_heap_start = (char*)addr;
    fake_heap_end   = (char*)addr + size;
}

void __attribute__((weak)) __appInit(void) {
    Result rc;

    rc = smInitialize();
    if (R_FAILED(rc))
        fatalThrow(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));

    rc = fsInitialize();
    if (R_FAILED(rc))
        fatalThrow(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));

    fsdevMountSdmc();
}

void __attribute__((weak)) userAppExit(void);

void __attribute__((weak)) __appExit(void) {
    fsdevUnmountAll();
    fsExit();
    smExit();
}

void initServices() {
    Result rc;
    rc = pglInitialize();
    if (R_FAILED(rc)) {
        fatalThrow(rc);
    }
    rc = pminfoInitialize();
    if (R_FAILED(rc)) {
        fatalThrow(rc);
    }
}

void exitServices() {
    pglExit();
    pminfoExit();
}

Result AtmosphereSetExternalContentSource(u64 program_id, FsFileSystem* out) {
    return serviceDispatchInOut(ldrShellGetServiceSession(), 65000, program_id, out);
}

Result AtmosphereClearExternalContentSource(u64 program_id) {
    return serviceDispatchIn(ldrShellGetServiceSession(), 65001, program_id);
}

int main(int argc, char* argv[])
{
    initServices();
    PglEventObserver observer;
    Event event;
    PmProcessEventInfo eventInfo;
    u64 program_id;
    Result rc;
    Handle handle;

    rc = pglGetEventObserver(&observer);
    if (R_FAILED(rc)) {
        fatalThrow(rc);
    }
    rc = pglEventObserverGetProcessEvent(&observer, &event);
    if (R_FAILED(rc)) {
        fatalThrow(rc);
    }
    while (true) {
        eventWait(&event, UINT64_MAX);
        rc = pglEventObserverGetProcessEventInfo(&observer, &eventInfo);

        if (R_SUCCEEDED(rc) && (eventInfo.event == PmProcessEvent_Start || eventInfo.event == PmProcessEvent_DebugStart)) {
            rc = pminfoGetProgramId(&program_id, eventInfo.process_id);
            /*
            todo: make sure this is accurate application program id range,
            alternativly use svcGetInfo with InfoType_IsApplication
            There is an atmos extension to get a handle for this
            */
            if (R_SUCCEEDED(rc) && (program_id >= 0x0100000000010000 && program_id <= 0x0101000000000000)) {
                // pause game while we run?
                svcDebugActiveProcess(&handle, eventInfo.process_id);

                // read and patch NPDM
                // set npdm and subsdk
                //FsFileSystem exefs;
                //AtmosphereSetExternalContentSource(0x01006A800016E000, &exefs);
                //fsdevMountDevice("exefs",exefs);
                // restart game?


                svcSleepThread(10e+9);
                svcCloseHandle(handle);
                fatalThrow(0xdeadcafe);
            }

        }
    }
    pglEventObserverClose(&observer);
    exitServices();
    return 0;
}
