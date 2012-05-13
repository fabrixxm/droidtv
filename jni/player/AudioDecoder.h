#ifndef DROIDTV_AUDIO_DECODER_H
#define DROIDTV_AUDIO_DECODER_H

#include "Decoder.h"
#include <jni.h>


class AudioDecoderListener {
public:
	virtual ~AudioDecoderListener();
	virtual void decodeAudioFrame(jbyteArray buffer, int size) = 0;
};

class AudioDecoder : public Decoder
{
public:
	AudioDecoder(AVStream* stream, AudioDecoderListener* listener);
	~AudioDecoder();

private:
	bool prepare();
	void decode();
	bool process(AVPacket* packet);

	AVFrame* mFrame;
	jbyteArray mjSamples;
	int mSamplesSize;
	AudioDecoderListener* mListener;
};

#endif // DROIDTV_AUDIO_DECODER_H
