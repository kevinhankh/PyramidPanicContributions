#include "AudioSource.h"


AudioSource::AudioSource(GameObject* gameObject) : Component(gameObject)
{
	
}

AudioSource::~AudioSource()
{
}


void AudioSource::playSFX(int sfxFile)
{
	AudioManager::getInstance()->playSound(sfxFile, gameObject->getTransform()->getX(), gameObject->getTransform()->getY());
}
