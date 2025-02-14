#pragma once
#include "Element.h"

class Ball : public Element
{
	public:
		Ball(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f));

		uti::NetworkBall getNball();

		void turnback();
		void move(float deltaTime);

		float velocityX = 1.0f, velocityZ = 0.0f;

		float moveSpeed = 50.0f;

	private:
};

