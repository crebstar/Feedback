#include "GameObject.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/gl.h> // ugh get rid of this dependency 

#include "../../CBEngine/EngineCode/Vector3D.hpp"
#include "../../CBEngine/EngineCode/MatrixStack.hpp"
#include "../../CBEngine/EngineCode/Matrix44.hpp"

GameObject::~GameObject() {

}


GameObject::GameObject() {

	setGameObjectDefaults();

	m_position.x = 100.0f;
	m_position.y = 100.0f;
}


void GameObject::update( float deltaSeconds ) {

	updatePhysics( deltaSeconds );
}


void GameObject::updatePhysics( float deltaSeconds ) {

	m_currentVelocity.x = m_currentVelocity.x * MAX_VELOCITY_PER_SECOND;
	m_currentVelocity.y = m_currentVelocity.y * MAX_VELOCITY_PER_SECOND;

	m_position.x = m_position.x + ( m_currentVelocity.x * deltaSeconds );
	m_position.y = m_position.y + ( m_currentVelocity.y * deltaSeconds );

	m_currentVelocity.x = 0.0f;
	m_currentVelocity.y = 0.0f;
}


void GameObject::updateDesiredPosition( float deltaSeconds ) {

	m_currentVelocity.x = m_currentVelocity.x * MAX_VELOCITY_PER_SECOND;
	m_currentVelocity.y = m_currentVelocity.y * MAX_VELOCITY_PER_SECOND;

	m_desiredPosition.x = m_desiredPosition.x + ( m_currentVelocity.x * deltaSeconds );
	m_desiredPosition.y = m_desiredPosition.y + ( m_currentVelocity.y * deltaSeconds );

	m_currentVelocity.x = 0.0f;
	m_currentVelocity.y = 0.0f;
}


void GameObject::render( float deltaSeconds ) const {

	UNUSED( deltaSeconds );

	glUseProgram(0);
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	MatrixStack* matrixStack = MatrixStack::sharedMatrixStack();
	cbengine::Vector3<float> translationToPosition( m_position.x, m_position.y, 0.0f );
	matrixStack->applyTranslationAndPushToStack( translationToPosition );
	float orientationRadians = cbengine::degreesToRadians( 0.0f );
	matrixStack->applyRotationAboutZAndPushToStack( orientationRadians );
	const Matrix44<float> & topOfStack = matrixStack->getMatrixFromTopOfStack();
	glLoadMatrixf( topOfStack.matrixData );

	glDisable( GL_TEXTURE_2D );

	glColor3f( m_playerColor.x, m_playerColor.y, m_playerColor.z );

	const float testSize = 25.0f;

	glBegin( GL_QUADS ); {

		glVertex2f( -m_objectSize.m_width, -m_objectSize.m_height );

		glVertex2f( m_objectSize.m_width, -m_objectSize.m_height );

		glVertex2f( m_objectSize.m_width, m_objectSize.m_height );

		glVertex2f( -m_objectSize.m_width, m_objectSize.m_height );

	} glEnd();


	matrixStack->popFromTopOfStack();
	matrixStack->popFromTopOfStack();

}


void GameObject::setGameObjectDefaults() {

	m_orientationDegrees = 0.0f;
	m_objectSize.m_width = 20.0f;
	m_objectSize.m_height = 20.0f;
	m_playerID = -1;
}


