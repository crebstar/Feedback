#include "NetworkManager.hpp"


NetworkManager::~NetworkManager() {

}


bool NetworkManager::establishConnectionToNetworkAsServer( const std::string& ipAddress, const std::string& portNumber )
{
	printf( "\n\nAttempting to create UDP Server with IP: %s and Port: %s \n", ipAddress.c_str(), portNumber.c_str() );

	m_IPAddress = ipAddress;
	m_Port = portNumber;

	WSAData wsaData;
	int winSockResult = 0;

	m_connectionSocket = INVALID_SOCKET;

	struct addrinfo* result = nullptr;
	struct addrinfo hints;

	winSockResult = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
	if ( winSockResult != 0 ) {

		printf( "WSAStartup failed with error number: %d\n", winSockResult );
		m_isConnected = false;
		return m_isConnected;
	}

	ZeroMemory( &hints, sizeof( hints ) );
	hints.ai_family		= AF_INET;
	hints.ai_socktype	= SOCK_DGRAM;
	hints.ai_protocol	= IPPROTO_UDP;
	hints.ai_flags		= AI_PASSIVE;


	winSockResult = getaddrinfo( m_IPAddress.c_str(), m_Port.c_str(), &hints, &result );
	if ( winSockResult != 0 ) {

		printf( "getaddrinfo function call failed with error number: %d\n", winSockResult );
		WSACleanup();

		m_isConnected = false;
		return m_isConnected;
	}

	// Initialize the ListenSocket ( Connect, Bind, then listen )
	m_connectionSocket = socket( result->ai_family, result->ai_socktype, result->ai_protocol );
	if ( m_connectionSocket == INVALID_SOCKET ) {

		printf( "socket function call failed with error number: %ld\n", WSAGetLastError() );

		freeaddrinfo(result);
		WSACleanup();

		m_isConnected = false;
		return m_isConnected;
	}

	winSockResult = bind( m_connectionSocket, result->ai_addr, static_cast<int>( result->ai_addrlen ) );

	if ( winSockResult == SOCKET_ERROR ) {

		printf( "Bind to listenSocket failed with error number: %d\n", WSAGetLastError() );

		freeaddrinfo( result );
		closesocket( m_connectionSocket );
		WSACleanup();

		m_isConnected = false;
		return m_isConnected;
	}

	u_long iMode = 1; // 0 = blocking ... != 0 is non blocking
	winSockResult = ioctlsocket( m_connectionSocket, FIONBIO, &iMode );	
	if ( winSockResult != NO_ERROR ) {

		printf( "ioctlsocket failed with error: %ld\n", winSockResult );
		return m_isConnected;
	}

	printf( "Connection successfully created for UDP Server" );
	freeaddrinfo(result);

	m_isConnected = true;
	return m_isConnected;
}



bool NetworkManager::establishConnectionToNetworkAsClient( const std::string& serverIPAddress, const std::string& serverPortNumber )
{
	printf( "Initializing connection to UDP server from UDP client...\n" );

	m_IPAddress = serverIPAddress;
	m_Port = serverPortNumber;

	WSAData wsaData;
	int wsResult = WSAStartup( MAKEWORD( 2,2 ), &wsaData ) ;

	if ( wsResult != 0 ) {

		printf("WSAStartup failed with error code: %d\n", wsResult );
		m_isConnected = false;
		return m_isConnected;
	}

	m_connectionSocket = INVALID_SOCKET;
	addrinfo* result = nullptr;
	addrinfo hints;

	ZeroMemory( &hints, sizeof( hints ) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM; 
	hints.ai_protocol = IPPROTO_UDP;

	wsResult = getaddrinfo( m_IPAddress.c_str(), m_Port.c_str(), &hints, &result );

	if ( wsResult != 0 ) {

		printf( "getaddrinfo failed with error: %d\n", wsResult );
		WSACleanup();

		m_isConnected = false;
		return m_isConnected;
	}

	addrinfo* ptr = nullptr;
	for ( ptr = result; ptr != nullptr; ptr = ptr->ai_next ) {

		m_connectionSocket = socket( ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol );
		if ( m_connectionSocket == INVALID_SOCKET ) {

			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();

			m_isConnected = false;
			return m_isConnected;
		}
	}

	freeaddrinfo( result );

	if ( m_connectionSocket == INVALID_SOCKET ) {

		printf("Unable to connect to server!\n");
		WSACleanup();

		m_isConnected = false;
		return m_isConnected;
	}

	// Make socket non blocking ( Will need to expose options in interface in future )
	u_long iMode = 1; // 0 = blocking ... != 0 is non blocking
	wsResult = ioctlsocket( m_connectionSocket, FIONBIO, &iMode );
	if ( wsResult != NO_ERROR ) {

		printf( "ioctlsocket failed with error: %ld\n", wsResult );
	}

	m_serverAddressLength = sizeof( m_serverAddress );

	memset( (char*) &m_serverAddress, 0, m_serverAddressLength );
	m_serverAddress.sin_family = AF_INET;

	m_serverAddress.sin_port = htons( ( unsigned short ) atoi( m_Port.c_str() ) );
	m_serverAddress.sin_addr.S_un.S_addr = inet_addr( m_IPAddress.c_str() );

	m_isConnected = true;
	return m_isConnected;
}

void NetworkManager::setNetworkManagerDefaults() {

	m_connectionSocket = INVALID_SOCKET;
	m_isConnected = false;
	m_Port = "Unknown";
	m_IPAddress = "Unknown";
}


unsigned int NetworkManager::getConnectionSocket() const {

	return m_connectionSocket;
}