#include <stdlib.h>
#include <unistd.h>

#include "MediaPlayer.h"
#include "utils.h"

#define MAX_QUEUE_SIZE (15 * 1024 * 1024) // taken from avplay.c

MediaPlayerListener::~MediaPlayerListener() {
}

class MediaPlayerAudioListener: public AudioDecoderListener {
public:
	MediaPlayerAudioListener(MediaPlayer* mp);
	void decodeAudioFrame(jbyteArray buffer, int size);
private:
	MediaPlayer* mPlayer;
};

MediaPlayerAudioListener::MediaPlayerAudioListener(MediaPlayer* mp) {
	mPlayer = mp;
}

void MediaPlayerAudioListener::decodeAudioFrame(jbyteArray buffer, int size) {
	mPlayer->decodeAudio(buffer, size);
}

class MediaPlayerVideoListener: public VideoDecoderListener {
public:
	MediaPlayerVideoListener(MediaPlayer* mp);
	void decodeVideoFrame(AVFrame* frame, double pts);
private:
	MediaPlayer* mPlayer;
};

MediaPlayerVideoListener::MediaPlayerVideoListener(MediaPlayer* mp) {
	mPlayer = mp;
}

void MediaPlayerVideoListener::decodeVideoFrame(AVFrame* frame, double pts) {
	mPlayer->decodeVideo(frame, pts);
}

MediaPlayer::MediaPlayer() {
	mState = STATE_UNINITIALIZED;
	mListener = NULL;
	mFrame = NULL;
	mAudioStreamIndex = 0;
	mVideoStreamIndex = 0;
	mInputFile = NULL;
}

MediaPlayer::~MediaPlayer() {
	if (mListener != NULL) {
		delete mListener;
	}
	avformat_free_context(mInputFile);
}

int MediaPlayer::prepare(JNIEnv *env, const char* fileName) {

	mState = STATE_PREPARING;
	int ret, i;

	// open file
	if ((ret = avformat_open_input(&mInputFile, fileName, NULL, NULL)) != 0) {
		LOGE("MediaPlayer::prepare", "avformat_open_input=%d", ret);
		return ERROR_OPEN;
	}

	// get stream information
	if ((ret = avformat_find_stream_info(mInputFile, NULL)) < 0) {
		LOGE("MediaPlayer::prepare", "avformat_find_stream_info=%d", ret);
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
		LOGE("MediaPlayer::prepare", "avcodec_open2=%d", ret);
		return ERROR_AUDIO_CODEC;
	}

	if (!mListener->postPrepareAudio(stream->codec->sample_rate)) {
		return ERROR_INVALID_TRACK;
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
		LOGE("MediaPlayer::prepare", "avcodec_open2=%d", ret);
		return ERROR_VIDEO_CODEC;
	}

	mVideoWidth = codec_ctx->width;
	mVideoHeight = codec_ctx->height;
	//duration =  mInputFile->duration;
	LOGD("MediaPlayer::prepare", "input:%dx%d@%d", mVideoWidth, mVideoHeight, mInputFile->duration);

	jobject bitmap = mListener->postPrepareVideo(mVideoWidth, mVideoHeight);

	if (bitmap == NULL) {
		LOGE("MediaPlayer::prepare", "Bitmap is NULL");
		return ERROR_INVALID_BITMAP;
	}

	AndroidBitmapInfo bitmapInfo;
	if ((ret = AndroidBitmap_getInfo(env, bitmap, &bitmapInfo))) {
		LOGE("MediaPlayer::prepare", "AndroidBitmap_getInfo(..) failed: 0x%x", ret);
		return ERROR_INVALID_BITMAP;
	}

	if (bitmapInfo.format != ANDROID_BITMAP_FORMAT_RGB_565) {
		LOGE("MediaPlayer::prepare", "Bitmap format is not RGB_565!");
		return ERROR_INVALID_BITMAP;
	}

	mState = STATE_PREPARED;
	return OK;
}

void MediaPlayer::setListener(MediaPlayerListener* listener) {
	mListener = listener;
}

