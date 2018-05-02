#pragma once
#include "AudioManager.h"
#include "GameObject.h"
#include "Transform.h"

class AudioSource : public Component
{
private:

public:
	AudioSource(GameObject* gameObject);
	~AudioSource();
	void onStart() {};
	void onUpdate(int ticks) {};
	void onEnd() {};
	void playSFX(int sfxFile);
};
