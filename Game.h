#pragma once
#include "Player.h"
#include "Ball.h"
#include "Wall.h"
#include <mutex>

class Game
{
	public:
		Game(short gameID, Player* p1, Player* p2, std::vector<Wall>* walls);
		~Game();

		Player* getOtherPlayer(short id);

		void run(SOCKET& udpSocket, float deltaTime);

		std::mutex mtx;

	private:
		unsigned short gameID = 0, id = 0;
		Player* p1 = nullptr, * p2 = nullptr;
		Ball ball;
		std::vector<Wall>* walls = nullptr;

		float distanceBetweenHitboxes(Element* e1, Element* e2);
};
