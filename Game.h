#pragma once
#include "Player.h"
#include "Ball.h"
#include "Wall.h"
#include <mutex>

class Game
{
	public:
		Game() {}
		Game(unsigned short id) { this->gameID = gameID; }
		Game(short gameID, Player* p1, Player* p2, std::vector<Wall>* walls);
		~Game();

		void reset();
		void set(short gameID, Player* p1, Player* p2, std::vector<Wall>* walls);

		Player* getOtherPlayer(short id);
		Player* getP1()					{ return p1;						}
		Player* getP2()					{ return p2;						}
		bool isFinished()				{ return allPlayersDisconnected();	}
		bool isAvailableInPool()		{ return availableInPool;			}
		bool allPlayersDisconnected();
		bool allPlayersLeftGame();

		void resetBall();
		void sendBallToPlayersTCP();
		void sendBallToPlayersUDP(SOCKET& udpSocket);
		void startRound(SOCKET& udpSocket);
		void resetRound();
		void increaseBallSpeed();
		void run(SOCKET& udpSocket, float deltaTime);

		std::mutex mtx;
		u_int64 game_created_time = 0, round_start_time = 0, ballSpeed_increase = 0;
		bool availableInPool = true, roundStarted = false;

		SOCKET* udpSocket = nullptr;

	private:
		unsigned short gameID = 0, id = 0;
		Player* p1 = nullptr, * p2 = nullptr, * lastWinner = nullptr;
		Ball ball;
		std::vector<Wall>* walls = nullptr;

		float distanceBetweenHitboxes(Element* e1, Element* e2);
};
