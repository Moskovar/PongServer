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
	Player(SOCKET* tcpSocket, short id, short hp, float xMap, float yMap);
	Player(SOCKET* tcpSocket, short id);
	~Player();

	//--- Données du joueur ---//
	short getID() { return id; }
	int getAddrLen() { return addrLen; }
	float getXMap() { return xMap; }
	float getYMap() { return yMap; }
	short getFaction() { return this->faction; }

	short getGameID()	{ return gameID;	}
	short getSide()		{ return side;		}
	uti::NetworkPaddleStart getNps()	{ return { uti::Header::NPS, gameID, id, side }; }
	uti::NetworkPaddle		getNp()		{ return { uti::Header::NP , gameID, id, (int)(paddle->getZ() * 1000)	  }; }

	void setGameID(short gameID) { this->gameID = gameID; }
	void setSide(short side);//utilisé quand la game est créée, on créée le paddle également
	void setZ(float z) { paddle->setZ(z); }
	
	void setPos(float xMap, float yMap) { this->xMap = xMap; this->yMap = yMap; }
	void setFaction(short faction) { this->faction = faction; }
	void applyDmg(short dmg);

	//--- Lien entre la communication et le joueur ---//
	void update(uti::NetworkPaddle& np);

	//--- communication ---//
	SOCKET* getTCPSocket() { return this->tcpSocket; }
	sockaddr_in* getPAddr() { return &addr; }
	bool isSocketValid() { return tcpSocket && *tcpSocket != INVALID_SOCKET; }
	float getPaddleWidth() { return paddleWidth; }
	Paddle* getPaddle() { return paddle; }

	void setAddr(sockaddr_in addr) { this->addr = addr; addrLen = sizeof(this->addr); }

	void sendNPSTCP(uti::NetworkPaddleStart ne);
	void sendNBALLTCP(uti::NetworkBall nball);
	void send_NPUDP(SOCKET& udpSocket, Player* pData);
	void send_NBUDP(SOCKET& udpSocket, Ball*   ball);

	bool connected = true;
	vector<char> recvBuffer;

private:
	SOCKET* tcpSocket = nullptr;
	sockaddr_in addr = {};
	int	  addrLen	= 0;
	short	gameID	= -1,//-1 = pas de game
			id		=  0, countDir	= 0   , hp         = 0   , faction    = 0   , spell = 0   , dir = 0;
	float xMap = 0.0f, yMap = 0.0f, xCenterBox = 0.0f, yCenterBox = 0.0f, xRate = 0.0f, yRate = 0.0f, xChange = 0.0f, yChange = 0.0f, speed = 400,
		z = 0.0f, paddleWidth = 10.2f;
	Paddle* paddle = nullptr;

	//Pong
	short side = 0;

	std::chrono::steady_clock::time_point prevTime;//pour calculer le deltatime
};

