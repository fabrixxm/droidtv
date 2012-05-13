#include "AudioDecoder.h"
#include "utils.h"

AudioDecoderListener::~AudioDecoderListener() {
}

AudioDecoder::AudioDecoder(AVStream* stream, AudioDecoderListener* listener) :
		Decoder(stream) {
	mListener = listener;
}

AudioDecoder::~AudioDecoder() {
	delete mListener;
}

bool AudioDecoder::prepare() {
	mSamplesSize = AVCODEC_MAX_AUDIO_FRAME_SIZE;
	JNIEnv* env = getJNIEnv();
	if (!(mjSamples = env->NewByteArray(mSamplesSize))) {
		LOGE(": env->NewByteArray(%d) returns NULL", mSamplesSize);
		return false;
	}
	if (!(mFrame = avcodec_alloc_frame())) {
		LOGE(": avcodec_alloc_frame returns NULL");
		return false;
	}
	return true;
}

bool AudioDecoder::process(AVPacket *packet) {
	int decoded, ret;
	avcodec_get_frame_defaults(mFrame);
	ret = avcodec_decode_audio4(pStream->codec, mFrame, &decoded, packet);
	if (ret < 0) {
		if (ret == AVERROR_INVALIDDATA) {
			LOGW("avcodec_decode_audio4=AVERROR_INVALIDDATA, abort.");
			return false;
		}
		LOGE("avcodec_decode_audio4=%d", ret);
		return true; // continue anyways.. it's not a fatal error
	}
	if (decoded != 0) {
		int size = av_samples_get_buffer_size(NULL, pStream->codec->channels,
				mFrame->nb_samples, pStream->codec->sample_fmt, 1);
		JNIEnv* env = getJNIEnv();
		env->SetByteArrayRegion(mjSamples, 0, size, (jbyte*) mFrame->data[0]);
		mListener->decodeAudioFrame(mjSamples, size);
	}
	return true;
}

void AudioDecoder::decode() {
	AVPacket packet;
	bool ok = true;

	LOGI("decoding audio");
	while (ok && !pAbort) {
		dequeue(&packet);
		ok = process(&packet);
		// allocated by av_read_frame
		av_free_packet(&packet);
	}
	LOGI("decoding audio ended");
	// allocated by av_malloc in ::prepare()
	av_free(mFrame);
}
