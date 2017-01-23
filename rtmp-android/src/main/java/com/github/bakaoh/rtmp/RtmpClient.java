package com.github.bakaoh.rtmp;

public class RtmpClient {

    public static final int PACKET_TYPE_AUDIO = 0x08;
    public static final int PACKET_TYPE_VIDEO = 0x09;
    public static final int PACKET_TYPE_INFO = 0x12;

    public native int open(String url, boolean enableWrite);

    public native int read(byte[] buffer, int offset, int size);

    public native int write(byte[] packet, int size, int type, int ts);

    public native void close();

    public native String getVersion();

    private long ctx;

    static {
        System.loadLibrary("rtmp");
    }
}
