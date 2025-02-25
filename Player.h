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

	//--- Gestion des �tats ---//
	short getID();
	short getGameID();
	short getSide();

	void setID(short id);
	void setGameID(short gameID);
	void setSide(short side);//utilis� quand la game est cr��e, on cr��e le paddle �galement
	void setStates();
	void resetStates();

	int						getAddrLen()	{ return addrLen;														}
	uti::NetworkPaddleStart getNps()		{ return { uti::Header::NPS, gameID, id, side };						}
	uti::NetworkPaddle		getNp()			{ return { uti::Header::NP , gameID, id, (int)(paddle.getZ() * 1000)}; }

	//--- Setters ---//
	//Paddle prot�g� par mtx_paddle
	void setZ(float z);
	//Paddle prot�g� par mtx_paddle
	void setAddr(sockaddr_in addr);//uniquement utils� dans listen_udpSocket
	
	//--- Lien entre la communication et le joueur ---//

	//Paddle prot�g� par mtx_paddle
	void update(uti::NetworkPaddle& np);

	//--- communication ---//
	SOCKET* getTCPSocket();
	sockaddr_in* getPAddr() { return &addr; }
	bool isSocketValid();
	float getPaddleWidth() { return paddleWidth; }
	Paddle  getPaddle()  { return paddle; }
	Paddle* getPPaddle() { return &paddle; }


	void leaveGame();

	//--- Envoie TCP ---//
	void sendVersionTCP(int version);
	void sendNPSTCP(uti::NetworkPaddleStart ne);
	void sendNBALLTCP(uti::NetworkBall nball);

	void send_NPUDP(SOCKET& udpSocket, Player* pData);
	void send_BALLUDP(SOCKET udpSocket, Ball* ball);

	//--- Gestion des �tats ---//
	std::atomic<bool> availableInPool{ true },//si l'objet est utilis� pour un joueur dans la pool
		connected = { false }, inGame = { false }, inMatchmaking = { false };
				      //Thread safe car:
	short gameID = -1//se fait au lancement d'une partie et � sa fin ? les 2 ne peuvent pas se faire en m�me temps ?!
		, id	 = 0//Se fait � la connexion d'un joueur, apr�s on n'y touche pas !
		, side	 = 0;//est modifi� lors de la cr�ation d'une game et c'est tout ?!
	//std::atomic<short> gameID{ -1 }, id{ 0 }, side { 0 };

	std::mutex mtx_socket;
	vector<char> recvBuffer;//uniquement utilis� et modifi� dans listen_tcpSocket ?! no need mtx

private:
	SOCKET* tcpSocket = nullptr;//uniquement modifi� dans listen_tcpSocket ?! no need mtx dans listen_tcp socket, uniquement dans getters

	sockaddr_in addr = {};//mettre un mutex udpSocket sur ces 2 l� ?!
	int	  addrLen	=  0;

	std::mutex mtx_states;

	//--- Paddle ---//
	std::mutex mtx_paddle;
	Paddle paddle;
	float paddleWidth = 10.2f;

	//Pong

	std::chrono::steady_clock::time_point prevTime;//pour calculer le deltatime
};

