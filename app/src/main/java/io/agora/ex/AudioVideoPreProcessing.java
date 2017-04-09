package io.agora.ex;

import io.agora.av.capturing.streaming.StreamingClient;

public class AudioVideoPreProcessing {
    static {
        System.loadLibrary("apm-plugin-audio-video-preprocessing");
    }

    public native void doRegisterPreProcessing();

    public native void doDeregisterPreProcessing();

    public final void registerPreProcessing() {
        if (mStreamingClient == null) {
            throw new IllegalStateException("should call setStreamingClient first");
        }
        mStreamingClient.startStreaming();
        doRegisterPreProcessing();
    }

    public final void deregisterPreProcessing() {
        doDeregisterPreProcessing();
        mStreamingClient.stopStreaming();
    }

    @SuppressWarnings("native call")
    private void VM_onMixedAudioData(final byte[] data) {
        mStreamingClient.sendPCMData(data);
    }

    @SuppressWarnings("native call")
    private void VM_onVideoData(final byte[] data, int width, int height) {
        mStreamingClient.sendYUVData(data, width, height);
    }

    public void setStreamingClient(StreamingClient client) {
        mStreamingClient = client;
    }

    private StreamingClient mStreamingClient;
}
