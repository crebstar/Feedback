#include "FeedbackWorld.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/gl.h> // ugh get rid of this dependency for rendering debug axis
#include <string>
#include <set>

#include "../../CBEngine/EngineCode/GameDirector.hpp"
#include "../../CBEngine/EngineCode/InputHandler.hpp" 
#include "../../CBEngine/EngineCode/TimeSystem.hpp"
#include "../../CBEngine/EngineCode/Clock.hpp"
#include "../../CBEngine/EngineCode/EntityMesh.hpp"
#include "../../CBEngine/EngineCode/CB3DSMaxImporter.hpp"
#include "../../CBEngine/EngineCode/MathUtil.hpp"
#include "../../CBEngine/EngineCode/MatrixStack.hpp"
#include "../../CBEngine/EngineCode/Matrix44.hpp"
#include "../../CBEngine/EngineCode/Geometry3D.hpp"
#include "../../CBEngine/EngineCode/TimeUtil.hpp"

#include "../../CBEngine/EngineCode/Console.hpp"
#include "../../CBEngine/EngineCode/TextRenderer.hpp"
#include "../../CBEngine/EngineCode/FontManager.hpp"
#include "../../CBEngine/EngineCode/BitmapFont.hpp"
#include "../../CBEngine/EngineCode/CBStringHelper.hpp"


#include "GameConstants.hpp"
#include "FeedbackGame.hpp"
#include "NetworkAgent.hpp"

#include "../../CBEngine/EngineCode/MemoryMacros.hpp"

FeedbackWorld::~FeedbackWorld() {

	freeMemoryOfOwnedObjectsAndCleanUp();
}


void FeedbackWorld::freeMemoryOfOwnedObjectsAndCleanUp() {


	// TODO:: Delete and clean up EntityMesh
}


FeedbackWorld::FeedbackWorld() {

	setDefaultVariableValues();
}



void FeedbackWorld::update( float deltaSeconds ) {

	if ( m_gameState == GAME_STATE_RUNNING ) {

		processKeyboardInput( deltaSeconds );

		computePlayerDesiredPosition( deltaSeconds );

		sendPlayerDesiredPosition( deltaSeconds );

		getUpdatedGameDataFromNetworkAgent();

		m_player.update( deltaSeconds );

		for ( int i = 0; i < m_otherPlayers.size(); ++i ) {

			GameObject* otherPlayer = m_otherPlayers[i];
		}

		m_flag.update( deltaSeconds );

		checkForCollisionsWithFlag( deltaSeconds );

	} else if ( m_gameState == GAME_STATE_WAITING_FOR_SERVER ) {

		attemptToConnectToServer( deltaSeconds );

	} else if ( m_gameState == GAME_STATE_IN_LOBBY ) {

		processKeyboardInput( deltaSeconds );

		getLobbyDataFromNetworkAgent( deltaSeconds );
	}
	
}


