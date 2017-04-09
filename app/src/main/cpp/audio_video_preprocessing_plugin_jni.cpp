#include <jni.h>
#include <android/log.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <algorithm>

#include "../include/agora/IAgoraMediaEngine.h"
#include "../include/agora/IAgoraRtcEngine.h"

#include "../include/libyuv.h"

#include "../include/VMUtil.h"
#include "../include/audio_video_preprocessing_plugin_jni.h"

using namespace libyuv;

static uid_t remoteUid = -1;

static const int maxPicSize = 1920 * 1080; // suppose it's 1920 * 1080

static agora::media::IVideoFrameObserver::VideoFrame remoteFrame;

static uint8_t* remoteY;
static uint8_t* remoteU;
static uint8_t* remoteV;

static uint8_t* rotateY;
static uint8_t* rotateU;
static uint8_t* rotateV;

static uint8_t* localRotatedY;
static uint8_t* localRotatedU;
static uint8_t* localRotatedV;

static bool hasRemoteVideo = false;

class AgoraAudioFrameObserver : public agora::media::IAudioFrameObserver
{
public:
    virtual bool onRecordAudioFrame(AudioFrame& audioFrame) override
    {
        return true;
    }

    virtual bool onPlaybackAudioFrame(AudioFrame& audioFrame) override
    {
        return true;
    }

    virtual bool onMixedAudioFrame(AudioFrame& audioFrame) override
    {
        VMUtil::instance().on_mixed_audio_data((int16*) audioFrame.buffer, audioFrame.samples * 2);
        return true;
    }

    virtual bool onPlaybackAudioFrameBeforeMixing(unsigned int uid, AudioFrame& audioFrame) override
    {
        return true;
    }
};

class AgoraVideoFrameObserver : public agora::media::IVideoFrameObserver
{
public:
    virtual bool onCaptureVideoFrame(VideoFrame& videoFrame) override
    {
        if (hasRemoteVideo) {
            // TODO picture composition
            // I420Rotate local
            // I420Rotate remote
            // I420Scale local and remote
        } else {
            int width = videoFrame.width;
            int height = videoFrame.height;

            I420Rotate((uint8_t*) videoFrame.yBuffer, videoFrame.yStride, (uint8_t*) videoFrame.uBuffer, videoFrame.uStride, (uint8_t*) videoFrame.vBuffer, videoFrame.vStride,
                                   localRotatedY, height, localRotatedU, height / 2, localRotatedV, height / 2,
                                   width, height, (RotationMode) videoFrame.rotation);

            VMUtil::instance().on_video_data(localRotatedY, localRotatedU, localRotatedV, width, height);
        }

        return true;
    }

    virtual bool onRenderVideoFrame(unsigned int uid, VideoFrame& videoFrame) override
    {
        if (remoteUid == -1 || remoteUid == uid) { // temporarily just use one remote stream;
            remoteFrame = videoFrame;
            remoteUid = uid;

            memcpy(remoteY, videoFrame.yBuffer, videoFrame.width * videoFrame.height);
            memcpy(remoteU, videoFrame.uBuffer, videoFrame.width * videoFrame.height / 4);
            memcpy(remoteV, videoFrame.vBuffer, videoFrame.width * videoFrame.height / 4);
        }

        hasRemoteVideo = true;

        return true;
    }
};

static AgoraAudioFrameObserver s_audioFrameObserver;
static AgoraVideoFrameObserver s_videoFrameObserver;
static agora::rtc::IRtcEngine* rtcEngine = NULL;

#ifdef __cplusplus
extern "C" {
#endif

int __attribute__((visibility("default"))) loadAgoraRtcEnginePlugin(agora::rtc::IRtcEngine* engine)
{
    rtcEngine = engine;
    return 0;
}

void __attribute__((visibility("default"))) unloadAgoraRtcEnginePlugin(agora::rtc::IRtcEngine* engine)
{
    rtcEngine = NULL;
}

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    JNIEnv *env = NULL;

    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        LOGE("ERROR: GetEnv failed\n");
        return result;
    }

    assert(env != NULL);

    VMUtil::instance().initialize(env);

    /* success -- return valid version number */
    result = JNI_VERSION_1_6;

    return result;
}

JNIEXPORT void JNICALL Java_io_agora_ex_AudioVideoPreProcessing_doRegisterPreProcessing
  (JNIEnv *env, jobject obj)
{
    if (!rtcEngine)
        return;

    agora::util::AutoPtr<agora::media::IMediaEngine> mediaEngine;
    mediaEngine.queryInterface(*rtcEngine, agora::rtc::AGORA_IID_MEDIA_ENGINE);
    if (mediaEngine) {
        mediaEngine->registerVideoFrameObserver(&s_videoFrameObserver);
        mediaEngine->registerAudioFrameObserver(&s_audioFrameObserver);
    }

    remoteY = (uint8_t*) malloc(maxPicSize * sizeof(uint8_t));
    remoteU = (uint8_t*) malloc(maxPicSize / 4 * sizeof(uint8_t));
    remoteV = (uint8_t*) malloc(maxPicSize / 4 * sizeof(uint8_t));

    rotateY = (uint8_t*) malloc(maxPicSize * sizeof(uint8_t));
    rotateU = (uint8_t*) malloc(maxPicSize / 4 * sizeof(uint8_t));
    rotateV = (uint8_t*) malloc(maxPicSize / 4 * sizeof(uint8_t));

    localRotatedY = (uint8_t*) malloc(maxPicSize * sizeof(uint8_t));
    localRotatedU = (uint8_t*) malloc(maxPicSize / 4 * sizeof(uint8_t));
    localRotatedV = (uint8_t*) malloc(maxPicSize / 4 * sizeof(uint8_t));

    VMUtil::instance().addJNIHostObject(env, obj);
}

JNIEXPORT void JNICALL Java_io_agora_ex_AudioVideoPreProcessing_doDeregisterPreProcessing
  (JNIEnv *env, jobject obj)
{
    agora::util::AutoPtr<agora::media::IMediaEngine> mediaEngine;
    mediaEngine.queryInterface(*rtcEngine, agora::rtc::AGORA_IID_MEDIA_ENGINE);
    if (mediaEngine) {
        mediaEngine->registerVideoFrameObserver(NULL);
        mediaEngine->registerAudioFrameObserver(NULL);
    }

    free(remoteY);
    free(remoteU);
    free(remoteV);

    free(rotateY);
    free(rotateU);
    free(rotateV);

    free(localRotatedY);
    free(localRotatedU);
    free(localRotatedV);

    VMUtil::instance().removeJNIHostObject(env);
}

#ifdef __cplusplus
}
#endif
