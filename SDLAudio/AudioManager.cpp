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

#include "AudioManager.h"

AudioManager* AudioManager::s_instance;

AudioManager* AudioManager::getInstance()
{
	if (s_instance == NULL)
		s_instance = new AudioManager();
	return s_instance;
}

void AudioManager::release()
{
	delete s_instance;
	s_instance = NULL;
}

AudioManager::AudioManager()
{
	Mix_GetError();
	//if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) < 0)
		std::cout << "ERROR Opening Mix_OpenAudio: " << Mix_GetError() << std::endl;

	Mix_AllocateChannels(MAX_CHANNELS);

	m_musicFiles[MUSIC_MENU] = Mix_LoadMUS(BuildPath(PATH_MUSIC_MENU).c_str());
	m_musicFiles[MUSIC_LEVEL_1] = Mix_LoadMUS(BuildPath(PATH_MUSIC_LEVEL_1).c_str());
	m_musicFiles[MUSIC_GHOST] = Mix_LoadMUS(BuildPath(PATH_MUSIC_GHOST).c_str());
	m_audioFiles[SFX_BOW] = Mix_LoadWAV(BuildPath(PATH_SFX_BOW).c_str());
	m_audioFiles[SFX_HIT] = Mix_LoadWAV(BuildPath(PATH_SFX_HIT).c_str());
	m_audioFiles[SFX_SWORD] = Mix_LoadWAV(BuildPath(PATH_SFX_SWORD).c_str());
	m_audioFiles[SFX_SHIELD] = Mix_LoadWAV(BuildPath(PATH_SFX_SHIELD).c_str());
	m_audioFiles[SFX_DASH] = Mix_LoadWAV(BuildPath(PATH_SFX_DASH).c_str());
	m_audioFiles[SFX_DOOR] = Mix_LoadWAV(BuildPath(PATH_SFX_DOOR).c_str());
	m_audioFiles[SFX_BUTTON_HOVER] = Mix_LoadWAV(BuildPath(PATH_SFX_BUTTON_HOVER).c_str());
	m_audioFiles[SFX_ITEM] = Mix_LoadWAV(BuildPath(PATH_SFX_ITEM).c_str());
	m_audioFiles[SFX_TRAP] = Mix_LoadWAV(BuildPath(PATH_SFX_TRAP).c_str());
	m_audioFiles[SFX_THRONE] = Mix_LoadWAV(BuildPath(PATH_SFX_THRONE).c_str());
	std::cout << Mix_GetError() << std::endl;
	m_listener = nullptr;
}

AudioManager::~AudioManager()
{
	Mix_Quit();
}

void AudioManager::setListener(GameObject* listenerObject)
{
	m_listener = listenerObject;
}

//Music will be looped in the background
void AudioManager::playMusic(int musicInput)
{
	if (Mix_PlayMusic(m_musicFiles[musicInput], -1) == -1)
	{
		printf("Mix_PlayMusic: %s\n", Mix_GetError());
	}
}
void AudioManager::pauseMusic()
{
	if (Mix_PlayingMusic() != 0)
		Mix_PauseMusic();
}

void AudioManager::resumeMusic()
{
	if (Mix_PausedMusic() != 0)
		Mix_ResumeMusic();
}

//Sound effects has a pool of 16 channels to play on
void AudioManager::playSound(int sfxInput, float sourceX, float sourceY)
{
	//loop through channels to find first available
	for (int i = 0; i < MAX_CHANNELS; ++i)
	{
		if (!Mix_Playing(i))
		{
			//checks for listener
			if (m_listener != nullptr && m_listener != NULL)
			{
				m_listenerX = m_listener->getTransform()->getX();
				m_listenerY = m_listener->getTransform()->getY();
				//checks distance between player and source
				m_distance = sqrt(pow((sourceX - m_listenerX), 2.0) + pow((sourceY - m_listenerY), 2.0));
			}
			else
			{
				//std::cout << "No listener found!" << endl;
				m_distance = 0;
				//break;
			}

			//play normally when source is withing range of the player
			if (m_distance < MIN_DISTANCE)
			{
				playChannel(i, MAX_VOLUME, m_distance, sfxInput);
				break;
			}
			//set the distance and volume of the channel when the source is further away
			else if (m_distance < MAX_DISTANCE)
			{
				m_distance *= DISTANCE_OFFSET;
				playChannel(i, DISTANCE_VOLUME, m_distance, sfxInput);
				break;
			}
		}
	}
}

void AudioManager::playChannel(int channel, int volume, int distance, int sfxInput)
{
	if (!Mix_Volume(channel, volume))
	{
		std::cout << "ERROR Mix_Volume: " << Mix_GetError() << std::endl;
	}
	if (!Mix_SetDistance(channel, distance))
	{
		std::cout << "ERROR Mix_Volume: " << Mix_GetError() << std::endl;
	}
	if (Mix_PlayChannel(channel, m_audioFiles[sfxInput], 0) == -1)
	{
		std::cout << "ERROR Mix_PlayChannel: " << Mix_GetError() << std::endl;
	}
}

void AudioManager::closeAudio()
{
	Mix_CloseAudio();
}