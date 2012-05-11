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
	mjSamples = env->NewShortArray(mSamplesSize);
	if (mjSamples == NULL) {
		// TODO error log needed?
		return false;
	}
	return true;
}

bool AudioDecoder::process(AVPacket *packet) {
	JNIEnv* env = getJNIEnv();
	int size = mSamplesSize;
	jshort* samples = env->GetShortArrayElements(mjSamples, NULL);
	avcodec_decode_audio3(mStream->codec, samples, &size, packet);
	env->ReleaseShortArrayElements(mjSamples, samples, 0);
	mListener->decodeAudioFrame(mjSamples, size);
	return true;
}

bool AudioDecoder::decode() {
	AVPacket packet;
	LOGI("decoding audio");
	while (mRunning) {
		dequeue(&packet);
		if (!process(&packet)) {
			mRunning = false;
			return false;
		}
		// allocated by av_read_frame
		av_free_packet(&packet);
	}
	LOGI("decoding audio ended");
	return true;
}
