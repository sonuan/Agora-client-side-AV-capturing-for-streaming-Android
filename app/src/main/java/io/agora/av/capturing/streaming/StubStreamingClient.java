package io.agora.av.capturing.streaming;

import com.qiniu.pili.droid.rtcstreaming.RTCStreamingManager;
import com.qiniu.pili.droid.streaming.av.common.PLFourCC;

import io.agora.av.capturing.streaming.ui.LiveRoomActivity;


/**
 * Implement your own streaming client
 */
public class StubStreamingClient extends StreamingClient {
    RTCStreamingManager mRTCStreamingManager;
    private final LiveRoomActivity mLiveRoomActivity;

    public StubStreamingClient(RTCStreamingManager rtcStreamingManager, LiveRoomActivity activity) {
        mRTCStreamingManager = rtcStreamingManager;
        mLiveRoomActivity = activity;
    }
    @Override
    public void startStreaming() {
        // do nothing
        mLiveRoomActivity.startPublishStreaming();
    }

    @Override
    public void stopStreaming() {
        // do nothing
        mLiveRoomActivity.stopPublishStreaming();
    }

    @Override
    public void sendYUVData(final byte[] yuv, final int width, final int height) {
        // do nothing
        if (mRTCStreamingManager.isStreamingStarted() && mLiveRoomActivity.isPublishStreamStarted()) {
            mRTCStreamingManager.inputVideoFrame(yuv, width, height, 90, false, PLFourCC.FOURCC_I420, System.nanoTime());
        }
    }

    @Override
    public void sendPCMData(final byte[] pcm) {
        // do nothing
        if (mRTCStreamingManager.isStreamingStarted() && mLiveRoomActivity.isPublishStreamStarted()) {
            mRTCStreamingManager.inputAudioFrame(pcm, System.nanoTime());
        }
    }
}
