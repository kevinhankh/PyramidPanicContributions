#pragma once
#include "AudioManager.h"
#include "GameObject.h"
#include "Transform.h"

class AudioListener : public Component
{
private:

public:
	AudioListener(GameObject* gameObject);
	~AudioListener();
	void onStart() {};
	void onUpdate(int ticks) {};
	void onEnd() {};
};