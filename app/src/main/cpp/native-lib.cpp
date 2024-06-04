#include <jni.h>
#include <string>
#include <android/log.h>
#include <unwindstack/Unwinder.h>
#include <unwindstack/RegsArm64.h>
#include <unwindstack/UserArm64.h>

void printNativeStack(sigcontext *sigcontext) {
    std::string str;
    auto process_memory = unwindstack::Memory::CreateProcessMemory(getpid());
    std::unique_ptr<unwindstack::Maps> maps(new unwindstack::LocalMaps());
    auto regs =
            unwindstack::RegsArm64::Read((unwindstack::arm64_user_regs *) sigcontext->regs);
    std::unique_ptr<unwindstack::Regs> unwind_regs(regs);

    if (!maps->Parse()) {
        __android_log_print(ANDROID_LOG_ERROR, "Cvm", "Failed to parse maps");
        return;
    }


    unwindstack::Unwinder unwinder(512, maps.get(), unwind_regs.get(), process_memory);
    unwinder.Unwind();

    __android_log_print(ANDROID_LOG_INFO, "Cvm", "NumFrames %d ", unwinder.NumFrames());

    for (size_t i = 0; i < unwinder.NumFrames(); i++) {
        str += unwinder.FormatFrame(i) + "\n";
    }

    __android_log_print(ANDROID_LOG_INFO, "Cvm", "FormatFrame \n%s ", str.c_str());
}

void signalHandler(int signum, siginfo_t *info, void *context) {
    ucontext_t *uc = (ucontext_t *) context;
    sigcontext *sc = &uc->uc_mcontext;

    __android_log_print(ANDROID_LOG_INFO, "Cvm", "Signal %d received", signum);

    printNativeStack(sc);
}

void setupSignalHandler() {
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = signalHandler;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGSYS, &sa, NULL) == -1) {
        __android_log_print(ANDROID_LOG_ERROR, "Cvm", "Failed to set up SIGSYS handler");
    }

}

extern "C" JNIEXPORT jstring

JNICALL
Java_com_Cvm_unwindstack_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    setupSignalHandler();
    raise(SIGSYS);
    return env->NewStringUTF(hello.c_str());
}