#pragma once
#include "Element.h"
#include <mutex>

#define MAX_MOVESPEED 100

class Ball : public Element
{
	public:
		Ball(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f));

		uti::NetworkBall getNball();
		float getVelocityX();
		float getVelocityZ();
		short getMoveSpeed();
		uint32_t getTimestamp();

		void setVelocityX(float vx);
		void setVelocityZ(float vz);
		void setMoveSpeed(short ms);
		void setTimestamp();

		void turnback();
		void move(float deltaTime);
		void reset();
		void start(short direction);
		bool increaseMoveSpeed();

		
		Element* lastElementHit = nullptr;

	private:
		float velocityX = 1.0f, velocityZ = 0.0f;

		short moveSpeed = 25;

		uint32_t timestamp = 0;
};

