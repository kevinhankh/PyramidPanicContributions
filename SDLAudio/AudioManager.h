/*
	Audio Manager

	Uses Mix_PlayMusic for background soundtrack.
	Cycles through 16 available channels for playing sound effects. (plays on first available channel)
	Listener is a pointer to the player game object.
	Audio source is played by a game object with its x and y position.
	The manager calculates the distance between the listener and source to apply distanced effect by altering the volume.

	*Latest Update - March 29, 2018
		- Added fixes to play audio on mac
		- Changed sound effects to .wav files
		- Changed music from channel 0 to Mix_PlayMusic to keep it as an mp3 file
		- Moved all audio file paths and keys definition to AudioBank.h

	*Update - March 18, 2018
		- Changed to a channel oriented system
		- Added distance sound effects functionality

	@author Kevin Han
*/

#pragma once
#include "GLHeaders.h"
#include <iostream>
#include <string>
#include "AudioBank.h"
#include "HelperFunctions.h"
#include "GameObject.h"
#include "SpriteRendererManager.h"
#include "Transform.h"

#define MAX_CHANNELS 16
#define MIN_DISTANCE 25
#define MAX_DISTANCE 40
#define MAX_VOLUME 128
#define DISTANCE_OFFSET 4
#define DISTANCE_VOLUME 100

class AudioManager
{
private:
	static AudioManager* s_instance;

	GameObject* m_listener;
	std::map<int, Mix_Chunk*> m_audioFiles;
	std::map<int, Mix_Music*> m_musicFiles;

	float m_distance;
	float m_listenerX;
	float m_listenerY;

	AudioManager();
	~AudioManager();

public:
	static AudioManager* getInstance();
	static void release();
	void setListener(GameObject* listenerObject);
	void playMusic(int musicInput);
	void pauseMusic();
	void resumeMusic();
	void playSound(int sfxInput, float x, float y);
	void playChannel(int channel, int volume, int distance, int sfxInput);
	void closeAudio();
};
