package io.github.rezadev05.hidebusybox;

public class NativeLoader {

    static {
        System.loadLibrary("hidebusybox");
    }

    public static native void nativeLoaderInit(String packageName);
}
