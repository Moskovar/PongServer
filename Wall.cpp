#include "Wall.h"

Wall::Wall(glm::vec3 position, glm::vec3 minPoint, glm::vec3 maxPoint) : Element(position)
{
	this->minPoint = minPoint;
	this->maxPoint = maxPoint;
}
