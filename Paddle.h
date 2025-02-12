#pragma once
#include "Element.h"

class Paddle : public Element
{
	public:
		Paddle(glm::vec3 position);

		//--- Hard value ---//
		float width = 10.2f;

	private:
};

