#include <malloc.h>
#include <android/log.h>
#include <jni.h>
#include <string.h>
#include "rtmp.h"

#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "rtmp_jni", __VA_ARGS__))

#define RTMP_JNI_OK 0
#define RTMP_JNI_ERR -1
#define RTMP_JNI_ERR_ALLOC -2
#define RTMP_JNI_ERR_SETUP_URL -3
#define RTMP_JNI_ERR_CONNECT -4
#define RTMP_JNI_ERR_CONNECT_STREAM -5

static void set_ctx(JNIEnv *env, jobject thiz, void *ctx) {
    jclass cls = (*env)->GetObjectClass(env, thiz);
    jfieldID fid = (*env)->GetFieldID(env, cls, "ctx", "J");
    (*env)->SetLongField(env, thiz, fid, (jlong) (uintptr_t) ctx);
}

static void *get_ctx(JNIEnv *env, jobject thiz) {
    jclass cls = (*env)->GetObjectClass(env, thiz);
    jfieldID fid = (*env)->GetFieldID(env, cls, "ctx", "J");
    return (void *) (uintptr_t) (*env)->GetLongField(env, thiz, fid);
}

JNIEXPORT jint open(JNIEnv *env, jobject thiz, jstring url, jboolean enableWrite) {
    if (get_ctx(env, thiz) != NULL) return RTMP_JNI_ERR;
    RTMP *rtmp = RTMP_Alloc();
    set_ctx(env, thiz, rtmp);
    if (rtmp == NULL) return RTMP_JNI_ERR_ALLOC;

    RTMP_Init(rtmp);

    const char *c_url = (*env)->GetStringUTFChars(env, url, 0);
    int ret = RTMP_SetupURL(rtmp, c_url);
    if (!ret) return RTMP_JNI_ERR_SETUP_URL;

    if (enableWrite) RTMP_EnableWrite(rtmp);
    if (!RTMP_Connect(rtmp, NULL)) return RTMP_JNI_ERR_CONNECT;
    if (!RTMP_ConnectStream(rtmp, 0)) return RTMP_JNI_ERR_CONNECT_STREAM;

    (*env)->ReleaseStringUTFChars(env, url, c_url);
    return RTMP_JNI_OK;
}

JNIEXPORT jint read(JNIEnv *env, jobject thiz, jbyteArray data, jint offset, jint size) {
    RTMP *rtmp = get_ctx(env, thiz);
    if (rtmp == NULL) return RTMP_JNI_ERR_ALLOC;

    char *buf = malloc(size * sizeof(char));
    int nRead = RTMP_Read(rtmp, buf, size);

    if (nRead > 0) (*env)->SetByteArrayRegion(env, data, offset, nRead, buf);
    free(buf);

    return nRead;
}

JNIEXPORT jint write(JNIEnv *env, jobject thiz, jbyteArray data, jint size, jint type, jint ts) {
    RTMP *rtmp = get_ctx(env, thiz);
    if (rtmp == NULL) return RTMP_JNI_ERR_ALLOC;

    RTMPPacket *packet = (RTMPPacket *) malloc(sizeof(RTMPPacket));
    RTMPPacket_Alloc(packet, size);
    RTMPPacket_Reset(packet);

    jbyte *body = (*env)->GetByteArrayElements(env, data, NULL);
    memcpy(packet->m_body, body, size);

    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet->m_hasAbsTimestamp = FALSE;
    packet->m_nTimeStamp = ts;
    packet->m_packetType = type;
    packet->m_nBodySize = size;
    packet->m_nInfoField2 = rtmp->m_stream_id;

    switch (type) {
        case RTMP_PACKET_TYPE_INFO:
            packet->m_nChannel = 0x03;
            break;
        case RTMP_PACKET_TYPE_VIDEO:
            packet->m_nChannel = 0x04;
            break;
        case RTMP_PACKET_TYPE_AUDIO:
            packet->m_nChannel = 0x05;
            break;
        default:
            packet->m_nChannel = -1;
    }

    int ret = RTMP_SendPacket(rtmp, packet, 0);
    RTMPPacket_Free(packet);
    free(packet);

    (*env)->ReleaseByteArrayElements(env, data, body, 0);
    return ret ? RTMP_JNI_OK : RTMP_JNI_ERR;
}

JNIEXPORT void close(JNIEnv *env, jobject thiz) {
    RTMP *rtmp = get_ctx(env, thiz);
    if (rtmp == NULL) return;
    RTMP_Close(rtmp);
    RTMP_Free(rtmp);
}

JNIEXPORT jstring getVersion(JNIEnv *env, jobject thiz) {
    return (*env)->NewStringUTF(env, "1.0.3");
}

static JNINativeMethod methods[] = {
        {"open",       "(Ljava/lang/String;Z)I", (void *) open},
        {"read",       "([BII)I",                (void *) read},
        {"write",      "([BIII)I",               (void *) write},
        {"close",      "()V",                    (void *) close},
        {"getVersion", "()Ljava/lang/String;",   (void *) getVersion},
};

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if ((*vm)->GetEnv(vm, (void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        LOGE("GetEnv error");
        return JNI_ERR;
    }

    jclass clz = (*env)->FindClass(env, "me/baka/rtmp/RtmpClient");
    if (clz == NULL) {
        LOGE("FindClass error");
        return JNI_ERR;
    }

    if ((*env)->RegisterNatives(env, clz, methods, sizeof(methods) / sizeof(methods[0]))) {
        LOGE("RegisterNatives error");
        return JNI_ERR;
    }

    return JNI_VERSION_1_6;
}