#include "Paddle.h"
#include <iostream>

Paddle::Paddle(glm::vec3 position) : Element(position)
{
	setMinMaxPoints(glm::vec3(-1.0f, -1.0f, -5.1f), glm::vec3(1.0f, 1.0f, 5.1f));
}

glm::vec3 Paddle::getPosition()
{
	std::lock_guard<std::mutex> lock(mtx_position);
	return position;
}

void Paddle::setZ(float z)
{
	{
		std::lock_guard<std::mutex> lock(mtx_position);
		this->position.z = z;
	}
}