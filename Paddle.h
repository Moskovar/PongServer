#pragma once
#include "Element.h"
#include <mutex>

class Paddle : public Element
{
	public:
		Paddle() {}
		Paddle(glm::vec3 position);

		glm::vec3 getPosition() override;

		void setZ(float z) override;


		std::mutex mtx_position;
		//--- Hard value ---//
		float width = 10.2f;

	private:
};

