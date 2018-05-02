#pragma once

#include <string>
#include "GameManager.h"
#include "TransformState.h"
#include "GameObject.h"
#include <vector>
#include "MessageManager.h"
#include "Transform.h"
#include <iostream>

class Receiver : public Component
{
private:
	int m_onUpdateID, m_destroySnowballID, m_destroyBattlerID;
	std::vector<int> m_messengingIDs;

public:
	int Subscribe(std::string event, Callback callback, void* owner);
	Receiver(GameObject* gameObject, int netID);
	~Receiver(); //Could be death message later
	//void ReceiveUpdate(TransformState* equivalentTransform);
	void onStart() {};
	void onUpdate(int ticks) {};
	void onEnd() {};
	int netID;
};