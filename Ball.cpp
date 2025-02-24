#include "Ball.h"

Ball::Ball(glm::vec3 position) : Element(position)
{
	setMinMaxPoints(glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(1.0f, 1.0f, 1.0f));
}

uti::NetworkBall Ball::getNball()
{
	uti::NetworkBall nball;

	{
		std::lock_guard<std::mutex> lock(mtx_ball);

		nball.x = position.x * 1000;
		nball.z = position.z * 1000;
		nball.velocityX = velocityX * 1000;
		nball.velocityZ = velocityZ * 1000;
		nball.speed = moveSpeed;
	}

	nball.timestamp = uti::getCurrentTimestamp();

	return nball;
}

float Ball::getVelocityX()
{
	std::lock_guard<std::mutex> lock(mtx_ball);
	return velocityX;
}

float Ball::getVelocityZ()
{
	std::lock_guard<std::mutex> lock(mtx_ball);
	return velocityZ;
}

short Ball::getMoveSpeed()
{
	std::lock_guard<std::mutex> lock(mtx_ball);
	return moveSpeed;
}

uint32_t Ball::getTimestamp()
{
	std::lock_guard<std::mutex> lock(mtx_ball);
	return timestamp;
}

void Ball::setVelocityX(float vx)
{
	std::lock_guard<std::mutex> lock(mtx_ball);
	velocityX = vx;
}

void Ball::setVelocityZ(float vz)
{
	std::lock_guard<std::mutex> lock(mtx_ball);
	velocityZ = vz;
}

void Ball::setMoveSpeed(short ms)
{
	std::lock_guard<std::mutex> lock(mtx_ball);
	moveSpeed = ms;
}

void Ball::setTimestamp()
{
	std::lock_guard<std::mutex> lock(mtx_ball);
	timestamp = uti::getCurrentTimestampMs();
}

void Ball::turnback()
{
	std::lock_guard<std::mutex> lock(mtx_ball);
	velocityX *= -1.0f;
}

void Ball::move(float deltaTime)
{
	std::lock_guard<std::mutex> lock(mtx_ball);
	timestamp = uti::getCurrentTimestampMs();

	position.x += moveSpeed * deltaTime * velocityX;
	position.z += moveSpeed * deltaTime * velocityZ;

	updateModelMatrixFromPosition();
}

void Ball::reset()
{
	std::lock_guard<std::mutex> lock(mtx_ball);
	position	= glm::vec3(0.0f, 0.0f, 0.0f);
	velocityX	= 0.0f;
	velocityZ	= 0.0f;

	//moveSpeed	= 50.0f;
}

void Ball::start(short direction)//-1 gauche, 0 par défaut, 1 droite
{
	std::lock_guard<std::mutex> lock(mtx_ball);
	if (direction != 0) velocityX = 1.0f * direction;
	else				velocityX = 1.0f;
}

bool Ball::increaseMoveSpeed()
{
	std::lock_guard<std::mutex> lock(mtx_ball);
	if (moveSpeed < MAX_MOVESPEED)
	{
		++moveSpeed;
		return true;
	}

	return false;
}
