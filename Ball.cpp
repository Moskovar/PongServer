#include "Ball.h"

Ball::Ball(glm::vec3 position) : Element(position)
{
	minPoint = glm::vec3(-1.0f, -1.0f, -1.0f);
	maxPoint = glm::vec3( 1.0f,  1.0f,  1.0f);

	updateModelMatrixFromPosition();
}

uti::NetworkBall Ball::getNball()
{
	uti::NetworkBall nball;
	nball.x			= position.x * 1000;
	nball.z			= position.z * 1000;
	nball.velocityX = velocityX  * 1000;
	nball.velocityZ = velocityZ  * 1000;

	return nball;
}

void Ball::turnback()
{
	velocityX *= -1.0f;
}

void Ball::move(float deltaTime)
{
	position.x += moveSpeed * deltaTime * velocityX;
	position.z += moveSpeed * deltaTime * velocityZ;

	updateModelMatrixFromPosition();
}
