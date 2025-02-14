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
		Player* getP1() { return p1; }
		Player* getP2() { return p2; }
		bool isFinished() { return !p1->connected && !p2->connected; }
		bool allPlayersDisconnected() { return !p1->connected && !p2->connected; }

		void sendBallToPlayers();
		void run(SOCKET& udpSocket, float deltaTime);

		std::mutex mtx;
		u_int64 game_created_time = 0, game_start_time = 0;
		bool gameStarted = false;

	private:
		unsigned short gameID = 0, id = 0;
		Player* p1 = nullptr, * p2 = nullptr;
		Ball ball;
		std::vector<Wall>* walls = nullptr;

		float distanceBetweenHitboxes(Element* e1, Element* e2);
};
