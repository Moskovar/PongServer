#pragma once
#include <winsock2.h>
#include <iostream>
#include <vector>
#include <chrono>
#include "uti.h"
#include "Paddle.h"
#include "Ball.h"
#include <mutex>
#include <atomic>
#include <shared_mutex>
//#include <ws2tcpip.h>

using namespace std;

class Player
{
public:
	Player() {}
	Player(short id) { this->id = id; }
	Player(SOCKET* tcpSocket);
	~Player();

	void setPlayer(SOCKET* tcpSocket);
	void resetPlayer();
	bool isAvailableInPool() { return availableInPool; }//si le joueur dispo dans la pool

	//--- Gestion des états ---//
	short getID();
	short getGameID();
	short getSide();

	void setID(short id);
	void setGameID(short gameID);
	void setSide(short side);//utilisé quand la game est créée, on créée le paddle également
	void setStates();
	void resetStates();

	uti::NetworkPaddleStart getNps()		{ return { uti::Header::NPS, gameID, id, side };						}
	uti::NetworkPaddle		getNp()			{ return { uti::Header::NP , gameID, id, (int)(paddle.getZ() * 1000)};  }//protéger le getZ

	//--- Setters ---//
	void setAddr(sockaddr_in addr);//uniquement utilsé dans listen_udpSocket
	sockaddr_in* getPAddr();
	int& getAddrLen();
	
	//--- Lien entre la communication et le joueur ---//

	//Paddle protégé par mtx_paddle
	void update(uti::NetworkPaddle& np);
	void updatePaddleModelMatrix(glm::vec3 position);

	//--- communication ---//
	SOCKET* getTCPSocket();
	bool isSocketValid();
	void closeSocket();
	float getPaddleWidth() { return paddleWidth; }
	//Paddle&  getPaddle()  { return paddle; }
	Paddle* getPPaddle() { return &paddle; }

	void leaveGame();

	//--- Envoie TCP ---//
	void sendVersionTCP(int version, uint32_t start_time);
	void sendNPSTCP(uti::NetworkPaddleStart ne);
	void sendNBALLTCP(uti::NetworkBall nball);

	void send_NPUDP(SOCKET& udpSocket, Player* pData);
	void send_BALLUDP(SOCKET udpSocket, Ball* ball, uint32_t elapsedTime);

	//--- Gestion des états ---//
	std::atomic<bool> availableInPool{ true },//si l'objet est utilisé pour un joueur dans la pool
		connected = { false }, inGame = { false }, inMatchmaking = { false };
				      //Thread safe car:
	short gameID = -1//se fait au lancement d'une partie et à sa fin ? les 2 ne peuvent pas se faire en même temps ?!
		, id	 = 0//Se fait à la connexion d'un joueur, après on n'y touche pas !
		, side	 = 0;//est modifié lors de la création d'une game et c'est tout ?!
	//std::atomic<short> gameID{ -1 }, id{ 0 }, side { 0 };

	std::shared_mutex mtx_udp;
	std::shared_mutex mtx_socket;
	vector<char> recvBuffer;//uniquement utilisé et modifié dans listen_tcpSocket ?! no need mtx

private:
	//ne pas utiliser un pointeur ! dans accept, trouver un joueur dispo et lui attribuer le socket? en parler avec le chat
	SOCKET* tcpSocket = nullptr;//uniquement modifié dans listen_tcpSocket ?! no need mtx dans listen_tcp socket, uniquement dans getters

	sockaddr_in addr = {};//mettre un mutex udpSocket sur ces 2 là ?!
	int	  addrLen	=  0;

	//std::mutex mtx_states;

	//--- Paddle ---//
	Paddle paddle;
	float paddleWidth = 10.2f;
};

