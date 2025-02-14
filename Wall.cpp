#include "Wall.h"

Wall::Wall(glm::vec3 position) : Element(position)
{
	setMinMaxPoints(glm::vec3(-74.9124f, -1.0f, -1.0f), glm::vec3(74.9124f, 1.0f, 1.0f));
}
