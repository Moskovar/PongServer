#pragma once
#include "Player.h"
#include "Ball.h"
#include "Wall.h"
#include <mutex>

class Game
{
	public:
		Game() {}
		~Game();

		void reset();
		bool set(short gameID, Player* p1, Player* p2, std::vector<Wall>* walls);

		Player* getOtherPlayer(short id);
		Player* getP1()					{ return p1;						}
		Player* getP2()					{ return p2;						}

		bool allPlayersDisconnected();
		bool allPlayersLeftGame();

		void resetBall();
		void sendBallToPlayersTCP();
		void sendBallToPlayersUDP(SOCKET& udpSocket, uint32_t elapsedTime);
		void startRound(SOCKET& udpSocket, uint32_t elapsedTime);
		void resetRound();
		void increaseBallSpeed();
		void run(SOCKET& udpSocket, float deltaTime, uint32_t elapsedTime);

		SOCKET* udpSocket = nullptr;
		//--- Gestion des états et du temps ---//
		std::atomic<bool> availableInPool{ true }, roundStarted{ false };
		u_int64 game_created_time = 0, round_start_time	= 0, ballSpeed_increase_time = 0;//actuellement uniquement utilisé dans la classe

	private:
		//std::mutex mtx_states;

		//--- Gestion de la balle ---//
		Ball ball;//utilisée uniquement dans le thread de logique de jeu, no need mutex ||--> on fait des reset ailleurs ?!

		unsigned short gameID = 0, id = 0;
		Player* p1 = nullptr, * p2 = nullptr, * lastWinner = nullptr;
		std::vector<Wall>* walls = nullptr;
		

		float distanceBetweenHitboxes(Element* e1, Element* e2);
};
