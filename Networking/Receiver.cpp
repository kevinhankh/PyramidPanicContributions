#include "Receiver.h"
#include "HostCharacter.h"
#include "NetworkingManager.h"
#include "CharacterController.h"
#include "Sender.h"
#include "HostPilot.h"
#include "GhostController.h"
#include "GhostReceiverPilot.h"
#include "GhostCharacter.h"
#include "PhysicsManager.h"
#include "SpriteRendererManager.h"
#include "Camera.h"
#include "SceneManager.h"
#include "MainMenuScene.h"

Receiver::Receiver(GameObject* gameObject, int netID) : Component(gameObject)
{

	this->netID = netID;
	//Add to map of type "event", key "id"

	this->m_onUpdateID = Subscribe("CREATE", [](std::map<std::string, void*> data) -> void
	{
		float netID = std::stoi(*(std::string*)data["netID"]);
		if (NetworkingManager::getInstance()->isSelf(netID))
			return;
		float type = std::stof(*(std::string*)data["type"]);
		float x = std::stof(*(std::string*)data["x"]);
		float y = std::stof(*(std::string*)data["y"]);
		float z = std::stof(*(std::string*)data["z"]);
		float angle = std::stof(*(std::string*)data["rotation"]);
		float scale = std::stof(*(std::string*)data["scale"]);
		std::cout << "RECEIVED MESSAGE CREATE";
		Receiver* self = (Receiver*)data["this"];
		Transform* transform = self->gameObject->getTransform();
		transform->setPosition(x, y, z);
		transform->setRotation(angle);
		transform->setScale(scale);
	}, this);

	Subscribe("SWAPPEDITEM", [](std::map<std::string, void*> data) -> void
	{
		Receiver* self = (Receiver*)data["this"];
		auto character = self->getGameObject()->getComponent<CharacterController>();
		if (character != nullptr) {
			character->trySwapItem();
		}
	}, this);

	Subscribe("HURT", [](std::map<std::string, void*> data) -> void
	{
		Receiver* self = (Receiver*)data["this"];
		float newHP = std::stoi(*(std::string*)data["newHealth"]);
		auto character = self->getGameObject()->getComponent<CharacterController>();
		if (character != nullptr) {
			character->setHealth(newHP);
		}
	}, this);

	Subscribe("TRYSWAPITEM", [](std::map<std::string, void*> data) -> void {
		float netID = std::stoi(*(std::string*)data["netID"]);
		if (NetworkingManager::getInstance()->isSelf(netID))
			return;
		Receiver* self = (Receiver*)data["this"];
		if (self->getGameObject()->getComponent<CharacterController>()->trySwapItem() != nullptr) {
			std::shared_ptr<Sender> sender = self->getGameObject()->getComponent<Sender>();
			if (sender != nullptr) {
				sender->sendSwappedItem();
			}
		}
	}, this);

	Subscribe("ANIMATE", [](std::map<std::string, void*> data) -> void {
		Receiver* self = (Receiver*)data["this"];
		int animID = std::stoi(*(std::string*)data["animID"]);
		int animReturn = std::stoi(*(std::string*)data["animReturn"]);
		HostCharacter* host = dynamic_cast<HostCharacter*>(self->getGameObject());
		if (host != nullptr) {
			if (animReturn != -1) {
				host->playAnimation(animID, animReturn);
			}
			else {
				host->playAnimation(animID);
			}
		}
	}, this);

	this->m_onUpdateID = Subscribe("DESTROY", [](std::map<std::string, void*> data) -> void
	{
		std::cout << "DESTROY CALLED" << std::endl;
		int id = std::stoi(*(std::string*)data["ID"]);
		Receiver* self = (Receiver*)data["this"];
		if (self != nullptr && self->getGameObject() != nullptr) {
			auto character = dynamic_cast<Character*>(self->getGameObject());
			if (character != nullptr) {
				character->onNetworkEnd();
			}
			else {
				std::cout << "ReceiverError::DESTROY tried to destroy a non-character" << std::endl;
			}
		}
	}, this);

	this->m_onUpdateID = Subscribe("UPDATE", [](std::map<std::string, void*> data) -> void
	{
		float netID = std::stoi(*(std::string*)data["netID"]);
		if (NetworkingManager::getInstance()->isSelf(netID))
			return;
		float x = std::stof(*(std::string*)data["x"]);
		float y = std::stof(*(std::string*)data["y"]);
		float z = std::stof(*(std::string*)data["z"]);
		float vecX = std::stof(*(std::string*)data["vecX"]);
		float vecY = std::stof(*(std::string*)data["vecY"]);
		float angle = std::stof(*(std::string*)data["rotation"]);
		float scale = std::stof(*(std::string*)data["scale"]);
		Receiver* self = (Receiver*)data["this"];
		Transform* transform = self->gameObject->getTransform();
		transform->setPosition(x, y, z);
		transform->setRotation(angle);
		transform->setScale(scale);
		Vector2 movement = Vector2(vecX, vecY);

		std::shared_ptr<CharacterController> cc = self->gameObject->getComponent<CharacterController>();
		if (cc != nullptr)
		{
			HostPilot* hostPilot = dynamic_cast<HostPilot*>(cc->getPilot());
			if (hostPilot != nullptr)
			{
				hostPilot->setMovement(movement, 6);
			}
		}
		else {
			auto ghostController = self->gameObject->getComponent<GhostController>();
			if (ghostController != nullptr)
			{
				GhostReceiverPilot* ghostPilot = dynamic_cast<GhostReceiverPilot*>(ghostController->getPilot());
				if (ghostPilot != nullptr)
				{
					ghostPilot->setMovement(movement, 6);
				}
			}
			//We are a ghost not a character. We may or may not need to do movement equivalency
		}
	}, this);

	this->m_onUpdateID = Subscribe("ENDGAME", [](std::map<std::string, void*> data) -> void
	{
		float netID = std::stoi(*(std::string*)data["ID"]);
		if (NetworkingManager::getInstance()->isSelf(netID))
			return;
        PhysicsManager::getInstance()->purge();
        SpriteRendererManager::getInstance()->purge();
        NetworkingManager::getInstance()->hardReset();
        Camera::getActiveCamera()->setActiveCamera(GameManager::getInstance()->createGameObject<Camera>(true));
        SceneManager::getInstance()->pushScene(new MainMenuScene());
	}, this);
}

Receiver::~Receiver()
{
	MessageManager::unSubscribe(std::to_string(netID) + "|UPDATE", this->m_onUpdateID);
}

int Receiver::Subscribe(std::string event, Callback callback, void* owner)
{
	return MessageManager::subscribe(std::to_string(netID) + "|" + event, callback, owner);
}
