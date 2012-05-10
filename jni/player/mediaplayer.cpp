#include "stdlib.h"
#include "mediaplayer.h"
#include "utils.h"

MediaPlayerListener::~MediaPlayerListener() {
}

MediaPlayer::MediaPlayer() {
	mState = STATE_UNINITIALIZED;
	mListener = NULL;
	mBitmap = NULL;
	mFrame = NULL;
	mCtxConvert = NULL;
	mAudioStreamIndex = 0;
	mVideoStreamIndex = 0;
	mInputFile = NULL;
}

MediaPlayer::~MediaPlayer() {
	if (mListener != NULL) {
		delete mListener;
	}
}

int MediaPlayer::prepare(JNIEnv *env, const char* fileName) {

	mState = STATE_PREPARING;
	int ret, i;

	// open file
	if ((ret = avformat_open_input(&mInputFile, fileName, NULL, NULL)) != 0) {
		LOGE("avformat_open_input=%d", ret);
		return ERROR_OPEN;
	}

	// get stream information
	if ((ret = avformat_find_stream_info(mInputFile, NULL)) < 0) {
		LOGE("avformat_find_stream_info=%d", ret);
		return ERROR_PARSE;
	}

	AVStream* stream;
	AVCodecContext* codec_ctx;
	AVCodec* codec;

	// prepare audio
	mAudioStreamIndex = -1;
	for (i = 0; i < mInputFile->nb_streams; i++) {
		if (mInputFile->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			mAudioStreamIndex = i;
			break;
		}
	}

	if (mAudioStreamIndex == -1) {
		return ERROR_AUDIO_STREAM;
	}

	stream = mInputFile->streams[mAudioStreamIndex];
	codec_ctx = stream->codec;
	codec = avcodec_find_decoder(codec_ctx->codec_id);
	if (codec == NULL) {
		return ERROR_AUDIO_DECODER;
	}

	if ((ret = avcodec_open2(codec_ctx, codec, NULL)) < 0) {
		LOGE("avcodec_open2=%d", ret);
		return ERROR_AUDIO_CODEC;
	}

	// prepare video
	mVideoStreamIndex = -1;
	for (i = 0; i < mInputFile->nb_streams; i++) {
		if (mInputFile->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			mVideoStreamIndex = i;
			break;
		}
	}

	if (mVideoStreamIndex == -1) {
		return ERROR_VIDEO_STREAM;
	}

	stream = mInputFile->streams[mVideoStreamIndex];
	codec_ctx = stream->codec;
	codec = avcodec_find_decoder(codec_ctx->codec_id);
	if (codec == NULL) {
		return ERROR_VIDEO_DECODER;
	}

	if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
		LOGE("avcodec_open2=%d", ret);
		return ERROR_VIDEO_CODEC;
	}

	int videoWidth = codec_ctx->width;
	int videoHeight = codec_ctx->height;
	//sDuration =  mInputFile->duration;
	LOGD("input:%dx%d@%d", videoWidth, videoHeight, mInputFile->duration);

	mCtxConvert = sws_getContext(stream->codec->width, stream->codec->height,
			stream->codec->pix_fmt, stream->codec->width, stream->codec->height,
			PIX_FMT_RGB565, SWS_POINT, NULL, NULL, NULL);

	if (mCtxConvert == NULL) {
		return ERROR_SWS_CONTEXT;
	}

	mFrame = avcodec_alloc_frame();
	if (mFrame == NULL) {
		return ERROR_ALLOC_FRAME;
	}

	mBitmap = mListener->postPrepare(videoWidth, videoHeight);

	if ((ret = AndroidBitmap_getInfo(env, mBitmap, &mBitmapInfo))) {
		LOGE("AndroidBitmap_getInfo(..) failed: 0x%x", ret);
		return ERROR_INVALID_BITMAP;
	}

	if (mBitmapInfo.format != ANDROID_BITMAP_FORMAT_RGB_565) {
		LOGE("Bitmap format is not RGB_565!");
		return ERROR_INVALID_BITMAP;
	}

	mState = STATE_PREPARED;
	return OK;
}

void MediaPlayer::setListener(MediaPlayerListener* listener) {
	mListener = listener;
}

int MediaPlayer::start() {
	// TODO implement
	return ERROR_UNKNOWN;
}

int MediaPlayer::stop() {
	// TODO implement
	return ERROR_UNKNOWN;
}

int MediaPlayer::getState() {
	return mState;
}
