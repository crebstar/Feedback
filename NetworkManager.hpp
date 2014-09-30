#ifndef included_NetworkManager
#define included_NetworkManager
#pragma once

#include <string>

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")


#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../../CBEngine/EngineCode/EngineMacros.hpp"


class NetworkManager {
public:
	static NetworkManager& getSharedNetworkManager() {

		static NetworkManager networkManager;
		return networkManager;
	}

	~NetworkManager();

	bool establishConnectionToNetworkAsServer( const std::string& serverIPAddress, const std::string& serverPortNumber );
	bool establishConnectionToNetworkAsClient( const std::string& serverIPAddress, const std::string& serverPortNumber );

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