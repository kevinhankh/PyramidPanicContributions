#include "AudioListener.h"


AudioListener::AudioListener(GameObject* gameObject) : Component(gameObject)
{
	AudioManager::getInstance()->setListener(gameObject);
}


AudioListener::~AudioListener()
{
}