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
	sendPlayerDesiredPosition( deltaSeconds );

	getUpdatedGameDataFromNetworkAgent();

	m_player.update( deltaSeconds );

	for ( int i = 0; i < m_otherPlayers.size(); ++i ) {

		GameObject* otherPlayer = m_otherPlayers[i];
		//otherPlayer->update( deltaSeconds );
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

			PlayerDataPacket playerPacket;
			playerPacket.m_playerID = m_player.m_playerID;
			playerPacket.m_xPos = m_player.m_desiredPosition.x;
			playerPacket.m_yPos = m_player.m_desiredPosition.y;
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

			PlayerDataPacket playerPacket;
			bool packetIsAvailable = true;

			while ( packetIsAvailable ) {

				packetIsAvailable = nwAgent->getLatestGamePacketData( playerPacket );
				if ( packetIsAvailable ) {

					if ( playerPacket.m_packetID == NEW_PLAYER_ACK_ID ) {

						m_player.m_playerColor.x = static_cast<float>( playerPacket.m_red );
						m_player.m_playerColor.y = static_cast<float>( playerPacket.m_green );
						m_player.m_playerColor.z = static_cast<float>( playerPacket.m_blue );

						m_player.m_playerColor.x = cbengine::rangeMapFloat( 0.0f, 255.0f, 0.0f, 1.0f, m_player.m_playerColor.x );
						m_player.m_playerColor.y = cbengine::rangeMapFloat( 0.0f, 255.0f, 0.0f, 1.0f, m_player.m_playerColor.y );
						m_player.m_playerColor.z = cbengine::rangeMapFloat( 0.0f, 255.0f, 0.0f, 1.0f, m_player.m_playerColor.z );

						m_player.m_playerColor.w = 1.0f;
						m_player.m_position.x = playerPacket.m_xPos;
						m_player.m_position.y = playerPacket.m_yPos;
						m_player.m_playerID = playerPacket.m_playerID;

					} else {

						if ( playerPacket.m_playerID == m_player.m_playerID ) {

							m_player.m_position.x = playerPacket.m_xPos;
							m_player.m_position.y = playerPacket.m_yPos;

						} else {

							bool playerFound = false;
							for ( int i = 0; i < m_otherPlayers.size(); ++i ) {

								GameObject* otherPlayer = m_otherPlayers[i];
								if ( playerPacket.m_playerID == otherPlayer->m_playerID ) {

									playerFound = true;
									otherPlayer->m_position.x = playerPacket.m_xPos;
									otherPlayer->m_position.y = playerPacket.m_yPos;
								}
							}

							if ( !playerFound ) {

								GameObject* newPlayerToAdd = new GameObject;
								newPlayerToAdd->m_position.x = playerPacket.m_xPos;
								newPlayerToAdd->m_position.y = playerPacket.m_yPos;
								newPlayerToAdd->m_playerID = playerPacket.m_playerID;
								newPlayerToAdd->m_playerColor.x = static_cast<float>( playerPacket.m_red );
								newPlayerToAdd->m_playerColor.y = static_cast<float>( playerPacket.m_green );
								newPlayerToAdd->m_playerColor.z = static_cast<float>( playerPacket.m_blue );

								newPlayerToAdd->m_playerColor.x = cbengine::rangeMapFloat( 0.0f, 255.0f, 0.0f, 1.0f, newPlayerToAdd->m_playerColor.x );
								newPlayerToAdd->m_playerColor.y = cbengine::rangeMapFloat( 0.0f, 255.0f, 0.0f, 1.0f, newPlayerToAdd->m_playerColor.y );
								newPlayerToAdd->m_playerColor.z = cbengine::rangeMapFloat( 0.0f, 255.0f, 0.0f, 1.0f, newPlayerToAdd->m_playerColor.z );

								newPlayerToAdd->m_playerColor.w = 1.0f;

								m_otherPlayers.push_back( newPlayerToAdd );
							}
						}
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
	m_player.render( deltaSeconds );

	for ( int i = 0; i < m_otherPlayers.size(); ++i ) {

		GameObject* otherPlayer = m_otherPlayers[i];
		otherPlayer->render( deltaSeconds );
	}
	// END TEST CODE

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
}