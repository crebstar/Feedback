#ifndef included_NetworkManager
#define included_NetworkManager
#pragma once

#include <string>
#include <set>

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")


#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../../CBEngine/EngineCode/EngineMacros.hpp"

/*
	Singleton which manages the WinSock API and UDP connections and sending of packets.
	Can connect as UDP client or UDP Server
*/


class NetworkManager {
public:
	static NetworkManager& getSharedNetworkManager() {

		static NetworkManager networkManager;
		return networkManager;
	}

	~NetworkManager();

	bool establishConnectionToNetworkAsServer( const std::string& serverIPAddress, const std::string& serverPortNumber );
	bool establishConnectionToNetworkAsClient( const std::string& serverIPAddress, const std::string& serverPortNumber );

	// Need to create this function so it is agnostic to packet being sent and relies on existing connection.
	// No data on connecting to clients should be stored here
	//void sendPacket( char* packetData, size_t sizeOfPacket  /* Address?*/ );

	// Mutators and Predicates
	unsigned int getConnectionSocket() const;
	bool isCurrentlyConnected();

protected:

	NetworkManager() {

		setNetworkManagerDefaults();
	}

	void setNetworkManagerDefaults();

private:
	PREVENT_COPY_AND_ASSIGN( NetworkManager );

	SOCKET							m_connectionSocket;

	std::string						m_IPAddress;
	std::string						m_Port;
	sockaddr_in						m_serverAddress;
	size_t							m_serverAddressLength;
	mutable bool					m_isConnected;
};


inline bool NetworkManager::isCurrentlyConnected() {

	return m_isConnected;
}


#endif