void FeedbackWorld::getLobbyDataFromNetworkAgent( float deltaSeconds ) {

	cbengine::GameDirector* sharedGameDirector = cbengine::GameDirector::sharedDirector();

	FeedbackGame* fbGame = dynamic_cast<FeedbackGame*>( sharedGameDirector->getCurrentWorld() );

	if ( fbGame != nullptr ) {

		NetworkAgent* nwAgent = fbGame->getCurrentNetworkAgent();
		if ( nwAgent != nullptr ) {

			std::set<CS6Packet> lobbyPackets;
			std::set<CS6Packet> resetPackets;
			nwAgent->getLobbyPacketsInOrder( lobbyPackets, resetPackets );

			std::set<CS6Packet>::iterator itLob;

			if ( !lobbyPackets.empty() ) {

				itLob = lobbyPackets.begin();
				if ( itLob != lobbyPackets.end() ) 
				{
					const CS6Packet& lobbyList = *(itLob);
					m_numGamesFromLobby = lobbyList.data.gameList.totalNumGames;

				} else {

					m_numGamesFromLobby = 0;
				}

			} else {

				m_numGamesFromLobby = 0;
			}

			if ( !resetPackets.empty() ) {

				std::set<CS6Packet>::iterator itRes = resetPackets.begin();

				const CS6Packet& resetPacket = *(itRes);

				m_player.m_playerColor.x = static_cast<float>( resetPacket.data.reset.playerColorAndID[0] );
				m_player.m_playerColor.y = static_cast<float>( resetPacket.data.reset.playerColorAndID[1] );
				m_player.m_playerColor.z = static_cast<float>( resetPacket.data.reset.playerColorAndID[2] );
				m_player.m_playerColor.x = cbengine::rangeMapFloat( 0.0f, 255.0f, 0.0f, 1.0f, m_player.m_playerColor.x );
				m_player.m_playerColor.y = cbengine::rangeMapFloat( 0.0f, 255.0f, 0.0f, 1.0f, m_player.m_playerColor.y );
				m_player.m_playerColor.z = cbengine::rangeMapFloat( 0.0f, 255.0f, 0.0f, 1.0f, m_player.m_playerColor.z );

				m_player.m_position.x = resetPacket.data.reset.playerXPosition;
				m_player.m_position.y = resetPacket.data.reset.playerYPosition;
				m_player.m_currentVelocity.x = 0.0f;
				m_player.m_currentVelocity.y = 0.0f;
				m_player.m_orientationDegrees = 0.0f;
				m_player.m_isFlag = false;

				m_flag.m_isFlag = true;
				m_flag.m_playerColor.x = 0.45f;
				m_flag.m_playerColor.y = 0.45f;
				m_flag.m_playerColor.z = 0.45f;

				m_flag.m_position.x = resetPacket.data.reset.flagXPosition;
				m_flag.m_position.y = resetPacket.data.reset.flagYPosition;
				m_flag.m_currentVelocity.x = 0.0f;
				m_flag.m_currentVelocity.y = 0.0f;
				m_flag.m_orientationDegrees = 0.0f;

				m_gameState = GAME_STATE_RUNNING;
				
			}
		}
	}
}


void FeedbackWorld::attemptToConnectToServer( float deltaSeconds ) {

	cbengine::GameDirector* sharedGameDirector = cbengine::GameDirector::sharedDirector();

	FeedbackGame* fbGame = dynamic_cast<FeedbackGame*>( sharedGameDirector->getCurrentWorld() );

	if ( fbGame != nullptr ) {

		NetworkAgent* nwAgent = fbGame->getCurrentNetworkAgent();
		if ( nwAgent != nullptr ) {

			CS6Packet lobbyPacket;
			bool lobbyPacketReceived = nwAgent->requestToJoinServer( deltaSeconds, lobbyPacket );
			if ( lobbyPacketReceived ) {

				// Player
				m_player.m_playerColor.x = static_cast<float>( lobbyPacket.playerColorAndID[0] );
				m_player.m_playerColor.y = static_cast<float>( lobbyPacket.playerColorAndID[1] );
				m_player.m_playerColor.z = static_cast<float>( lobbyPacket.playerColorAndID[2] );
				m_player.m_playerColor.x = cbengine::rangeMapFloat( 0.0f, 255.0f, 0.0f, 1.0f, m_player.m_playerColor.x );
				m_player.m_playerColor.y = cbengine::rangeMapFloat( 0.0f, 255.0f, 0.0f, 1.0f, m_player.m_playerColor.y );
				m_player.m_playerColor.z = cbengine::rangeMapFloat( 0.0f, 255.0f, 0.0f, 1.0f, m_player.m_playerColor.z );
			
				m_gameState = GAME_STATE_IN_LOBBY;

				CS6Packet lobbyAck;
				lobbyAck.packetType = TYPE_Acknowledge;
				lobbyAck.packetNumber = lobbyPacket.packetNumber;
				lobbyAck.data.acknowledged.packetType = TYPE_JoinLobby;
				lobbyAck.data.acknowledged.packetNumber = lobbyPacket.packetNumber;

				nwAgent->sendAckPacket( lobbyAck );
			}
		}
	}
}


