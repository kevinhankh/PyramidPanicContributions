#include "NetworkingManager.h"
#include "MessageManager.h"
#include "SpawnManager.h"

NetworkingManager* NetworkingManager::s_instance;

NetworkingManager* NetworkingManager::getInstance()
{
	if (s_instance == NULL)
		s_instance = new NetworkingManager();
	return s_instance;
}

void NetworkingManager::hardReset()
{
	closeClient();
	closeUDP();
	s_instance = new NetworkingManager();
}

NetworkingManager::NetworkingManager()
{
	SDLNet_Init();
	m_messageQueue = new ThreadQueue<std::string>();
}

bool NetworkingManager::createHost()
{
	return host();
}

bool NetworkingManager::createClient()
{
	return join();
}

bool NetworkingManager::isConnected()
{
	return m_socket != NULL;
}

bool NetworkingManager::isHost()
{
	return m_isHost;
}

IPaddress NetworkingManager::getIP() {
	IPaddress ip;
	SDLNet_ResolveHost(&ip, NULL, m_port);
	return ip;
}

bool NetworkingManager::startGame() {
	
	if (!isHost() || !m_inLobby || m_gameStarted)
		return false;

	m_inLobby = false;
	m_gameStarted = true;
	std::cout << "Server start game!" << std::endl;
	return true;
}

bool NetworkingManager::startGameClient() {
	if (isHost() || !m_inLobby || m_gameStarted)
		return false;

	m_inLobby = false;
	m_gameStarted = true;
	std::cout << "Client start game!" << std::endl;
	return true;
}

bool NetworkingManager::host()
{
	if (m_inLobby || m_gameStarted)
		return false;
	// create a listening TCP socket on port 9999 (server)
	IPaddress ip;
	int channel;

	if (SDLNet_ResolveHost(&ip, NULL, m_port) == -1)
	{
		printf("SDLNet_ResolveHost: %s\n", SDLNet_GetError());
		return false;
	}

	m_socket = SDLNet_TCP_Open(&ip);
	if (!m_socket)
	{
		//  printf("SDLNet_TCP_Open: %s\n", SDLNet_GetError());
		std::string error = SDLNet_GetError();
		return false;
	}
	m_udpSocket = SDLNet_UDP_Open(m_port);
	if (!m_udpSocket)
	{
		std::string udpError = SDLNet_GetError();
		return false;
	}

	m_inLobby = true;
	m_gameStarted = false;
	m_isHost = true;

	std::cout << "Hosting server." << std::endl;
	addPlayer (ip.host, m_socket);
	m_assignedID = 0;
	pollSocketAccept ();
	pollMessagesUDP ();
}

void NetworkingManager::pollSocketAccept ()
{
	m_socketAcceptThread = std::thread (&NetworkingManager::socketAcceptThread, this);
	m_socketAcceptThread.detach ();
}

void NetworkingManager::socketAcceptThread ()
{
	while (m_inLobby) {
		if (accept ())
		{
			std::cout << "Connection established." << std::endl;
		}
	}
}

bool NetworkingManager::accept()
{
	if (m_gameStarted)
		return false;

	TCPsocket m_client = SDLNet_TCP_Accept(m_socket);
	if (!m_client)
	{
		return false;
	}
	IPaddress *ip = SDLNet_TCP_GetPeerAddress(m_client);
	int newID;
	if ((newID = addPlayer (ip->host, m_client)) == -1) {
		SDLNet_TCP_Close (m_client);
		std::cout << "Too many players." << std::endl;
		return false;
	}
	pollMessagesTCP(newID);

	IPaddress udpIP;
	if (SDLNet_ResolveHost (&udpIP, SDLNet_ResolveIP (ip), m_port) == -1) {
		std::cout << "Couldn't find resolved client IP " << SDLNet_ResolveIP (ip) << std::endl;
	}
	int channel = -1;
	if ((channel = SDLNet_UDP_Bind (m_udpSocket, -1, &udpIP)) == -1)
	{
		printf ("SDLNet_UDP_Bind to channel: %s\n", SDLNet_GetError ());
	}
	if (channel >= 0 && channel <= 3) //16 max connections
		channels[channel] = true;
	
	sendAcceptPacket (newID);
	// communicate over new_tcpsock
	std::cout << "Accepted a client." << std::endl;
	return true;
}

