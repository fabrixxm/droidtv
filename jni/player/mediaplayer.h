#ifndef DROIDTV_MEDIAPLAYER_H
#define DROIDTV_MEDIAPLAYER_H

#include "com_chrulri_droidtv_player_AVPlayer.h"

#define STATE_PREPARING com_chrulri_droidtv_player_AVPlayer_STATE_PREPARING

class MediaPlayerListener {
public:
	void postAudio(int16_t* buffer, int size) = 0;
	void postVideo() = 0;
	void postPrepare(int width, int height) = 0;
	void postNotify(int msg, int ext1, int ext2) = 0;
};

class MediaPlayer {
public:
	int prepare(const char* fileName);
	void setListener(MediaPlayerListener* listener);

	int start();
	int stop();
	int getState();

private:
	int mState;
	MediaPlayerListener* mListener;
};

#endif // DROIDTV_MEDIAPLAYER_H