void FeedbackWorld::checkForCollisionsWithFlag( float deltaSeconds ) {

	bool playerDidCollideWithFlag = cbengine::doesDiskIntersectDiskOrTouch( m_player.m_collisionDisk, m_flag.m_collisionDisk );

	if ( playerDidCollideWithFlag ) {

		cbengine::GameDirector* sharedGameDirector = cbengine::GameDirector::sharedDirector();

		FeedbackGame* fbGame = dynamic_cast<FeedbackGame*>( sharedGameDirector->getCurrentWorld() );

		if ( fbGame != nullptr ) {

			NetworkAgent* nwAgent = fbGame->getCurrentNetworkAgent();
			if ( nwAgent != nullptr ) {

				CS6Packet victoryPacket;
				victoryPacket.packetType = TYPE_Victory;
				victoryPacket.packetNumber = 0;
				float unNormalizedRed = cbengine::rangeMapFloat( 0.0f, 1.0f, 0.0f, 255.0f, m_player.m_playerColor.x );
				float unNormalizedGreen = cbengine::rangeMapFloat( 0.0f, 1.0f, 0.0f, 255.0f, m_player.m_playerColor.y );
				float unNormalizedBlue = cbengine::rangeMapFloat( 0.0f, 1.0f, 0.0f, 255.0f, m_player.m_playerColor.z );
				victoryPacket.playerColorAndID[0] = static_cast<unsigned char>( unNormalizedRed );
				victoryPacket.playerColorAndID[1] = static_cast<unsigned char>( unNormalizedGreen );
				victoryPacket.playerColorAndID[2] = static_cast<unsigned char>( unNormalizedBlue );
				victoryPacket.timestamp = cbutil::getCurrentTimeSeconds();
				victoryPacket.data.victorious.playerColorAndID[0] = static_cast<unsigned char>( unNormalizedRed );
				victoryPacket.data.victorious.playerColorAndID[1] = static_cast<unsigned char>( unNormalizedGreen );
				victoryPacket.data.victorious.playerColorAndID[2] = static_cast<unsigned char>( unNormalizedBlue );

				nwAgent->sendPlayerVictoryPacket( victoryPacket );
			}
		}
	}
}


void FeedbackWorld::computePlayerDesiredPosition( float deltaSeconds ) {

	m_player.updateDesiredPosition( deltaSeconds );
}


void FeedbackWorld::sendPlayerDesiredPosition( float deltaSeconds ) {

	cbengine::GameDirector* sharedGameDirector = cbengine::GameDirector::sharedDirector();

	FeedbackGame* fbGame = dynamic_cast<FeedbackGame*>( sharedGameDirector->getCurrentWorld() );

	if ( fbGame != nullptr ) {

		NetworkAgent* nwAgent = fbGame->getCurrentNetworkAgent();
		if ( nwAgent != nullptr ) {

			CS6Packet playerPacket;
			playerPacket.packetType = TYPE_Update;
			playerPacket.packetNumber = 0;
			float unNormalizedRed = cbengine::rangeMapFloat( 0.0f, 1.0f, 0.0f, 255.0f, m_player.m_playerColor.x );
			float unNormalizedGreen = cbengine::rangeMapFloat( 0.0f, 1.0f, 0.0f, 255.0f, m_player.m_playerColor.y );
			float unNormalizedBlue = cbengine::rangeMapFloat( 0.0f, 1.0f, 0.0f, 255.0f, m_player.m_playerColor.z );
			playerPacket.playerColorAndID[0] = static_cast<unsigned char>( unNormalizedRed );
			playerPacket.playerColorAndID[1] = static_cast<unsigned char>( unNormalizedGreen );
			playerPacket.playerColorAndID[2] = static_cast<unsigned char>( unNormalizedBlue );
			playerPacket.timestamp = cbutil::getCurrentTimeSeconds();
			playerPacket.data.updated.xPosition = m_player.m_desiredPosition.x;
			playerPacket.data.updated.yPosition = m_player.m_desiredPosition.y;
			playerPacket.data.updated.xVelocity = m_player.m_currentVelocity.x;
			playerPacket.data.updated.yVelocity = m_player.m_currentVelocity.y;
			playerPacket.data.updated.yawDegrees = m_player.m_orientationDegrees;

			nwAgent->sendPlayerDataPacketToServer( playerPacket );
		}
	}
}


