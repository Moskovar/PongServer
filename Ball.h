#pragma once
#include "Element.h"

#define MAX_MOVESPEED 100

class Ball : public Element
{
	public:
		Ball(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f));

		uti::NetworkBall getNball();

		void turnback();
		void move(float deltaTime);
		void reset();
		void start(short direction);
		bool increaseMoveSpeed();

		float velocityX = 1.0f, velocityZ = 0.0f;

		short moveSpeed = 25;

		uint32_t timestamp = 0;

		Element* lastElementHit = nullptr;

	private:
};

