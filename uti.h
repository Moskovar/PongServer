#pragma once
#define NOMINMAX  // Désactive la définition des macros min et max dans Windows.h
#define SDL_MAIN_HANDLED
#include <glm/glm.hpp>
#include <chrono>
#include <map>
#include <string>
#include <cmath>
#include <array>

using namespace std;

namespace uti {
	enum Header {
		NE = 0,
		NES = 1,
		NESE = 2,
		NEF = 3,
		NET = 4,
		NPS,
		NP,
		BALL
	};

	struct MoveRate {//utile ??
		float xRate, yRate;
	};

#pragma pack(push, 1)
	struct NetworkPaddleStart 
	{
		short header = Header::NPS;
		short gameID = 0, id = 0, side = -1;
	};
#pragma pack(pop)

#pragma pack(push, 1)
	struct NetworkPaddle
	{
		short header = Header::NP;
		short gameID = 0, id = 0;
		int z = 0, timestamp = -1;
	};
#pragma pack(pop)

#pragma pack(push, 1)
	struct NetworkBall
	{
		short header = Header::BALL;
		int x = 0, z = 0, velocityX = 0, velocityZ = 0, timestamp = -1;
	};
#pragma pack(pop)

	uint64_t getCurrentTimestamp();

	extern map<int, map<int, string>> categories;


	struct OBB
	{
		glm::vec3 center;  // Position centrale de la hitbox
		glm::mat3 orientation;  // Matrice 3x3 définissant les axes locaux (orientations)
		glm::vec3 halfSize;  // Moitié de la taille sur chaque axe
		glm::vec3 minPoint;
		glm::vec3 maxPoint;

		void updateBounds()
		{
			glm::vec3 localMin = -halfSize;
			glm::vec3 localMax = halfSize;

			// Calculer les coins transformés (appliquer l'orientation sur chaque axe)
			glm::vec3 xAxis = orientation[0] * halfSize.x;
			glm::vec3 yAxis = orientation[1] * halfSize.y;
			glm::vec3 zAxis = orientation[2] * halfSize.z;

			// Calculer les 8 coins de l'OBB dans l'espace monde
			std::array<glm::vec3, 8> corners =
			{
				center - xAxis - yAxis - zAxis,  // Coin inférieur arrière gauche
				center + xAxis - yAxis - zAxis,  // Coin inférieur arrière droit
				center - xAxis + yAxis - zAxis,  // Coin supérieur arrière gauche
				center + xAxis + yAxis - zAxis,  // Coin supérieur arrière droit
				center - xAxis - yAxis + zAxis,  // Coin inférieur avant gauche
				center + xAxis - yAxis + zAxis,  // Coin inférieur avant droit
				center - xAxis + yAxis + zAxis,  // Coin supérieur avant gauche
				center + xAxis + yAxis + zAxis   // Coin supérieur avant droit
			};

			// Initialiser minPoint et maxPoint à la première valeur
			minPoint = corners[0];
			maxPoint = corners[0];

			// Trouver les min et max sur chaque axe
			for (const auto& corner : corners)
			{
				minPoint = glm::min(minPoint, corner);  // Trouve les composantes minimales
				maxPoint = glm::max(maxPoint, corner);  // Trouve les composantes maximales
			}

			//std::cout << "MP: " << maxPoint.x << " : " << maxPoint.y << " : " << maxPoint.z << std::endl;
			//std::cout << "mp: " << minPoint.x << " : " << minPoint.y << " : " << minPoint.z << std::endl;
		}
	};
}