void FeedbackWorld::getUpdatedGameDataFromNetworkAgent() {

	cbengine::GameDirector* sharedGameDirector = cbengine::GameDirector::sharedDirector();

	FeedbackGame* fbGame = dynamic_cast<FeedbackGame*>( sharedGameDirector->getCurrentWorld() );

	if ( fbGame != nullptr ) {

		NetworkAgent* nwAgent = fbGame->getCurrentNetworkAgent();
		if ( nwAgent != nullptr ) {

			CS6Packet playerPacket;
			bool packetIsAvailable = true;

			while ( packetIsAvailable ) {

				packetIsAvailable = nwAgent->getLatestGamePacketData( playerPacket );
				if ( packetIsAvailable ) {

					if ( playerPacket.packetType == TYPE_Update ) {

						float unNormalizedRed = cbengine::rangeMapFloat( 0.0f, 1.0f, 0.0f, 255.0f, m_player.m_playerColor.x );
						float unNormalizedGreen = cbengine::rangeMapFloat( 0.0f, 1.0f, 0.0f, 255.0f, m_player.m_playerColor.y );
						float unNormalizedBlue = cbengine::rangeMapFloat( 0.0f, 1.0f, 0.0f, 255.0f, m_player.m_playerColor.z );

						if ( playerPacket.playerColorAndID[0] == static_cast<unsigned char>( unNormalizedRed ) &&
							playerPacket.playerColorAndID[1] == static_cast<unsigned char>( unNormalizedGreen ) &&
							playerPacket.playerColorAndID[2] == static_cast<unsigned char>( unNormalizedBlue ) ) 
						{
							m_player.m_position.x = playerPacket.data.updated.xPosition;
							m_player.m_position.y = playerPacket.data.updated.yPosition;
							m_player.m_orientationDegrees = playerPacket.data.updated.yawDegrees;
							m_player.m_currentVelocity.x = playerPacket.data.updated.xVelocity;
							m_player.m_currentVelocity.y = playerPacket.data.updated.yVelocity;

						} else {

							bool playerMatchFound = false;
							for ( int i = 0; i < m_otherPlayers.size(); ++i ) {

								GameObject* otherPlayer = m_otherPlayers[i];

								float unNormalizedRed = cbengine::rangeMapFloat( 0.0f, 1.0f, 0.0f, 255.0f, otherPlayer->m_playerColor.x );
								float unNormalizedGreen = cbengine::rangeMapFloat( 0.0f, 1.0f, 0.0f, 255.0f, otherPlayer->m_playerColor.y );
								float unNormalizedBlue = cbengine::rangeMapFloat( 0.0f, 1.0f, 0.0f, 255.0f, otherPlayer->m_playerColor.z );

								if ( static_cast<unsigned char>( unNormalizedRed ) == playerPacket.playerColorAndID[0] &&
									static_cast<unsigned char>( unNormalizedGreen ) == playerPacket.playerColorAndID[1] &&
									static_cast<unsigned char>( unNormalizedBlue ) == playerPacket.playerColorAndID[2] ) 
								{
									playerMatchFound = true;

									otherPlayer->m_position.x = playerPacket.data.updated.xPosition;
									otherPlayer->m_position.y = playerPacket.data.updated.yPosition;
									otherPlayer->m_orientationDegrees = playerPacket.data.updated.yawDegrees;
									otherPlayer->m_currentVelocity.x = playerPacket.data.updated.xVelocity;
									otherPlayer->m_currentVelocity.y = playerPacket.data.updated.yVelocity;
								}
							}

							if ( !playerMatchFound ) {

								GameObject* newPlayerToAdd = new GameObject;

								newPlayerToAdd->m_playerColor.x = cbengine::rangeMapFloat( 0.0f, 255.0f, 0.0f, 1.0f, static_cast<float>( playerPacket.playerColorAndID[0] ) );
								newPlayerToAdd->m_playerColor.y = cbengine::rangeMapFloat( 0.0f, 255.0f, 0.0f, 1.0f, static_cast<float>( playerPacket.playerColorAndID[1] ) );
								newPlayerToAdd->m_playerColor.z = cbengine::rangeMapFloat( 0.0f, 255.0f, 0.0f, 1.0f, static_cast<float>( playerPacket.playerColorAndID[2] ) );
								newPlayerToAdd->m_playerColor.w = 1.0f;
								newPlayerToAdd->m_isFlag = false;


								// Initial Position, Velocity, and Orientation
								newPlayerToAdd->m_position.x = playerPacket.data.updated.xPosition;
								newPlayerToAdd->m_position.y = playerPacket.data.updated.yPosition;
								newPlayerToAdd->m_orientationDegrees = playerPacket.data.updated.yawDegrees;
								newPlayerToAdd->m_currentVelocity.x = playerPacket.data.updated.xVelocity;
								newPlayerToAdd->m_currentVelocity.y = playerPacket.data.updated.yVelocity;

								m_otherPlayers.push_back( newPlayerToAdd );
							}
						}
	
					} else if ( playerPacket.packetType == TYPE_Victory ) {
						
						CS6Packet ackPacketForVictory;
						ackPacketForVictory.packetNumber = playerPacket.packetNumber;
						ackPacketForVictory.timestamp = cbutil::getCurrentTimeSeconds();
						ackPacketForVictory.packetType = TYPE_Acknowledge;
						ackPacketForVictory.data.acknowledged.packetNumber = playerPacket.packetNumber;
						ackPacketForVictory.data.acknowledged.packetType = TYPE_Victory;

						nwAgent->sendAckPacket( ackPacketForVictory );
						
					} else if ( playerPacket.packetType == TYPE_Reset ) {

						m_player.m_position.x = playerPacket.data.reset.playerXPosition;
							m_player.m_position.y = playerPacket.data.reset.playerYPosition;
						m_flag.m_position.x = playerPacket.data.reset.flagXPosition;
						m_flag.m_position.y = playerPacket.data.reset.flagYPosition;

						m_player.m_playerColor.x = cbengine::rangeMapFloat( 0.0f, 255.0f, 0.0f, 1.0f, static_cast<float>( playerPacket.playerColorAndID[0] ) );
						m_player.m_playerColor.y = cbengine::rangeMapFloat( 0.0f, 255.0f, 0.0f, 1.0f, static_cast<float>( playerPacket.playerColorAndID[1] ) );
						m_player.m_playerColor.z = cbengine::rangeMapFloat( 0.0f, 255.0f, 0.0f, 1.0f, static_cast<float>( playerPacket.playerColorAndID[2] ) );

						CS6Packet ackPacketForReset;
						ackPacketForReset.packetNumber = playerPacket.packetNumber;
						ackPacketForReset.timestamp = cbutil::getCurrentTimeSeconds();
						ackPacketForReset.packetType = TYPE_Acknowledge;
						ackPacketForReset.data.acknowledged.packetNumber = playerPacket.packetNumber;
						ackPacketForReset.data.acknowledged.packetType = TYPE_Reset;

						nwAgent->sendAckPacket( ackPacketForReset );

					} else if ( playerPacket.packetType == TYPE_JoinLobby ) {

						for ( size_t i = 0; i < m_otherPlayers.size(); ++i ) {

							GameObject* player = m_otherPlayers[i];
							delete player;
							player = nullptr;
						}

						m_otherPlayers.clear();
						
						m_gameState = GAME_STATE_IN_LOBBY;
						return;
					}

				} // end packet is available
			}
		}
	}
}


