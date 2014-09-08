#include "FeedbackWorld.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/gl.h> // ugh get rid of this dependency for rendering debug axis
#include <string>

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

	processKeyboardInput( deltaSeconds );
	computePlayerDesiredPosition( deltaSeconds );

	if ( m_gameState == GAME_STATE_RUNNING ) {

		sendPlayerDesiredPosition( deltaSeconds );

		getUpdatedGameDataFromNetworkAgent();

		m_player.update( deltaSeconds );

		for ( int i = 0; i < m_otherPlayers.size(); ++i ) {

			GameObject* otherPlayer = m_otherPlayers[i];
		}

		m_flag.update( deltaSeconds );

	} else if ( m_gameState == GAME_STATE_WAITING_FOR_SERVER ) {

		attemptToConnectToServer( deltaSeconds );

	}
	
}


void FeedbackWorld::attemptToConnectToServer( float deltaSeconds ) {

	cbengine::GameDirector* sharedGameDirector = cbengine::GameDirector::sharedDirector();

	FeedbackGame* fbGame = dynamic_cast<FeedbackGame*>( sharedGameDirector->getCurrentWorld() );

	if ( fbGame != nullptr ) {

		NetworkAgent* nwAgent = fbGame->getCurrentNetworkAgent();
		if ( nwAgent != nullptr ) {

			CS6Packet resetPacket;
			bool resetPacketReceived = nwAgent->requestToJoinServer( deltaSeconds, resetPacket );
			if ( resetPacketReceived ) {

				// Player
				m_player.m_playerColor.x = static_cast<float>( resetPacket.data.reset.playerColorAndID[0] );
				m_player.m_playerColor.y = static_cast<float>( resetPacket.data.reset.playerColorAndID[1] );
				m_player.m_playerColor.z = static_cast<float>( resetPacket.data.reset.playerColorAndID[2] );
				m_player.m_position.x = resetPacket.data.reset.playerXPosition;
				m_player.m_position.y = resetPacket.data.reset.playerYPosition;
				m_player.m_desiredPosition = m_player.m_position;
				m_player.m_currentVelocity.x = 0.0f;
				m_player.m_currentVelocity.y = 0.0f;
				m_player.m_orientationDegrees = 0.0f;
				m_player.m_isFlag = false;

				// Flag
				m_flag.m_orientationDegrees = 0.0f;
				m_flag.m_position.x = resetPacket.data.reset.flagXPosition;
				m_flag.m_position.y = resetPacket.data.reset.flagYPosition;
				m_flag.m_playerColor.x = 0.50f;
				m_flag.m_playerColor.y = 0.55f;
				m_flag.m_playerColor.z = 0.30f;
				m_flag.m_playerColor.w = 1.0f;

				m_gameState = GAME_STATE_RUNNING;
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
			playerPacket.playerColorAndID[0] = static_cast<unsigned char>( m_player.m_playerColor.x );
			playerPacket.playerColorAndID[1] = static_cast<unsigned char>( m_player.m_playerColor.y );
			playerPacket.playerColorAndID[2] = static_cast<unsigned char>( m_player.m_playerColor.z );
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

						if ( playerPacket.playerColorAndID[0] == static_cast<unsigned char>( m_player.m_playerColor.x ) &&
							playerPacket.playerColorAndID[1] == static_cast<unsigned char>( m_player.m_playerColor.y ) &&
							playerPacket.playerColorAndID[2] == static_cast<unsigned char>( m_player.m_playerColor.z ) ) 
						{
							m_player.m_position.x = playerPacket.data.updated.xPosition;
							m_player.m_position.y = playerPacket.data.updated.yPosition;
							m_player.m_orientationDegrees = playerPacket.data.updated.yawDegrees;
							m_player.m_currentVelocity.x = playerPacket.data.updated.xVelocity;
							m_player.m_currentVelocity.y = playerPacket.data.updated.xVelocity;

						} else {

							bool playerMatchFound = false;
							for ( int i = 0; i < m_otherPlayers.size(); ++i ) {

								GameObject* otherPlayer = m_otherPlayers[i];
								if ( static_cast<unsigned char>( otherPlayer->m_playerColor.x ) == playerPacket.playerColorAndID[0] &&
									static_cast<unsigned char>( otherPlayer->m_playerColor.y ) == playerPacket.playerColorAndID[1] &&
									static_cast<unsigned char>( otherPlayer->m_playerColor.z ) == playerPacket.playerColorAndID[2] ) 
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
	
					} else {

						
					}
				}
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
	}
	

	matrixStack->emptyCurrentMatrixStackAndPushIdentityMatrix();
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

}


void FeedbackWorld::setDefaultVariableValues() {

	m_camera2D.m_position.x = 0.0f;
	m_camera2D.m_position.y = 0.0f;
	m_gameState = GAME_STATE_WAITING_FOR_SERVER;
}