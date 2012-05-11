#ifndef DROIDTV_AUDIO_DECODER_H
#define DROIDTV_AUDIO_DECODER_H

#include "Decoder.h"


class AudioDecoderListener {
public:
	virtual ~AudioDecoderListener();
	virtual void decodeAudioFrame(int16_t* buffer, int size) = 0;
};

class AudioDecoder : public Decoder
{
public:
	AudioDecoder(AVStream* stream, AudioDecoderListener* listener);
	~AudioDecoder();

private:
	bool prepare();
	bool decode();
	bool process(AVPacket* packet);

	int16_t* mSamples;
	int mSamplesSize;
	AudioDecoderListener* mListener;
};

#endif // DROIDTV_AUDIO_DECODER_H