void FeedbackWorld::render( float deltaSeconds ) const {

	MatrixStack* matrixStack = MatrixStack::sharedMatrixStack();
	matrixStack->emptyCurrentMatrixStackAndPushIdentityMatrix();
	matrixStack->createOrthoMatrixAndPushToStack( 0.0f, 
		static_cast<float>( feedback::ORTHO_SCREEN_WIDTH ), 
		0.0f, 
		static_cast<float>( feedback::ORTHO_SCREEN_HEIGHT ), 0.0f, 1.0f );


	m_camera2D.setUpCameraPositionForRendering();

	// TEST CODE
	if ( m_gameState == GAME_STATE_RUNNING ) {

		m_player.render( deltaSeconds );

		for ( int i = 0; i < m_otherPlayers.size(); ++i ) {

			GameObject* otherPlayer = m_otherPlayers[i];
			otherPlayer->render( deltaSeconds );
		}
		// END TEST CODE

		m_flag.render( deltaSeconds );

	} else if ( m_gameState == GAME_STATE_IN_LOBBY ) {

		glUseProgram(0);
		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity();
		matrixStack->emptyCurrentMatrixStackAndPushIdentityMatrix();

		matrixStack->createOrthoMatrixAndPushToStack( 0.0f, 
			( feedback::SCREEN_WIDTH ), 
			0.0f, 
			( feedback::SCREEN_HEIGHT ),
			0.0f,
			1.0f );

		const Matrix44<float> & topOfStack = matrixStack->getMatrixFromTopOfStack();
		glLoadMatrixf( topOfStack.matrixData );

		const std::string fontName = "MainFont";
		const std::string fontXMLFilePath = "Fonts/MainFont_EN.FontDef.xml";
		const std::string fontTextureFilePath = "Fonts/MainFont_EN_00.png";
		const float debugTextSize = 25.0f;
		const float dispositionIncrement = 30.0f;

		const float textXStartPos = feedback::HALF_SCREEN_WIDTH * 0.08f;
		const float textYStartPos = feedback::SCREEN_HEIGHT * 0.85f;

		float currentTextXPos = textXStartPos;
		float currentTextYPos = textYStartPos;

		Console* sharedDeveloperConsole = Console::getSharedDeveloperConsole();
		TextRenderer* sharedTextRenderer = TextRenderer::sharedTextRenderer();
		FontManager* sharedFontManager = FontManager::sharedFontManager();

		BitmapFont* textFont = sharedFontManager->loadBitmapFont( fontName, fontTextureFilePath, fontTextureFilePath ); // Will only load first time
	
		if ( textFont != nullptr ) {

			const std::string InLobbyString = "Game Lobby List of Game Rooms: ";
			cbengine::Vector2 positionForText( textXStartPos , textYStartPos );
			sharedTextRenderer->drawDebugText( InLobbyString, *(textFont), debugTextSize, positionForText );

			if ( m_numGamesFromLobby == 0 ) {

				currentTextYPos -= dispositionIncrement;
				std::string noGames = "There are currently no game rooms";
				cbengine::Vector2 noGamePos( currentTextXPos, currentTextYPos );

				sharedTextRenderer->drawDebugText( noGames, *(textFont), debugTextSize, noGamePos );
			}
	
			for ( int i = 0; i < m_numGamesFromLobby; ++i ) {

				int gameNumToDisplay = ( i + 1 );
				currentTextYPos -= dispositionIncrement;

				std::string gameNumToDisplayString = "GameNumber: ";
				std::string gameNum = std::to_string( (long double) gameNumToDisplay );
				gameNumToDisplayString += gameNum;

				cbengine::Vector2 gameNumPos( textXStartPos, currentTextYPos );

				sharedTextRenderer->drawDebugText( gameNumToDisplayString, *(textFont), debugTextSize, gameNumPos );
			} 
		}

		matrixStack->emptyCurrentMatrixStackAndPushIdentityMatrix();
	}
	

	matrixStack->emptyCurrentMatrixStackAndPushIdentityMatrix();
}


