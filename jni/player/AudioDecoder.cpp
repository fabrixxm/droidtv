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
	mSamples = (int16_t *) av_malloc(mSamplesSize);
	if (mSamples == NULL) {
		// TODO error log needed?
		return false;
	}
	return true;
}

bool AudioDecoder::process(AVPacket *packet) {
	int size = mSamplesSize;
	avcodec_decode_audio3(mStream->codec, mSamples, &size, packet);
	mListener->decodeAudioFrame(mSamples, size);
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
	// allocated by av_malloc in ::prepare()
	av_free(mSamples);
	return true;
}
