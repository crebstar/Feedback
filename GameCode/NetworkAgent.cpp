#include "NetworkAgent.hpp"


NetworkAgent::~NetworkAgent() {

}


NetworkAgent::NetworkAgent() {

	setNetworkAgentDefaults();
}


NetworkAgent::NetworkAgent( const std::string& serverIPAddress, const std::string& serverPortNumber, bool connectOnConstruction /* = true */ ) :
	m_serverIPAddress( serverIPAddress ),
	m_serverPortNumber( serverPortNumber )
{
	setNetworkAgentDefaults();

	if ( connectOnConstruction ) {

		establishConnectionToServer(); 
	}
}


void NetworkAgent::update( float deltaSeconds ) {

}


void NetworkAgent::render( float deltaSeconds ) const {


}


void NetworkAgent::establishConnectionToServer() {

	printf( "Initializing connection to UDP server from UDP client...\n" );

	WSAData wsaData;
	int wsResult = WSAStartup( MAKEWORD( 2,2 ), &wsaData ) ;

	if ( wsResult != 0 ) {

		printf("WSAStartup failed with error code: %d\n", wsResult );
		return;
	}

	m_serverSocket = INVALID_SOCKET;
	addrinfo* result = nullptr;
	addrinfo hints;

	ZeroMemory( &hints, sizeof( hints ) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM; 
	hints.ai_protocol = IPPROTO_UDP;

	wsResult = getaddrinfo( m_serverIPAddress.c_str(), m_serverPortNumber.c_str(), &hints, &result );

	if ( wsResult != 0 ) {

		printf( "getaddrinfo failed with error: %d\n", wsResult );
		WSACleanup();
		return;
	}

	addrinfo* ptr = nullptr;
	for ( ptr = result; ptr != nullptr; ptr = ptr->ai_next ) {

		m_serverSocket = socket( ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol );
		if ( m_serverSocket == INVALID_SOCKET ) {

			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return;
		}
	}

	freeaddrinfo( result );

	if ( m_serverSocket == INVALID_SOCKET ) {

		printf("Unable to connect to server!\n");
		WSACleanup();
		return;
	}

	// Make socket non blocking ( Will need to expose options in interface in future )
	u_long iMode = 1; // 0 = blocking ... != 0 is non blocking
	wsResult = ioctlsocket( m_serverSocket, FIONBIO, &iMode );
	if ( wsResult != NO_ERROR ) {

		printf( "ioctlsocket failed with error: %ld\n", wsResult );
	}

	m_serverAddressLength = sizeof( m_serverAddress );

	memset( (char*) &m_serverAddress, 0, m_serverAddressLength );
	m_serverAddress.sin_family = AF_INET;

	m_serverAddress.sin_port = htons( ( unsigned short ) atoi( m_serverPortNumber.c_str() ) );
	m_serverAddress.sin_addr.S_un.S_addr = inet_addr( m_serverIPAddress.c_str() );

	m_connectionIsActive = true;
}


void NetworkAgent::establishConnectionToNewServer( const std::string& serverIPAddress, const std::string& serverPortNumber ) {


}


void NetworkAgent::sendPlayerDataPacketToServer( const PlayerDataPacket& playerPacket ) {

	if ( m_connectionIsActive ) {

		int wsResult = 0;
		wsResult = sendto( m_serverSocket, (char*) &playerPacket, sizeof( PlayerDataPacket ), 0, (sockaddr*) &m_serverAddress, m_serverAddressLength ); // For UDP use sendTo

		if  (wsResult == SOCKET_ERROR ) {

			printf( "sendto failed with error: %d\n", WSAGetLastError() );
			//closesocket( connectSocket );
			//WSACleanup();
		}

	} else {

		// TODO:: Send Warning
		int x = 3;
	}
}


bool NetworkAgent::getLatestGamePacketData( PlayerDataPacket& out_playerPacketData ) {

	bool packetIsAvailable = false;

	if ( m_connectionIsActive ) {

		int winSockResult = 0;
		sockaddr_in serverAddress;
		int sizeOfServeraddress = sizeof( serverAddress );

		winSockResult = recvfrom( m_serverSocket, (char*) &out_playerPacketData, sizeof( out_playerPacketData ), 0, (sockaddr*) &serverAddress, &sizeOfServeraddress );

		if ( winSockResult > 0 ) {

			packetIsAvailable = true;
		}

	} else {

		// TODO:: Send Warning
		int x = 3;
	}

	return packetIsAvailable;
}


void NetworkAgent::setNetworkAgentDefaults() {

	m_connectionIsActive = false;
}




