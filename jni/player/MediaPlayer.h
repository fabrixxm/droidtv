#ifndef DROIDTV_MEDIAPLAYER_H
#define DROIDTV_MEDIAPLAYER_H

#include "com_chrulri_droidtv_player_AVPlayer.h"

#define STATE_UNINITIALIZED	com_chrulri_droidtv_player_AVPlayer_STATE_UNINITIALIZED
#define STATE_PREPARING		com_chrulri_droidtv_player_AVPlayer_STATE_PREPARING
#define STATE_PREPARED		com_chrulri_droidtv_player_AVPlayer_STATE_PREPARED
#define STATE_PLAYING		com_chrulri_droidtv_player_AVPlayer_STATE_PLAYING
#define STATE_ERROR			com_chrulri_droidtv_player_AVPlayer_STATE_ERROR
#define STATE_FINISHED		com_chrulri_droidtv_player_AVPlayer_STATE_FINISHED
#define STATE_STARTING		com_chrulri_droidtv_player_AVPlayer_STATE_STARTING
#define STATE_STOPPING		com_chrulri_droidtv_player_AVPlayer_STATE_STOPPING

#define ERROR_UNKNOWN			(-1)
#define OK						0
#define ERROR_OPEN				1
#define ERROR_PARSE				2
#define	ERROR_AUDIO_STREAM		3
#define ERROR_AUDIO_DECODER		4
#define ERROR_AUDIO_CODEC		5
#define ERROR_VIDEO_STREAM	 	6
#define ERROR_VIDEO_DECODER		7
#define	ERROR_VIDEO_CODEC		8
#define ERROR_SWS_CONTEXT		9
#define ERROR_ALLOC_FRAME		10
#define ERROR_INVALID_BITMAP	11
#define ERROR_INVALID_TRACK		12
#define ERROR_INVALID_OPERATION	13

#include <android/bitmap.h>
#include <pthread.h>

#include "libav.h"

#include "AudioDecoder.h"
#include "VideoDecoder.h"

class MediaPlayerListener {
public:
	virtual ~MediaPlayerListener();
	virtual jint postAudio(jbyteArray buffer, int size) = 0;
	virtual void postVideo() = 0;
	virtual jboolean postPrepareAudio(int sampleRate) = 0;
	virtual jobject postPrepareVideo(int width, int height) = 0;
	virtual void postNotify(int msg, int ext1, int ext2) = 0;
};

class MediaPlayer {
public:
	MediaPlayer();
	~MediaPlayer();

	int prepare(JNIEnv *env, const char* fileName);
	void setListener(MediaPlayerListener* listener);

	int start();
	int stop();
	int getState();

	void decodeVideo(AVFrame* frame, double pts);
	void decodeAudio(jbyteArray buffer, int size);

	void drawFrame(JNIEnv* env, jobject bitmap);

private:
	static void* runMainThread(void* ptr);

	void decodeStream();

	int mVideoWidth;
	int mVideoHeight;
	int mState;
	MediaPlayerListener* mListener;

	pthread_t mMainThread;

	AVFrame* mFrame;
	AVFormatContext* mInputFile;
	int mAudioStreamIndex;
	int mVideoStreamIndex;

	AudioDecoder* mAudioDecoder;
	VideoDecoder* mVideoDecoder;
};

#endif // DROIDTV_MEDIAPLAYER_H