bool NetworkingManager::join ()
{
	if (m_inLobby || m_gameStarted)
		return false;
	
	if (SDLNet_ResolveHost (&m_hostAddress, IP, m_port) == -1)
	{
		printf ("SDLNet_ResolveHost: %s\n", SDLNet_GetError ());
		return false;
	}


	listenforAcceptPacket ();

	m_socket = SDLNet_TCP_Open (&m_hostAddress);
	pollMessagesTCP (addPlayer (m_hostAddress.host, m_socket));
	if (!m_socket)
	{
		printf ("SDLNet_TCP_Open: %s\n", SDLNet_GetError ());
		stopListeningForAcceptPacket ();
		SDLNet_TCP_Close (m_socket);
		return false;
	}
		
	m_udpSocket = SDLNet_UDP_Open(m_port);
	if (!m_udpSocket)
	{
		printf("SDLNet_UDP_Open: %s\n", SDLNet_GetError());
		closeUDP();
		return false;
	}

	//if (-1 == SDLNet_UDP_Bind(m_udpSocket, DEFAULT_CHANNEL, &m_hostAddress))
	//{
	//	printf("SDLNet_UDP_Bind: %s\n", SDLNet_GetError());
	//}

	m_inLobby = true;
	m_gameStarted = false;

	std::cout << "Joined as a client." << std::endl;
	pollMessagesUDP();
	return true;
}

bool NetworkingManager::closeClientAsHost (int id) {
	if (m_clients.find (id) != m_clients.end ())
	{
		SDLNet_TCP_Close (m_clients[id].second);
		m_clients.erase (id);
	}
	return true;
}

bool NetworkingManager::closeClient()
{
	if (m_socket != NULL)
	{
		SDLNet_TCP_Close(m_socket);
		m_socket = NULL;
	}
	return true;
}

bool NetworkingManager::closeUDP()
{
	if (m_udpSocket != NULL)
	{
		SDLNet_UDP_Close(m_udpSocket);
		m_udpSocket = NULL;
	}
	return true;
}

//Host->Sending Messages->Client Exits->Host Crashes on line SDLNet_TCP_Send
void NetworkingManager::send(int id, std::string *msg)
{
	int result, len;
	len = msg->length() + 1;

	//std::cout << "Sending: ID: " << id << " Packet: " << m_clients[id].first << " Message: " << *msg << std::endl;

	if (m_clients.find (id) != m_clients.end ()) {
		result = SDLNet_TCP_Send (m_clients[id].second, msg->c_str (), len);
	}
	else if (m_socket != NULL) {
		result = SDLNet_TCP_Send (m_socket, msg->c_str (), len);
	}

	if (result < len)
	{
		//printf("SDLNet_TCP_Send: %s\n", SDLNet_GetError());
	}
}

bool NetworkingManager::createUDPPacket(int packetSize)
{
	m_udpPacket = SDLNet_AllocPacket(packetSize);

	if (m_udpPacket == NULL)
	{
		std::cout << "SDLNet_AllocPacket failed : " << SDLNet_GetError() << "\n";
		return false;
	}

	if (!isHost ()) {
		m_udpPacket->address = m_hostAddress;
	}
	m_udpPacket->len = packetSize + 1;

	return true;
}

void NetworkingManager::sendUDP(std::string *msg)
{
	createUDPPacket(msg->length());
	memcpy(m_udpPacket->data, msg->c_str(), msg->length());

	if (isHost ()) {
		for (size_t i = 0; i < 4; i++) {
			if (channels[i]) {
				m_udpPacket->channel = i;
				if (!SDLNet_UDP_Send (m_udpSocket, i, m_udpPacket))
					std::cout << "SDLNET_UDP_SEND failed: " << SDLNet_GetError () << "\n";
			}
		}
	}
	else {
		if (!SDLNet_UDP_Send (m_udpSocket, -1, m_udpPacket))
			std::cout << "SDLNET_UDP_SEND failed: " << SDLNet_GetError () << "\n";
	}
}

void NetworkingManager::pollMessagesTCP(int id)
{
	m_messagesToSendTCP.clear();
	m_receiverThread = std::thread(&NetworkingManager::pollMessagesThreadTCP, this, id);
	m_receiverThread.detach();
}

