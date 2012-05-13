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
	pStream->codec->get_buffer = getBuffer;
	pStream->codec->release_buffer = releaseBuffer;
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
	ret = avcodec_decode_video2(pStream->codec, mFrame, &completed, packet);
	if (ret < 0) {
		if (ret == AVERROR_INVALIDDATA) {
			LOGD("VideoDecoder::process",
					"avcodec_decode_video2=AVERROR_INVALIDDATA, be strong!");
			return true;
		}
		LOGW("VideoDecoder::process", "avcodec_decode_video2=%d", ret);
		return true; // continue anyways.. it's not a fatal error
	}

	if (packet->dts == AV_NOPTS_VALUE && mFrame->opaque
			&& *(uint64_t*) mFrame->opaque != AV_NOPTS_VALUE) {
		pts = *(uint64_t *) mFrame->opaque;
	} else if (packet->dts != AV_NOPTS_VALUE) {
		pts = packet->dts;
	} else {
		pts = 0;
	}
	pts *= av_q2d(pStream->time_base);

	if (completed) {
		pts = synchronize(mFrame, pts);
		mListener->decodeVideoFrame(mFrame, pts);
		return true;
	}
	return true;
}

void VideoDecoder::decode() {
	AVPacket packet;
	bool ok = true;

	LOGI("VideoDecoder", "decoding video");
	while (ok && !pAbort) {
		dequeue(&packet);
		ok = process(&packet);
		// allocated by av_read_frame
		av_free_packet(&packet);
	}
	LOGI("VideoDecoder", "decoding video ended");
	// allocated by av_malloc in ::prepare()
	av_free(mFrame);
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
	frame_delay = av_q2d(pStream->codec->time_base);
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
