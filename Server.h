#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <map>
#include <thread>
#include <mutex>
#include <ctime>
#include "Player.h"
#include "Game.h"
#include "Wall.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define MAX_PLAYER_NUMBER	100
#define MAX_GAME_NUMBER		(MAX_PLAYER_NUMBER/2)

class Server
{
	public:
		Server();
		~Server();

		void accept_connections();

		std::vector<Wall> walls;

	private:
		bool run = true, isDeleting = false;
		WSADATA wsaData;

		SOCKET connectionSocket = INVALID_SOCKET;//socket pour recevoir et accepter les connexions des clients
		SOCKET udpSocket		= INVALID_SOCKET;//socket pour communiquer en UDP
		map<int, Player*>		players;//liste dans laquelle sont plac�s les nouveaux joueurs dont la connexion a �t� accept�e -> acc�s rapide
		std::vector<Player*>	v_players;
		std::map<int, Game*>	games;

		sockaddr_in udpServerAddr, tcpServerAddr;//addesse du socket de connexion et du socket udp

		mutex   mtx_players, mtx_games;//mutex pour g�rer l'acc�s aux ressources 
		thread* t_listen_clientsTCP = nullptr;//thread pour �couter les joueurs en TCP
		thread* t_listen_clientsUDP = nullptr;//thread pour �couter les joueurs en UDP
		thread* t_send_clientsTCP   = nullptr;//thread pour envoyer des donn�es en TCP aux joueurs
		thread* t_send_clientsUDP   = nullptr;//thread pour envoyer des donn�es en UDP aux joueurs
		thread* t_run_games			= nullptr;

		void listen_clientsTCP();
		void listen_clientsUDP();
		void run_games();
};

