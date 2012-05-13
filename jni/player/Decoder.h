#ifndef DROIDTV_DECODER_H
#define DROIDTV_DECODER_H

#include "libav.h"

#include <pthread.h>

struct PacketQueue {
	AVPacketList *first, *last;
	int size;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
};

class Decoder
{
public:
	Decoder(AVStream* stream);
	virtual ~Decoder();

	void start();
	void stop();
	int enqueue(AVPacket* packet);
	void dequeue(AVPacket* packet);
	int packets();
	int wait();

protected:
	AVStream* pStream;
	bool pAbort;

	virtual bool prepare() = 0;
	virtual void decode() = 0;
	virtual bool process(AVPacket *packet) = 0;

private:
	static void* runThread(void*);

	PacketQueue mQueue;
	pthread_t mThread;
	bool mRunning;
};

#endif // DROIDTV_DECODER_H