void FeedbackWorld::joinGame( unsigned int gameNumTojoin ) {

	if ( m_gameState == GAME_STATE_IN_LOBBY ) {

		if ( gameNumTojoin > m_numGamesFromLobby ) {

			return;
		}

		cbengine::GameDirector* sharedGameDirector = cbengine::GameDirector::sharedDirector();

		FeedbackGame* fbGame = dynamic_cast<FeedbackGame*>( sharedGameDirector->getCurrentWorld() );

		if ( fbGame != nullptr ) {

			NetworkAgent* nwAgent = fbGame->getCurrentNetworkAgent();
			if ( nwAgent != nullptr ) {

				CS6Packet joinPacket;
				joinPacket.packetType = TYPE_JoinGame;
				joinPacket.packetNumber = 0;
				joinPacket.timestamp = cbutil::getCurrentTimeSeconds();

				joinPacket.data.joinGame.gameNumToJoin = gameNumTojoin;

				nwAgent->sendPacket( joinPacket );
			}
		}
	}
}


void FeedbackWorld::createGame() {

	if ( m_gameState == GAME_STATE_IN_LOBBY ) {

		cbengine::GameDirector* sharedGameDirector = cbengine::GameDirector::sharedDirector();

		FeedbackGame* fbGame = dynamic_cast<FeedbackGame*>( sharedGameDirector->getCurrentWorld() );

		if ( fbGame != nullptr ) {

			NetworkAgent* nwAgent = fbGame->getCurrentNetworkAgent();
			if ( nwAgent != nullptr ) {

				CS6Packet createPacket;
				createPacket.packetType = TYPE_CreateGame;
				createPacket.packetNumber = 0;
				createPacket.timestamp = cbutil::getCurrentTimeSeconds();
				
				nwAgent->sendPacket( createPacket );
			}
		}
	}
}


