#ifndef DROIDTV_VIDEO_DECODER_H
#define DROIDTV_VIDEO_DECODER_H

#include "Decoder.h"

class VideoDecoderListener {
public:
	virtual ~VideoDecoderListener();
	virtual void decodeVideoFrame(AVFrame* frame, double pts) = 0;
};

class VideoDecoder : public Decoder {
public:
	VideoDecoder(AVStream* stream, VideoDecoderListener* listener);
	~VideoDecoder();

private:
	static int getBuffer(struct AVCodecContext *c, AVFrame *pic);
	static void releaseBuffer(struct AVCodecContext *c, AVFrame *pic);

	bool prepare();
	void decode();
	bool process(AVPacket *packet);
	double synchronize(AVFrame *src_frame, double pts);

	AVFrame* mFrame;
	double mVideoClock;
	VideoDecoderListener* mListener;
};

#endif // DROIDTV_VIDEO_DECODER_H
