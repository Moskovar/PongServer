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

		//--- Gestion des états ---//

		bool isAvailableInPool();
		bool isRoundStarted();

		void setAvailableInPool(bool state);
		void setRoundStarted(bool state);
		
		//--- Gestion du temps ---//

		u_int64 getGame_created_time();
		u_int64 getRound_start_time();
		u_int64 getBallSpeed_increase_time();

		void setGame_created_time(u_int64 time);
		void setRound_start_time(u_int64 time);
		void setBallSpeed_increase_time(u_int64 time);


		bool allPlayersDisconnected();
		bool allPlayersLeftGame();

		void resetBall();
		void sendBallToPlayersTCP();
		void sendBallToPlayersUDP(SOCKET& udpSocket);
		void startRound(SOCKET& udpSocket);
		void resetRound();
		void increaseBallSpeed();
		void run(SOCKET& udpSocket, float deltaTime);

		
		

		SOCKET* udpSocket = nullptr;

	private:
		//--- Gestion des états et du temps ---//
		std::mutex mtx_states;
		bool availableInPool = true, roundStarted = false;
		u_int64 game_created_time = 0, round_start_time	= 0, ballSpeed_increase = 0;

		//--- Gestion de la balle ---//
		Ball ball;//utilisée uniquement dans le thread de logique de jeu, no need mutex ||-> on fait des reset ailleurs ?!

		unsigned short gameID = 0, id = 0;
		Player* p1 = nullptr, * p2 = nullptr, * lastWinner = nullptr;
		std::vector<Wall>* walls = nullptr;
		

		float distanceBetweenHitboxes(Element* e1, Element* e2);
};
