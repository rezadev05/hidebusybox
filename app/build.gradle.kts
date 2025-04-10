plugins {
    id("com.android.application")
}

android {
    namespace = "io.github.rezadev05.hidebusybox"
    compileSdk = 34

    defaultConfig {
        applicationId = "io.github.rezadev05.hidebusybox"
        minSdk = 29
        targetSdk = 34
        versionCode = 1
        versionName = "25.04.1"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
        externalNativeBuild {
            cmake {
                cppFlags += ""
            }
        }

        ndk {
            abiFilters.add("armeabi-v7a")
            abiFilters.add("x86_64")
            abiFilters.add("arm64-v8a")
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
            signingConfig = signingConfigs["debug"]
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_1_8
        targetCompatibility = JavaVersion.VERSION_1_8
    }
    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
}

dependencies {
    compileOnly(files("libs/api-82.jar"))
}