#include "Ball.h"

Ball::Ball(glm::vec3 position) : Element(position)
{
	setMinMaxPoints(glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(1.0f, 1.0f, 1.0f));
}

uti::NetworkBall Ball::getNball()
{
	uti::NetworkBall nball;

	nball.x			= position.x * 1000;
	nball.z			= position.z * 1000;
	nball.velocityX = velocityX * 1000;
	nball.velocityZ = velocityZ * 1000;
	nball.speed		= moveSpeed;
	nball.timestamp = uti::getCurrentTimestamp();

	return nball;
}

float Ball::getVelocityX()
{
	return velocityX;
}

float Ball::getVelocityZ()
{
	return velocityZ;
}

short Ball::getMoveSpeed()
{
	return moveSpeed;
}

uint32_t Ball::getTimestamp()
{
	return timestamp;
}

void Ball::setVelocityX(float vx)
{
	velocityX = vx;
}

void Ball::setVelocityZ(float vz)
{
	velocityZ = vz;
}

void Ball::setMoveSpeed(short ms)
{
	moveSpeed = ms;
}

void Ball::setTimestamp()
{
	timestamp = uti::getCurrentTimestampMs();
}

void Ball::turnback()
{
	velocityX *= -1.0f;
}

void Ball::move(float deltaTime)
{
	timestamp = uti::getCurrentTimestampMs();

	position.x += moveSpeed * deltaTime * velocityX;
	position.z += moveSpeed * deltaTime * velocityZ;

	updateModelMatrixFromPosition();
}

void Ball::reset()
{
	position	= glm::vec3(0.0f, 0.0f, 0.0f);
	velocityX	= 0.0f;
	velocityZ	= 0.0f;

	//moveSpeed	= 50.0f;
}

void Ball::start(short direction)//-1 gauche, 0 par défaut, 1 droite
{
	if (direction != 0) velocityX = 1.0f * direction;
	else				velocityX = 1.0f;
}

bool Ball::increaseMoveSpeed()
{
	if (moveSpeed < MAX_MOVESPEED)
	{
		++moveSpeed;
		return true;
	}

	return false;
}
