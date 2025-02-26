#include "Element.h"

Element::Element(glm::vec3 position)
{
	this->position = position;

	updateModelMatrixFromPosition();
}

void Element::setZ(float z)
{
	std::cout << "new Z : " << z << std::endl;
	this->position.z = z;
	updateModelMatrixFromPosition();

	//std::cout << "Nouvelle pos: " << position.x << " : " << position.y << " : " << position.z << std::endl;
}

void Element::setPosition(glm::vec3 position)
{
	this->position = position;
	updateModelMatrixFromPosition();
}

void Element::updateModelMatrixFromPosition()
{
	this->modelMatrix[3].x = this->position.x;
	this->modelMatrix[3].y = this->position.y;
	this->modelMatrix[3].z = this->position.z;

	calculateHitBox();
}

void Element::updateModelMatrixFromPosition(glm::vec3 position)
{
	this->modelMatrix[3].x = position.x;
	this->modelMatrix[3].y = position.y;
	this->modelMatrix[3].z = position.z;

	calculateHitBox();
}

void Element::setMinMaxPoints(glm::vec3 minPoint, glm::vec3 maxPoint)
{
	this->minPoint = minPoint;
	this->maxPoint = maxPoint;

	calculateHitBox();
}

void Element::calculateHitBox()
{
	// Mettre à jour le centre de l'OBB
	hitbox.center = this->position;

	// Extraire l'orientation depuis la matrice de modèle
	hitbox.orientation = glm::mat3(this->modelMatrix);

	// Calculer l'échelle à partir des colonnes de la matrice de modèle
	glm::vec3 scale(
		glm::length(this->modelMatrix[0]),  // Échelle sur l'axe X
		glm::length(this->modelMatrix[1]),  // Échelle sur l'axe Y
		glm::length(this->modelMatrix[2])   // Échelle sur l'axe Z
	);

	// Calculer halfSize en prenant en compte l'échelle
	hitbox.halfSize = (maxPoint - minPoint) * 0.5f * scale;

	//std::cout << hitbox.orientation[0].x << " : " << hitbox.orientation[0].y << " : " << hitbox.orientation[0].z << std::endl;
	//std::cout << hitbox.orientation[1].x << " : " << hitbox.orientation[1].y << " : " << hitbox.orientation[1].z << std::endl;
	//std::cout << hitbox.orientation[2].x << " : " << hitbox.orientation[2].y << " : " << hitbox.orientation[2].z << std::endl;

	hitbox.updateBounds();
}