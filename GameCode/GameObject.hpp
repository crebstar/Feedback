#ifndef included_GameObject
#define included_GameObject
#pragma once

#include "../../CBEngine/EngineCode/EngineMacros.hpp"

#include "../../CBEngine/EngineCode/Vector2.hpp"
#include "../../CBEngine/EngineCode/Vector4.hpp"
#include "../../CBEngine/EngineCode/Size.hpp"

// GetHostByName()

const float MAX_VELOCITY_PER_SECOND = 100.0f; 

class GameObject {
public:
	friend class FeedbackWorld;

	virtual ~GameObject();
	explicit GameObject();

	virtual void update( float deltaSeconds );
	virtual void render( float deltaSeconds ) const;

	cbengine::Vector2			m_position;
	cbengine::Vector2			m_currentVelocity;
	float						m_orientationDegrees;

	// Networking related
	cbengine::Vector2			m_desiredPosition;
	cbengine::Vector4<float>	m_playerColor;

	cbengine::Size				m_objectSize;
	int							m_playerID;
	bool						m_isFlag;

protected:

	void updatePhysics( float deltaSeconds );
	void updateDesiredPosition( float deltaSeconds );

	void setGameObjectDefaults();

private:
	PREVENT_COPY_AND_ASSIGN( GameObject );

};


#endif