void FeedbackWorld::processKeyboardInput( float deltaSeconds ) {

	UNUSED( deltaSeconds );

		// Handle Keyboard Input
	InputHandler& sharedInputHandler = InputHandler::getSharedInputHandler();
	const bool* virtualKeys = sharedInputHandler.getKeyboardStatus();

	if ( virtualKeys[ 'W' ] ) {
		m_player.m_currentVelocity.y += 1.0f;
	}

	if ( virtualKeys [ 'D' ] ) {
		m_player.m_currentVelocity.x += 1.0f;
	}

	if ( virtualKeys[ 'S' ] ) {
		m_player.m_currentVelocity.y += -1.0f;
	}

	if ( virtualKeys[ 'A' ] ) {
		m_player.m_currentVelocity.x += -1.0f;
	}

	if ( virtualKeys[ VK_LEFT ] ) {

		m_player.increaseOrientationDegrees( 3.50f );
	}
	//129.119.228.95
	//129.119.246.128

	if ( virtualKeys[ VK_RIGHT ] ) {

		m_player.increaseOrientationDegrees( -3.50f );
	}

	static bool cKeyDown = false;
	if ( virtualKeys[ 'C' ] ) {

		if ( !cKeyDown ) {
			
			createGame();
		}

		cKeyDown = true;

	} else {

		cKeyDown = false;
	}

	static bool oneKeyDown = false;
	if ( virtualKeys[ '1' ] ) {
		if ( !oneKeyDown ) {

			joinGame( 1 );
		}

		oneKeyDown = true;
	} else {
		oneKeyDown = false;
	}

	static bool twoKeyDown = false;
	if ( virtualKeys[ '2' ] ) {
		if ( !twoKeyDown ) {

			joinGame( 2 );
		}

		twoKeyDown = true;
	} else {
		twoKeyDown = false;
	}

	static bool threeKeyDown = false;
	if ( virtualKeys[ '3' ] ) {
		if ( !oneKeyDown ) {

			joinGame( 3 );
		}

		threeKeyDown = true;
	} else {
		threeKeyDown = false;
	}


	static bool fourKeyDown = false;
	if ( virtualKeys[ '4' ] ) {
		if ( !fourKeyDown ) {

			joinGame( 4 );
		}

		fourKeyDown = true;
	} else {
		fourKeyDown = false;
	}


	static bool fiveKeyDown = false;
	if ( virtualKeys[ '5' ] ) {
		if ( !fiveKeyDown ) {

			joinGame( 5 );
		}

		fiveKeyDown = true;
	} else {
		fiveKeyDown = false;
	}

	static bool sixKeyDown = false;
	if ( virtualKeys[ '6' ] ) {
		if ( !sixKeyDown ) {

			joinGame( 6 );
		}

		sixKeyDown = true;
	} else {
		sixKeyDown = false;
	}

	static bool sevenKeyDown = false;
	if ( virtualKeys[ '7' ] ) {
		if ( !sevenKeyDown ) {

			joinGame( 7 );
		}

		sevenKeyDown = true;
	} else {
		sevenKeyDown = false;
	}

	static bool eightKeyDown = false;
	if ( virtualKeys[ '8' ] ) {
		if ( !eightKeyDown ) {

			joinGame( 8 );
		}

		eightKeyDown = true;
	} else {
		eightKeyDown = false;
	}


	static bool nineKeyDown = false;
	if ( virtualKeys[ '9' ] ) {
		if ( !nineKeyDown ) {

			joinGame( 9 );
		}

		nineKeyDown = true;
	} else {
		nineKeyDown = false;
	}
}


void FeedbackWorld::setDefaultVariableValues() {

	m_camera2D.m_position.x = 0.0f;
	m_camera2D.m_position.y = 0.0f;
	m_gameState = GAME_STATE_WAITING_FOR_SERVER;
	m_numGamesFromLobby = 0;
}