package io.github.rezadev05.hidebusybox;

import android.app.Application;
import android.content.Context;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.List;

import de.robv.android.xposed.IXposedHookLoadPackage;
import de.robv.android.xposed.XC_MethodHook;
import de.robv.android.xposed.XSharedPreferences;
import de.robv.android.xposed.XposedBridge;
import de.robv.android.xposed.XposedHelpers;
import de.robv.android.xposed.callbacks.XC_LoadPackage;

public class HideBusybox implements IXposedHookLoadPackage {

    private static final String TAG = "HIDE_BUSYBOX_JAVA";

    private static final String PREF_NAME = "HIDE_BUSYBOX";

    private void isModuleEnabledForApp(String packageName) {
        XSharedPreferences pref = new XSharedPreferences(HideBusybox.class.getPackage().getName(), PREF_NAME);
        pref.reload();
        boolean enabled = pref.getBoolean("enable_for_" + packageName, false);
        XposedBridge.log(TAG + ": [" + packageName + "] isModuleEnabled = " + enabled);
    }

    @Override
    public void handleLoadPackage(XC_LoadPackage.LoadPackageParam lpparam) throws Throwable {
        XposedBridge.log(TAG + ": Hooking " + lpparam.packageName);

        isModuleEnabledForApp(lpparam.packageName);

        hookRuntimeExec(lpparam);
        hookFileExists(lpparam);
        hookFileInputStream(lpparam);
        hookProcessBuilder(lpparam);
        hookApplicationAttach(lpparam);
    }

    private void hookRuntimeExec(XC_LoadPackage.LoadPackageParam lpparam) {
        XposedHelpers.findAndHookMethod(Runtime.class, "exec", String[].class, new XC_MethodHook() {
            @Override
            protected void beforeHookedMethod(MethodHookParam param) {
                String[] commands = (String[]) param.args[0];
                for (String cmd : commands) {
                    if (cmd.contains("busybox")) {
                        XposedBridge.log(TAG + ": Blocked BusyBox exec: " + cmd);
                        param.setThrowable(new RuntimeException("Access Denied"));
                        break;
                    }
                }
            }
        });
    }

    // HOOK FILE BUSYBOX
    private void hookFileExists(XC_LoadPackage.LoadPackageParam lpparam) {
        XposedHelpers.findAndHookMethod(File.class, "exists", new XC_MethodHook() {
            @Override
            protected void afterHookedMethod(MethodHookParam param) {
                File file = (File) param.thisObject;
                String path = file.getAbsolutePath();
                if (path.contains("busybox") || path.contains("/su") || path.contains("/magisk")) {
                    XposedBridge.log(TAG + ": File.exists() → Hiding path: " + path);
                    param.setResult(false);
                }
            }
        });
    }

    private void hookFileInputStream(XC_LoadPackage.LoadPackageParam lpparam) {
        XposedHelpers.findAndHookConstructor(FileInputStream.class, String.class, new XC_MethodHook() {
            @Override
            protected void beforeHookedMethod(MethodHookParam param) {
                String path = (String) param.args[0];
                if (path.contains("busybox")) {
                    XposedBridge.log(TAG + ": Blocked FileInputStream: " + path);
                    param.setThrowable(new FileNotFoundException("File not found"));
                }
            }
        });
    }

    private void hookProcessBuilder(XC_LoadPackage.LoadPackageParam lpparam) {
        XposedHelpers.findAndHookConstructor(ProcessBuilder.class, List.class, new XC_MethodHook() {
            @Override
            protected void beforeHookedMethod(MethodHookParam param) {
                List<String> commands = (List<String>) param.args[0];
                for (String cmd : commands) {
                    if (cmd.contains("busybox")) {
                        XposedBridge.log(TAG + ": Blocked ProcessBuilder: " + cmd);
                        param.setThrowable(new IOException("Access Denied"));
                        break;
                    }
                }
            }
        });
    }

    private void hookApplicationAttach(XC_LoadPackage.LoadPackageParam lpparam) {
        XposedHelpers.findAndHookMethod(Application.class, "attach", Context.class, new XC_MethodHook() {
            @Override
            protected void afterHookedMethod(MethodHookParam param) {
                Context context = (Context) param.args[0];
                try {
                    NativeLoader.nativeLoaderInit(context.getPackageName());
                    XposedBridge.log(TAG + ": Native lib success");
                } catch (Throwable t) {
                    XposedBridge.log(TAG + ": Native lib failed: " + t.getMessage());
                }
            }
        });
    }
}