void NetworkingManager::pollMessagesThreadTCP(int id)
{

	int result;
	char msg[MAXLEN_TCP];

	while (m_socket != NULL) { 
		if (m_clients.find(id) != m_clients.end())
			result = SDLNet_TCP_Recv(m_clients[id].second, msg, MAXLEN_TCP);
		else if (m_socket != NULL)
			result = SDLNet_TCP_Recv(m_socket, msg, MAXLEN_TCP);
		if (result <= 0)
		{
			if (isHost ()) {
				closeClientAsHost (id);
			} else {
				closeClient ();
			}
			continue;
		}
		std::string newMsg = msg;
		//std::cout << "RECIEVING: " << msg << std::endl;
		m_messageQueue->push(newMsg);
	};
	if (isHost ()) {
		closeClientAsHost (id);
	}
	else {
		closeClient ();
	}
}


void NetworkingManager::sendAcceptPacket (int id) {
	std::string packet = "[{key:ACCEPT,netID:0,myNetID:" + std::to_string (id) + "}]";
	send (id, &packet);
}

void NetworkingManager::listenforAcceptPacket ()
{
	this->m_handshakeListenerID = MessageManager::subscribe ("0|ACCEPT", [](std::map<std::string, void*> data) -> void
	{
		NetworkingManager::getInstance()->m_assignedID = std::stoi (*(std::string*)data["myNetID"]);
		NetworkingManager::getInstance ()->stopListeningForAcceptPacket ();
		SpawnManager::getInstance ()->listenForStartPacket ();

	}, this);
}

void NetworkingManager::stopListeningForAcceptPacket () {
	std::cout << "Our Network ID: " << m_assignedID << std::endl;
	MessageManager::unSubscribe ("0|ACCEPT", m_handshakeListenerID);
}

//void NetworkingManager::sendStartPacket (Uint32 ip) {
//	//includes sycnronization (must pass all other players)
//	std::string packet = "[{key:HANDSHAKE|START, ";
//
//	int i = 0;
//	for (auto it = m_clients.begin (); it != m_clients.end (); it++) {
//		packet += "player" + std::to_string(i) + ": " + std::to_string (it->first);
//	}
//
//	packet += "}]";
//
//	send (ip, &packet);
//}

void NetworkingManager::pollMessagesUDP()
{
	m_messagesToSendUDP.clear();
	m_udpReceiverThread = std::thread(&NetworkingManager::pollMessagesThreadUDP, this);
	m_udpReceiverThread.detach();
}

void NetworkingManager::pollMessagesThreadUDP()
{
	int result;
	char msg[MAXLEN_UDP];

	while (m_udpSocket != NULL)
	{ //replace with on connection lost

		UDPpacket *recPacket = SDLNet_AllocPacket(MAXLEN_UDP);

		if (m_udpSocket != NULL)
			result = SDLNet_UDP_Recv(m_udpSocket, recPacket);
		if (result == -1)
		{

		}
		else if (result == 0)
		{

		}
		else if (result == 1)
		{
			std::string newMsg (recPacket->data, recPacket->data + recPacket->len);
			m_messageQueue->push (newMsg);
		}

		SDLNet_FreePacket(recPacket);
		recPacket = NULL;
	}
	closeUDP();
}

bool NetworkingManager::getMessage(std::string &msg)
{
	if (!m_messageQueue->isEmpty())
	{
		m_messageQueue->pop(msg);
		//std::cout << "Msg: " << msg << std::endl;
		return true;
	}
	return false;
}

void NetworkingManager::prepareMessageForSendingUDP (int netID, std::string key, std::map<std::string, std::string> data)
{
	Message message;
	message.netID = netID;
	message.key = key;
	message.data = data;
	m_messagesToSendUDP.push_back (message);
}

void NetworkingManager::prepareMessageForSendingTCP (int netID, std::string key, std::map<std::string, std::string> data)
{
	Message message;
	message.netID = netID;
	message.key = key;
	message.data = data;
	m_messagesToSendTCP.push_back (message);
}

void NetworkingManager::sendQueuedEvents () {
	sendQueuedEventsTCP ();
	sendQueuedEventsUDP ();
}

void NetworkingManager::sendQueuedEventsTCP ()
{
	if (m_messagesToSendTCP.size () < 1)
		return;
	std::string packet = "[";
	for (size_t i = 0; i < m_messagesToSendTCP.size (); i++)
	{
		packet += serializeMessage (m_messagesToSendTCP[i]);
		packet += ",";
	}
	packet.pop_back ();
	packet += "]";
	//Submit it
	m_messagesToSendTCP.clear ();

	for (auto it = m_clients.begin (); it != m_clients.end (); it++) {
		if ((it->second).first == -1) {
			m_clients.erase (it);
			--it;
		}
		else if (m_assignedID != it->first) {
			send(it->first, new std::string(packet));
		}
	}
}

