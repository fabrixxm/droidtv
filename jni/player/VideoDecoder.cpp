#include <stdint.h>
#include "VideoDecoder.h"
#include "utils.h"

#ifndef INT64_C
#define INT64_C __INT64_C
#endif

VideoDecoderListener::~VideoDecoderListener() {
}

VideoDecoder::VideoDecoder(AVStream* stream, VideoDecoderListener* listener) :
		Decoder(stream) {
	mStream->codec->get_buffer = getBuffer;
	mStream->codec->release_buffer = releaseBuffer;
	mListener = listener;
}

VideoDecoder::~VideoDecoder() {
	delete mListener;
}

bool VideoDecoder::prepare() {
	mFrame = avcodec_alloc_frame();
	if (mFrame == NULL) {
		return false;
	}
	return true;
}

bool VideoDecoder::process(AVPacket *packet) {
	int ret;
	int completed;
	double pts = 0;

	if ((ret = avcodec_decode_video2(mStream->codec, mFrame, &completed, packet))
			< 0) {
		LOGE("avcodec_decode_video2=%d", ret);
		return false;
	}

	if (packet->dts == AV_NOPTS_VALUE && mFrame->opaque
			&& *(uint64_t*) mFrame->opaque != AV_NOPTS_VALUE) {
		pts = *(uint64_t *) mFrame->opaque;
	} else if (packet->dts != AV_NOPTS_VALUE) {
		pts = packet->dts;
	} else {
		pts = 0;
	}
	pts *= av_q2d(mStream->time_base);

	if (completed) {
		pts = synchronize(mFrame, pts);
		mListener->decodeVideoFrame(mFrame, pts);
		return true;
	}
	return false;
}

bool VideoDecoder::decode() {
	AVPacket packet;

	LOGI("decoding video");

	while (mRunning) {
		dequeue(&packet);
		if (!process(&packet)) {
			mRunning = false;
			return false;
		}
		// allocated by av_read_frame
		av_free_packet(&packet);
	}
	LOGI("decoding video ended");
	// allocated by av_malloc in ::prepare()
	av_free(mFrame);
	return true;
}

double VideoDecoder::synchronize(AVFrame *src_frame, double pts) {

	double frame_delay;

	if (pts != 0) {
		/* if we have pts, set video clock to it */
		mVideoClock = pts;
	} else {
		/* if we aren't given a pts, set it to the clock */
		pts = mVideoClock;
	}
	/* update the video clock */
	frame_delay = av_q2d(mStream->codec->time_base);
	/* if we are repeating a frame, adjust clock accordingly */
	frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
	mVideoClock += frame_delay;
	return pts;
}

int VideoDecoder::getBuffer(struct AVCodecContext* codec, AVFrame* frame) {
	int ret = avcodec_default_get_buffer(codec, frame);
	uint64_t* pts = (uint64_t *) av_malloc(sizeof(uint64_t));
	*pts = AV_NOPTS_VALUE;
	frame->opaque = pts;
	return ret;
}

void VideoDecoder::releaseBuffer(struct AVCodecContext* codec, AVFrame* frame) {
	if (frame != NULL) {
		av_freep(&frame->opaque);
	}
	avcodec_default_release_buffer(codec, frame);
}
