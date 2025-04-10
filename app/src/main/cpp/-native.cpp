#include <dlfcn.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <android/log.h>
#include <jni.h>
#include <stdarg.h>

#include "dobby.h"
#include "package_list.h"

// ANDROID SYSCALL UNTUK ARM 64
#ifndef __NR_access
#define __NR_access 21
#endif

#ifndef __NR_faccessat
#define __NR_faccessat 79
#endif

#ifndef __NR_stat
#define __NR_stat 106
#endif

#ifndef __NR_lstat
#define __NR_lstat 107
#endif

#ifndef __NR_fstatat
#define __NR_fstatat 79
#endif

#ifndef __NR_readlink
#define __NR_readlink 89
#endif

#ifndef __NR_openat
#define __NR_openat 56
#endif

#define LOG_TAG "HIDE_BUSYBOX_NATIVE"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// POINTER KE ORIGINAL SYSCALL
static long (*original_syscall)(long number, ...) = nullptr;

// CEK PATH MENCURIGAKAN
bool should_block(const char* pathname) {
    if (!pathname) return false;

    // ALLOW LIST: UNTUK PATH PENTING
    if (strstr(pathname, "/data/") ||
        strstr(pathname, "/proc/") ||
        strstr(pathname, "/system/") ||
        strstr(pathname, "/vendor/") ||
        strstr(pathname, "/dev/") ||
        strstr(pathname, "/lib/") ||
        strstr(pathname, "/mnt/")) {
        return false;
    }

    // BLOCK UNTUK PATH ABNORMAL
    if (strstr(pathname, "magisk") ||
        strstr(pathname, "/su") ||
        strstr(pathname, "/xposed") ||
        strstr(pathname, "busybox")) {
        return true;
    }

    return false;
}

// HOOK SYSCALL
extern "C"
long my_syscall(long number, ...) {

    if (!original_syscall) {
        LOGI("original_syscall is null!");
        return -1;
    }

    va_list args;
    va_start(args, number);

    long result;

    switch (number) {
        case __NR_faccessat: {
            int dirfd = va_arg(args, int);
            const char* pathname = va_arg(args, const char*);
            int mode = va_arg(args, int);

            LOGI("faccessat: %s", pathname ? pathname : "null");

            if (pathname && should_block(pathname)) {
                LOGI("Blocked faccessat to: %s", pathname);
                errno = ENOENT;
                va_end(args);
                return -1;
            }

            va_end(args);
            va_start(args, number);
            result = original_syscall(number, args);
            va_end(args);
            return result;
        }

        case __NR_openat: {
            int dirfd = va_arg(args, int);
            const char* pathname = va_arg(args, const char*);
            int flags = va_arg(args, int);
            mode_t mode = va_arg(args, int);

            LOGI("openat: %s", pathname ? pathname : "null");

            if (pathname && should_block(pathname)) {
                LOGI("Blocked openat to: %s", pathname);
                errno = ENOENT;
                va_end(args);
                return -1;
            }

            va_end(args);
            va_start(args, number);
            result = original_syscall(number, args);
            va_end(args);
            return result;
        }

        case __NR_access: {
            const char* pathname = va_arg(args, const char*);
            int mode = va_arg(args, int);

            LOGI("access: %s", pathname ? pathname : "null");

            if (pathname && should_block(pathname)) {
                LOGI("Blocked access to: %s", pathname);
                errno = ENOENT;
                va_end(args);
                return -1;
            }

            va_end(args);
            return original_syscall(number, pathname, mode);
        }


        case __NR_stat:
        case __NR_lstat: {
            const char* pathname = va_arg(args, const char*);
            void* statbuf = va_arg(args, void*);

            LOGI("stat/lstat: %s", pathname ? pathname : "null");

            if (pathname && should_block(pathname)) {
                LOGI("Blocked stat/lstat to: %s", pathname);
                errno = ENOENT;
                va_end(args);
                return -1;
            }

            va_end(args);
            va_start(args, number);
            result = original_syscall(number, args);
            va_end(args);
            return result;
        }

        case __NR_fstatat: {
            int dirfd = va_arg(args, int);
            const char* pathname = va_arg(args, const char*);
            void* statbuf = va_arg(args, void*);
            int flags = va_arg(args, int);

            LOGI("fstatat: %s", pathname ? pathname : "null");

            if (pathname && should_block(pathname)) {
                LOGI("Blocked fstatat to: %s", pathname);
                errno = ENOENT;
                va_end(args);
                return -1;
            }

            va_end(args);
            va_start(args, number);
            result = original_syscall(number, args);
            va_end(args);
            return result;
        }

        case __NR_readlink: {
            const char* pathname = va_arg(args, const char*);
            char* buf = va_arg(args, char*);
            size_t bufsiz = va_arg(args, size_t);

            LOGI("readlink: %s", pathname ? pathname : "null");

            if (pathname && should_block(pathname)) {
                LOGI("Blocked readlink to: %s", pathname);
                errno = ENOENT;
                va_end(args);
                return -1;
            }

            va_end(args);
            va_start(args, number);
            result = original_syscall(number, args);
            va_end(args);
            return result;
        }

        default:
            result = original_syscall(number, args);
            va_end(args);
            return result;
    }
}

// HOOK INISIALISASI
void init_hook() {
    void* target = dlsym(RTLD_DEFAULT, "syscall");
    if (target) {
        DobbyHook(target, (void*)my_syscall, (void**)&original_syscall);
        LOGI("Successfully hooked syscall!");
    } else {
        LOGI("Failed to find syscall symbol");
    }
}

bool is_in_list(const char* pkg, const char* list[]) {
    for (int i = 0; list[i] != nullptr; i++) {
        if (strcmp(pkg, list[i]) == 0) {
            return true;
        }
    }
    return false;
}

extern "C"
JNIEXPORT void JNICALL
Java_io_github_rezadev05_hidebusybox_NativeLoader_nativeLoaderInit(JNIEnv* env, jclass clazz, jstring pkgNameStr) {
    const char* pkgName = env->GetStringUTFChars(pkgNameStr, 0);
    LOGI("Package name (from Java): %s", pkgName);

    if (is_in_list(pkgName, WHITELISTED_PACKAGES)) {
        LOGI("WHITELISTED package: skip native hook");
    } else if (is_in_list(pkgName, BLACKLISTED_PACKAGES)) {
        LOGI("BLACKLISTED package: applying hook");
        init_hook();
    } else {
        LOGI("Package not listed: default no hook");
    }

    env->ReleaseStringUTFChars(pkgNameStr, pkgName);
}

