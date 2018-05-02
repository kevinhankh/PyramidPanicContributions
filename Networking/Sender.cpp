#include "Sender.h"
#include "NetworkingManager.h"
#include "CharacterController.h"
#include "GhostController.h"
#include "GhostPilot.h"
#include "BasePossessableController.h"

Sender::Sender(GameObject* gameObject, int ID) : Component(gameObject)
{
	this->m_id = ID;
}

void Sender::sendCreate()
{
	std::map<std::string, std::string> payload;
	Transform* transform = gameObject->getTransform();
	payload["x"] = std::to_string(transform->getX());
	payload["y"] = std::to_string(transform->getY());
	payload["z"] = std::to_string (transform->getZ ());
	payload["rotation"] = std::to_string(transform->getRotation());
	payload["scale"] = std::to_string(transform->getScale());
	sendNetworkMessage("CREATE", payload);
}

void Sender::sendDestroy()
{
	std::map<std::string, std::string> payload;
	payload["ID"] = std::to_string(gameObject->getId());
	sendNetworkMessage("DESTROY", payload);
}

void Sender::sendEndGame()
{
	std::map<std::string, std::string> payload;
	payload["ID"] = std::to_string(gameObject->getId());
	sendNetworkMessage("ENDGAME", payload);
}

void Sender::sendUpdate()
{
	if (!NetworkingManager::getInstance ()->inGame ())
		return;
	std::map<std::string, std::string> payload;
	Transform* transform = gameObject->getTransform ();

	// Handle Generic Transform
	payload["x"] = std::to_string(transform->getX());
	payload["y"] = std::to_string(transform->getY());
	payload["z"] = std::to_string(transform->getZ());
	payload["rotation"] = std::to_string(transform->getRotation());
	payload["scale"] = std::to_string(transform->getScale());

	// Handle Velocity/Movement

	Vector2 lastMovementVector;
	std::shared_ptr<CharacterController> cc = gameObject->getComponent<CharacterController> ();
	if (cc != nullptr) 
	{
		PlayerPilot* pp = (PlayerPilot*)cc->getPilot();
		lastMovementVector = Vector2(pp->m_lastMoveVector.getX(), pp->m_lastMoveVector.getY());
	}
	else {
		std::shared_ptr<GhostController> gc = gameObject->getComponent<GhostController>();
		auto gp = (GhostPilot*)gc->getPilot();
		if (gp != nullptr)
		{
			lastMovementVector = Vector2(gp->getLastMovement().getX(), gp->getLastMovement().getY());
		}
		else {
			//Ghost is possessing something other than its body
			auto possessable = gc->getPossessingItem();
			if (possessable != nullptr)
			{
				payload["x"] = std::to_string(possessable->getGameObject()->getTransform()->getX());
				payload["y"] = std::to_string(possessable->getGameObject()->getTransform()->getY());
			}
			else {
				std::cout << "ERROR: Updating non Character/Ghost or can't find it" << std::endl;
			}
		}
		
	}

	payload["vecX"] = std::to_string (lastMovementVector.getX());
	payload["vecY"] = std::to_string (lastMovementVector.getY());

	//Send Update Message
	sendNetworkMessage("UPDATE", payload, false);
}

void Sender::spawnPlayers(float p1x, float p1y, float p2x, float p2y)
{
	//Host tells reciever 
	std::map<std::string, std::string> payload;
	payload["p1x"] = p1x;
	payload["p1y"] = p1y;
	payload["p2x"] = p2x;
	payload["p2y"] = p2y;

	sendNetworkMessage("SPAWN", payload);
}

void Sender::sendAttack ()
{
	std::map<std::string, std::string> payload;
	sendNetworkMessage ("ATTACK", payload);
}

void Sender::sendAnimation (int animID, int animReturn)
{
	std::map<std::string, std::string> payload;
	payload["animID"] = std::to_string (animID);
	payload["animReturn"] = std::to_string (animReturn);
	sendNetworkMessage ("ANIMATE", payload);
}

void Sender::sendHurt (int newHP)
{
	std::map<std::string, std::string> payload;
	payload["newHealth"] = std::to_string (newHP);
	sendNetworkMessage ("HURT", payload);
}

void Sender::sendTrySwapItem ()
{
	std::map<std::string, std::string> payload;
	sendNetworkMessage ("TRYSWAPITEM", payload);
}

void Sender::sendSwappedItem ()
{
	std::map<std::string, std::string> payload;
	sendNetworkMessage ("SWAPPEDITEM", payload);
}

void Sender::sendTrigger()
{
	std::map<std::string, std::string> payload;
	sendNetworkMessage("TRIGGER", payload);
}

void Sender::sendGhostMovePossession(Vector2 movement)
{
	std::map<std::string, std::string> payload;
	payload["xVel"] = std::to_string(movement.getX());
	payload["yVel"] = std::to_string(movement.getY());
	sendNetworkMessage("GHOSTMOVEPOSSESSION", payload);
}

void Sender::sendGhostTrigger()
{
	std::map<std::string, std::string> payload;
	sendNetworkMessage("GHOSTTRIGGER", payload);
}

void Sender::sendGhostPossess()
{
	std::map<std::string, std::string> payload;
	sendNetworkMessage("GHOSTPOSSESS", payload);
}

void Sender::sendGhostUnpossess()
{
	std::map<std::string, std::string> payload;
	sendNetworkMessage("GHOSTUNPOSSESS", payload);
}

void Sender::sendNetworkMessage(std::string messageKey, std::map<std::string, std::string> payload, bool useTCP)
{
	if (NetworkingManager::getInstance ()->inGame ()) {
		std::remove_if (messageKey.begin (), messageKey.end (), ::isspace); //::isspace lets it know to use std::isspace override as theres multiple
		if (useTCP)
			NetworkingManager::getInstance ()->prepareMessageForSendingTCP (m_id, messageKey, payload);
		else
			NetworkingManager::getInstance ()->prepareMessageForSendingUDP (m_id, messageKey, payload);
	}
}

Sender::~Sender()
{
	std::map<std::string, std::string> payload;
}

void Sender::onUpdate (int ticks)
{
	m_lastUpdate += ticks;
	if (m_lastUpdate >= 80) {
		sendUpdate ();
		m_lastUpdate = 0;
	}
}

int Sender::getNetworkID() {
	return m_id;
}