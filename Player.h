#pragma once
#include <winsock2.h>
#include <iostream>
#include <vector>
#include <chrono>
#include "uti.h"
#include "Paddle.h"
#include "Ball.h"
//#include <ws2tcpip.h>

using namespace std;

class Player
{
public:
	Player(SOCKET* tcpSocket, short id);
	~Player();

	//--- Données du joueur ---//
	short getID() { return id; }
	int getAddrLen() { return addrLen; }

	short getGameID()	{ return gameID;	}
	short getSide()		{ return side;		}
	uti::NetworkPaddleStart getNps()	{ return { uti::Header::NPS, gameID, id, side }; }
	uti::NetworkPaddle		getNp()		{ return { uti::Header::NP , gameID, id, (int)(paddle->getZ() * 1000)	  }; }

	void setGameID(short gameID) { this->gameID = gameID; }
	void setSide(short side);//utilisé quand la game est créée, on créée le paddle également
	void setZ(float z) { paddle->setZ(z); }
	
	//--- Lien entre la communication et le joueur ---//
	void update(uti::NetworkPaddle& np);

	//--- communication ---//
	SOCKET* getTCPSocket() { return this->tcpSocket; }
	sockaddr_in* getPAddr() { return &addr; }
	bool isSocketValid() { return tcpSocket && *tcpSocket != INVALID_SOCKET; }
	float getPaddleWidth() { return paddleWidth; }
	Paddle* getPaddle() { return paddle; }

	void setAddr(sockaddr_in addr) { this->addr = addr; addrLen = sizeof(this->addr); }

	void leaveGame();

	//--- Envoie TCP ---//
	void sendVersionTCP(int version);
	void sendNPSTCP(uti::NetworkPaddleStart ne);
	void sendNBALLTCP(uti::NetworkBall nball);

	void send_NPUDP(SOCKET& udpSocket, Player* pData);
	void send_BALLUDP(SOCKET udpSocket, Ball* ball);

	bool connected = true, inGame = false;;
	vector<char> recvBuffer;

private:
	SOCKET* tcpSocket = nullptr;
	sockaddr_in addr = {};
	int	  addrLen	= 0;
	short	gameID	= -1,//-1 = pas de game
			id		=  0;
	float z = 0.0f, paddleWidth = 10.2f;
	Paddle* paddle = nullptr;

	//Pong
	short side = 0;

	std::chrono::steady_clock::time_point prevTime;//pour calculer le deltatime
};

