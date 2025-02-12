#pragma once
#include "Element.h"

class Wall : public Element
{
	public:
		Wall(glm::vec3 position, glm::vec3 minPoint, glm::vec3 maxPoint);
};

