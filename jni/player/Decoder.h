#ifndef DROIDTV_DECODER_H
#define DROIDTV_DECODER_H

extern "C" {

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

}

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
	PacketQueue mQueue;
	AVStream* mStream;
	bool mRunning;

	virtual bool prepare() = 0;
	virtual bool decode() = 0;
	virtual bool process(AVPacket *packet) = 0;

private:
	static void* runThread(void*);

	pthread_t mThread;
	bool mAbort;
};

#endif // DROIDTV_DECODER_H