void NetworkingManager::sendQueuedEventsUDP ()
{
	if (m_messagesToSendUDP.size () < 1)
		return;
	std::string packet = "[";
	for (size_t i = 0; i < m_messagesToSendUDP.size (); i++)
	{
		packet += serializeMessage (m_messagesToSendUDP[i]);
		packet += ",";
	}
	packet.pop_back ();
	packet += "]";
	//Submit it
	m_messagesToSendUDP.clear ();
	sendUDP(new std::string(packet));
}

void NetworkingManager::sendEventToReceiver(std::map<std::string, void*> data)
{
	std::string* key = (std::string*)data["key"];
	std::string netID = *(std::string*)data["netID"];
	std::string value = netID + "|" + *key;
	//std::cout << "Event: " << value << " NetID: " << netID << std::endl;
	MessageManager::sendEvent(value, data);
}

std::string NetworkingManager::serializeMessage(Message message)
{
	std::string result = "{";
	message.data["netID"] = std::to_string(message.netID).c_str();
	message.data["key"] = message.key.c_str ();

	for (const auto tuple : message.data)
	{
		result += tuple.first + ":" + message.data[tuple.first] + ",";
	}
	result.pop_back();
	result += "}";
	return result;
}


void NetworkingManager::handleParsingEvents(std::string packet)
{
	std::vector<std::string> messages;
	if (packet.size() > 2)
	{
		packet.erase(0, 1);
		packet.pop_back();
		std::string currentMessage;
		bool reading = false;
		while (packet.size() > 0)
		{
		if (!reading)
		{
			if (packet[0] == '{')
			{
				reading = true;
				currentMessage += packet[0];
			}
		}
		else
		{
			currentMessage += packet[0];
			if (packet[0] == '}')
			{
				reading = false;
				messages.push_back (currentMessage);
				sendEventToReceiver (deserializeMessage (currentMessage));
				currentMessage = "";
			}
		}

		packet.erase (0, 1);
		}
	}
}



//TODO: Deserialize this:
//Example: {key : Player|UPDATE,rotation : 37.000000,scale : 1.000000,x : 1.000000,y : 0.000000}
std::map<std::string, void*> NetworkingManager::deserializeMessage (std::string message)
{
	std::map<std::string, void*> data;
	std::string currentKey = "";
	std::string currentValue = "";
	bool readingKey;
	bool readingValue;
	while (message.size () > 0)
	{
		char curChar = message[0];
		message.erase (0, 1);

		if (curChar == ',' || curChar == '{')
		{
			//Start reading key
			readingKey = true;
			readingValue = false;
			if (curChar == ',')
			{
				data[currentKey] = (void*)new std::string (currentValue);
			}
			std::string newCurrentString = "";
			std::string newCurrentValue = "";
			currentKey = newCurrentString;
			currentValue = newCurrentValue;
			continue;
		}
		else if (curChar == ':')
		{
			//Start reading value
			readingValue = true;
			readingKey = false;
			continue;
		}
		else if (curChar == '}')
		{
			//End
			data[currentKey] = (void*)new std::string (currentValue);
			break;
		}
		if (!::isspace (curChar))
		{
			if (readingKey)
			{
				currentKey += curChar;
			}
			else if (readingValue)
			{
				currentValue += curChar;
			}
		}

	}
	return data;
}

void NetworkingManager::setIP (char *ip, int p)
{
	IP = ip;
	m_port = p;
}

int NetworkingManager::addPlayer (Uint32 ip, TCPsocket sock)
{
	if (m_clients.size () > 16)
	{
		return -1;
	}
	int id = m_clients.size ();
	m_clients.insert (std::pair<int, std::pair<Uint32, TCPsocket>> (id, std::pair<Uint32, TCPsocket> (ip, sock)));

	std::cout << "---- " << m_clients.size() << std::endl;
	for (auto it = m_clients.begin (); it != m_clients.end (); it++) {
		std::cout << "Client: " << it->first << " IP: " << (it->second).first << std::endl;
	}

	return id;
}

int NetworkingManager::removePlayer(int id)
{
	for (auto it = m_clients.begin(); it != m_clients.end(); it++)
	{
		if (it->first == id) 
		{
			(it->second).first = -1;
			return 1;
		}
	}
	return 0;
}

bool NetworkingManager::isSelf (int id) {
	return m_assignedID == id;
}