int MediaPlayer::start() {
	if (mState != STATE_PREPARED) {
		return ERROR_INVALID_OPERATION;
	}
	mState = STATE_STARTING;
	pthread_create(&mMainThread, NULL, runMainThread, this);
	return OK;
}

int MediaPlayer::stop() {
	if (mState != STATE_PLAYING) {
		return ERROR_INVALID_OPERATION;
	}
	mState = STATE_STOPPING;
	pthread_join(mMainThread, NULL);
	return OK;
}

int MediaPlayer::getState() {
	return mState;
}

void* MediaPlayer::runMainThread(void* ptr) {
	LOGI("MediaPlayer", "starting main thread");
	MediaPlayer* player = (MediaPlayer*) ptr;
	player->decodeStream();
	LOGI("MediaPlayer", "ending main thread");
	return NULL;
}

void MediaPlayer::decodeStream() {
	AVPacket packet;
	int ret;

	mState = STATE_PLAYING;

	AVStream* audioStream = mInputFile->streams[mAudioStreamIndex];
	MediaPlayerAudioListener* audioListener = new MediaPlayerAudioListener(
			this);
	mAudioDecoder = new AudioDecoder(audioStream, audioListener);
	mAudioDecoder->start();

	AVStream* videoStream = mInputFile->streams[mVideoStreamIndex];
	MediaPlayerVideoListener* videoListener = new MediaPlayerVideoListener(
			this);
	mVideoDecoder = new VideoDecoder(videoStream, videoListener);
	mVideoDecoder->start();

	while (mState == STATE_PLAYING) {
		if (mVideoDecoder->packets() > MAX_QUEUE_SIZE
				&& mAudioDecoder->packets() > MAX_QUEUE_SIZE) {
			LOGW("MediaPlayer::decodeStream", "packets() > MAX_QUEUE_SIZE");
			usleep(200);
			continue;
		}

		if ((ret = av_read_frame(mInputFile, &packet)) < 0) {
			if (ret == AVERROR_EOF) {
				LOGI("MediaPlayer::decodeStream", "av_read_frame=EOF");
				mState = STATE_FINISHED;
			} else {
				LOGW("MediaPlayer::decodeStream", "av_read_frame=%d", ret);
				usleep(200);
				continue;
			}
		}

		// determine packet type
		if (packet.stream_index == mVideoStreamIndex) {
			if ((ret = mVideoDecoder->enqueue(&packet)) != 0) {
				LOGE("MediaPlayer::decodeStream", "VideoDecoder::err=", ret);
				mState = STATE_ERROR;
			}
		} else if (packet.stream_index == mAudioStreamIndex) {
			if ((ret = mAudioDecoder->enqueue(&packet)) != 0) {
				LOGE("MediaPlayer::decodeStream", "AudioDecoder::err=", ret);
				mState = STATE_ERROR;
			}
		} else {
			// TODO handle subtitle and EPG
			av_free_packet(&packet);
		}
	}

	// stop worker threads
	mVideoDecoder->stop();
	delete mVideoDecoder;
	mAudioDecoder->stop();
	delete mAudioDecoder;

	if (mState == STATE_ERROR) {
		LOGW("MediaPlayer::decodeStream", "MediaPlayer ended with error");
	}
	mState = STATE_FINISHED;
	LOGI("MediaPlayer", "end of playing");
}

void MediaPlayer::decodeVideo(AVFrame* frame, double pts) {
	mFrame = frame;
	mListener->postVideo();
}

void MediaPlayer::drawFrame(JNIEnv* env, jobject bitmap) {
	uint16_t* pixels;
	AndroidBitmap_lockPixels(env, bitmap, (void**)&pixels);

	// convert to RGB565
	yuv420_2_rgb565(pixels, mFrame->data[0], mFrame->data[1], mFrame->data[2], mVideoWidth, mVideoHeight, mFrame->linesize[0], mFrame->linesize[1], mVideoWidth, yuv2rgb565_table, 0);

	AndroidBitmap_unlockPixels(env, bitmap);
}

void MediaPlayer::decodeAudio(jbyteArray buffer, int size) {
	mListener->postAudio(buffer, size);
}
