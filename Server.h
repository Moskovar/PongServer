#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <map>
#include <unordered_set>
#include <thread>
#include <mutex>
#include <ctime>
#include "Player.h"
#include "Game.h"
#include "Wall.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define MAX_PLAYER_NUMBER	1000
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
		std::map<int, Player>		playersPool;//liste dans laquelle sont placés les nouveaux joueurs dont la connexion a été acceptée -> accès rapide
		std::vector<Player*>		matchmaking;
		std::unordered_set<Player*> playersConnected;
		std::map<int, Game>			gamesPool;
		std::unordered_set<Game*>	gamesLaunched;

		long long last_timestamp_send_ball = 0;

		sockaddr_in udpServerAddr, tcpServerAddr;//addesse du socket de connexion et du socket udp

		mutex   mtx_players, mtx_games;//mutex pour gérer l'accès aux set de joueurs connectés et games lancées
		thread* t_listen_clientsTCP = nullptr;//thread pour écouter les joueurs en TCP
		thread* t_listen_clientsUDP = nullptr;//thread pour écouter les joueurs en UDP
		thread* t_send_clientsTCP   = nullptr;//thread pour envoyer des données en TCP aux joueurs
		thread* t_send_clientsUDP   = nullptr;//thread pour envoyer des données en UDP aux joueurs
		thread* t_run_games			= nullptr;//thread qui s'occupe de faire tourner les parties
		thread* t_run_matchmaking	= nullptr;//thread qui s'occupe de créer les parties

		void listen_clientsTCP();
		void listen_clientsUDP();
		void run_games();
		void run_matchmaking();
};

