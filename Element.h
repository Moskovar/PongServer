#pragma once
#include <GLM/glm.hpp>
#include "uti.h"
#include <iostream>

class Element
{
	public:
		Element() {}
		Element(glm::vec3 position);

		float getX() { return position.x; }
		float getZ() { return position.z; }

		uti::OBB& getRHitbox() { return hitbox; }

		void setZ(float z);
		void setPosition(glm::vec3 position);
		void updateModelMatrixFromPosition();
		void setMinMaxPoints(glm::vec3 minPoint, glm::vec3 maxPoint);

	protected:
		glm::vec3	position = glm::vec3(0.0f, 0.0f, 0.0f),
					minPoint = glm::vec3(-1.0f, -1.0f, -5.1f),
					maxPoint = glm::vec3(1.0f, 1.0f, 5.1f);
		//------------------//

		glm::mat4 modelMatrix = glm::mat4(1.0f);
		uti::OBB hitbox;

		void calculateHitBox();
};

