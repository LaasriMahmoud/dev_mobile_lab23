#include <jni.h>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <android/log.h>
#include <sys/ptrace.h>
#include <unistd.h>

#define LOG_TAG "ANTI_DEBUG"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// --------------------------------------------------
// Contrôle 1 : tentative de détection de traçage
// --------------------------------------------------
static bool isBeingTraced() {
    long result = ptrace(PTRACE_TRACEME, 0, 0, 0);
    if (result == -1) {
        LOGE("Etat suspect : trace/debug detecte");
        return true;
    }
    LOGI("Aucun trace/debug detecte via ptrace");
    return false;
}

// --------------------------------------------------
// Contrôle 2 : recherche de signatures dans /proc/self/maps
// --------------------------------------------------
static bool containsSuspiciousLibraryNames() {
    FILE* maps = fopen("/proc/self/maps", "r");
    if (!maps) {
        LOGW("Impossible d'ouvrir /proc/self/maps");
        return false;
    }

    char line[512];

    while (fgets(line, sizeof(line), maps)) {
        if (strstr(line, "frida") ||
            strstr(line, "xposed") ||
            strstr(line, "libfrida") ||
            strstr(line, "gdbserver") ||
            strstr(line, "libgdb") ||
            strstr(line, "magisk")) {
            LOGE("Signature suspecte trouvee dans maps : %s", line);
            fclose(maps);
            return true;
        }
    }

    fclose(maps);
    LOGI("Aucune signature suspecte trouvee dans /proc/self/maps");
    return false;
}

// --------------------------------------------------
// Contrôle global appelé depuis Java
// --------------------------------------------------
extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_jnidemo_MainActivity_isDebugDetected(
        JNIEnv* env,
        jobject /* this */) {

    bool traced = isBeingTraced();
    bool suspiciousMaps = containsSuspiciousLibraryNames();

    if (traced || suspiciousMaps) {
        LOGE("Etat de securite : DEBUG / INSTRUMENTATION detecte");
        return JNI_TRUE;
    }

    LOGI("Etat de securite : OK");
    return JNI_FALSE;
}

// --------------------------------------------------
// Fonctions JNI du laboratoire précédent
// --------------------------------------------------
extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_jnidemo_MainActivity_helloFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    return env->NewStringUTF("Hello from C++ via JNI !");
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_jnidemo_MainActivity_factorial(
        JNIEnv* env,
        jobject /* this */,
        jint n) {

    if (n < 0) {
        return -1;
    }

    long long fact = 1;
    for (int i = 1; i <= n; i++) {
        fact *= i;
    }

    return static_cast<jint>(fact);
}
