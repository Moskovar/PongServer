#include "Server.h"

Server::Server()//gérer les erreurs avec des exceptions
{
    // Initialiser Winsock
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        exit(1);
    }

    //--- TCP SOCKET ---//
    // Initialisation du socket TCP
    connectionSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connectionSocket == INVALID_SOCKET) {
        std::cerr << "Erreur lors de la création du socket TCP" << std::endl;
        WSACleanup();
        exit(1);
    }

    //Initialisation de l'adresse et du port d'écoute du serveur TCP
    tcpServerAddr.sin_family = AF_INET;
    tcpServerAddr.sin_port = htons(50000);
    tcpServerAddr.sin_addr.s_addr = INADDR_ANY;

    //Bind du socket TCP à son adresse et port d'écoute
    if (bind(connectionSocket, (sockaddr*)&tcpServerAddr, sizeof(tcpServerAddr)) == SOCKET_ERROR) {
        std::cerr << "Erreur lors du bind du socket TCP" << std::endl;
        closesocket(connectionSocket);
        WSACleanup();
        exit(1);
    }

    cout << "Socket TCP a l'ecoute" << endl;

    //Mise à l'écoute du socket TCP
    if (listen(connectionSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Erreur lors de l'écoute sur le socket TCP." << std::endl;
        closesocket(connectionSocket);
        WSACleanup();
        exit(1);
    }

    //--- UDP SOCKET ---//
    // Initialisation du socket UDP
    udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket == INVALID_SOCKET) {
        std::cerr << "Erreur lors de la création du socket UDP." << std::endl;
        closesocket(connectionSocket);
        WSACleanup();
        exit(1);
    }

    //Définition de l'adresse et du port d'écoute du socket UDP
    udpServerAddr.sin_family = AF_INET;
    udpServerAddr.sin_port = htons(50001);
    udpServerAddr.sin_addr.s_addr = INADDR_ANY;

    //Bind du socket UDP à son adresse et port d'écoute
    if (bind(udpSocket, (const sockaddr*)&udpServerAddr, sizeof(udpServerAddr)) == SOCKET_ERROR) {
        std::cerr << "Erreur lors du bind du socket UDP." << std::endl;
        closesocket(connectionSocket);
        closesocket(udpSocket);
        WSACleanup();
        exit(1);
    }
    cout << "Socket UDP a l'ecoute" << endl;

    //Mise à l'état non bloquant du socket UDP
    u_long mode = 1;
    ioctlsocket(udpSocket, FIONBIO, &mode);

    //--- Création des décors statiques ---//
    walls.push_back(Wall(glm::vec3(0.0f, 0.0f, -40.5f)));
    walls.push_back(Wall(glm::vec3(0.0f, 0.0f,  40.5f)));


    //t_check_connections = new thread(&Server::check_connections, this);
    //cout << "- Check connections thread runs !" << endl;

    t_listen_clientsTCP = new thread(&Server::listen_clientsTCP, this);
    cout << "- Listen TCP thread runs !" << endl;

    t_listen_clientsUDP = new thread(&Server::listen_clientsUDP, this);
    cout << "- Listen UDP thread runs !" << endl;

    t_run_games = new thread(&Server::run_games, this);
    cout << "- Run games thread runs !" << endl;

    t_run_matchmaking = new thread(&Server::run_matchmaking, this);
    cout << "- Matchmaking thread runs !" << endl;
    
    //t_send_clientsUDP   = new thread(&Server::send_NEUDP, this);
    //cout << "- Send UDP thread runs !" << endl;

    //t_move_players = new thread(&Server::move_players, this);
    //cout << "- Move players thread runs !" << endl;
}

Server::~Server()
{
    run = false;

    if (t_listen_clientsTCP != nullptr && t_listen_clientsTCP->joinable())  t_listen_clientsTCP->join(); 
    if (t_listen_clientsUDP != nullptr && t_listen_clientsUDP->joinable())  t_listen_clientsUDP->join();
    if (t_send_clientsTCP   != nullptr && t_send_clientsTCP->joinable())    t_send_clientsTCP->join();
    if (t_send_clientsUDP   != nullptr && t_send_clientsUDP->joinable())    t_send_clientsUDP->join();
    if (t_run_games         != nullptr && t_run_games->joinable())          t_run_games->join();
    if (t_run_matchmaking   != nullptr && t_run_matchmaking->joinable())    t_run_matchmaking->join();

    delete t_listen_clientsTCP;
    delete t_listen_clientsUDP;
    delete t_send_clientsTCP;
    delete t_send_clientsUDP;
    delete t_run_games;
    delete t_run_matchmaking;

    t_listen_clientsTCP = nullptr;
    t_listen_clientsUDP = nullptr;
    t_send_clientsTCP   = nullptr;
    t_send_clientsUDP   = nullptr;
    t_run_games         = nullptr;
    t_run_matchmaking   = nullptr;

    for (auto it = players.begin(); it != players.end();)
    {
        delete it->second;
        it->second = nullptr;
        it = players.erase(it);
    }

    for (auto it = games.begin(); it != games.end();)
    {
        delete it->second;
        it->second = nullptr;
        it = games.erase(it);
    }

    closesocket(connectionSocket);
    closesocket(udpSocket);
    WSACleanup();

    cout << "Le serveur est complétement nettoyé" << endl;
}

//--- Accepte les connexions du joueur sur le serveur ---//
void Server::accept_connections()
{
    std::cout << "Waiting for connections..." << std::endl;
    while (run)
    {
        SOCKET* tcpSocket = new SOCKET();
        *tcpSocket = accept(connectionSocket, NULL, NULL);
        if (*tcpSocket != INVALID_SOCKET) 
        {
            int id      = -1;//pour donner un id au joueur
            for (int i = 0; i < MAX_PLAYER_NUMBER; i++)//<= pour parcourir une case de plus et si tout est full alors le player sera placé à la fin (id = size)
            {
                if (players[i] == nullptr) { id = i; break; }//si on trouve un trou, on met id à i et on sort de la boucle
            }
            if (id == -1) 
            { 
                cout << "Le serveur est plein..." << endl; //fermer la connexion en fermant le socket et pas en voyant id -1 ???
                //envoyer un msg de serveur full
                continue; 
            }
            //uti::NetworkEntity ne = { 0, id, 0, 50, (1920 + 800) * 100, (1080 + 500) * 100, 0 };//structure pour communiquer dans le réseau

            Player* p = new Player(tcpSocket, id);
        
            //matchmaking.push_back(p);
            mtx_players.lock();
            players[id] = p;
            mtx_players.unlock();

            p->sendVersionTCP(1010);//on envoie la version au joueur

            cout << "Connection succeed !" << endl << players.size() << " clients " << id << " connected !" << endl;
        }
        else std::cerr << "Connexion échoué -> erreur: " << WSAGetLastError() << std::endl;
        std::cout << "Waiting for new connection..." << std::endl;
    }
}

//--- Ecoute les messages envoyés par l'ensemble des joueurs connectés ---//
void Server::listen_clientsTCP()
{
    fd_set readfds; // structure pour surveiller un ensemble de descripteurs de fichiers pour lire (ici les sockets)
    timeval timeout;
    const int recvbuflen = 512;

    while (run) 
    {
        //std::cout << "LISTEN" << std::endl;
        FD_ZERO(&readfds); // reset l'ensemble &readfds
        mtx_players.lock();//lock manuellement
        for (auto it = players.begin(); it != players.end();)
        {
            Player* p = it->second;
            if ((!p || !p->connected || !p->isSocketValid()) && p && !p->inGame)//Si un joueur n'est plus valid (déco nullptr etc) on le nettoie
            {
                if (p)
                {
                    if (p->isSocketValid())
                    {
                        SOCKET* tcpSocket = p->getTCPSocket();
                        closesocket(*tcpSocket);
                        tcpSocket = nullptr;
                    }

                    delete p;
                    p = nullptr;
                }

                it = players.erase(it);//si le pointeur est nullptr, on supprime le player de la liste des joueurs

                std::cout << "A disconnected player has been removed... " << players.size() << " left !" << std::endl;
                continue;
            }

            if(it->second) FD_SET(*it->second->getTCPSocket(), &readfds);

            ++it;
        }
        mtx_players.unlock();//unlock manuellement pour éviter les 100ms de pause après

        // Vérifiez si le fd_set est vide
        if (readfds.fd_count == 0) 
        {
            // Aucun socket à surveiller, attendez un peu avant de réessayer
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            continue;
        }

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int iResult = select(0, &readfds, NULL, NULL, &timeout); // Attend que l'un des sockets dans readfds soit prêt pour la lecture (ou jusqu'à ce que le délai d'attente expire)
        if (iResult == SOCKET_ERROR) {
            std::cerr << "Select failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        std::lock_guard<std::mutex> lock_players(mtx_players);
        for (auto it = players.begin(); it != players.end();) 
        {
            SOCKET* clientSocket = it->second->getTCPSocket();
            if (clientSocket && it->second->connected && FD_ISSET(*clientSocket, &readfds)) //On check si on le socket a reçu des données
            {
                char buffer[recvbuflen];
                iResult = recv(*clientSocket, buffer, recvbuflen, 0);
                if (iResult > 0) //Si on a reçu quelque chose
                {
                    it->second->recvBuffer.insert(it->second->recvBuffer.end(), buffer, buffer + iResult);

                    // Process received data
                    while (it->second->recvBuffer.size() >= sizeof(short)) 
                    {
                        // Check if we have enough data for the header
                        short header = 0;
                        std::memcpy(&header, it->second->recvBuffer.data(), sizeof(short));
                        header = ntohs(header);
                        //cout << "Header: " << header << endl;

                        unsigned long dataSize = 0;
                        if      (header == uti::Header::SPELL)          dataSize = sizeof(uti::NetworkSpell);
                        else if (header == uti::Header::MATCHMAKING)    dataSize = sizeof(uti::NetworkMatchmaking);
                        else if (header == uti::Header::STATE)          dataSize = sizeof(uti::NetworkState);

                        if (it->second->recvBuffer.size() >= dataSize)
                        {
                            // On gère les données reçu en fonction du header
                            if      (header == uti::Header::SPELL)
                            {
                                uti::NetworkSpell ns;
                                std::memcpy(&ns, it->second->recvBuffer.data(), sizeof(uti::NetworkSpell));

                                ns.header   = ntohs(ns.header);
                                ns.spellID  = ntohs(ns.spellID);
                                //ne.hp        = ntohs(ne.hp);
                                //ne.countDir  = ntohs(ne.countDir);
                                //ne.xMap      = ntohl(ne.xMap);
                                //ne.yMap      = ntohl(ne.yMap);
                                //ne.timestamp = ntohll(ne.timestamp);

                                std::cout << "SPELL RECEIVED: " << ns.header << " : " << ns.spellID << std::endl;

                                //uint64_t now = static_cast<uint64_t>(std::time(nullptr));

                                //if (ns.timestamp > (now - 5)) {//Si les données ne datent pas trop on les gèrent
                                //    //--- AJOUTER UNE VERIF DISTANCE PARCOURUE / TEMPS ---//
                                //    it->second->update(ns);//On met à jour le joueur en fonction des données reçues
                                //    send_NETCP(ne, it->second);//On envoie la nouvelle position du joueur à tous les joueurs
                                //}
                                //else {//Si les données datent trop, on cancel l'action en renvoyant les dernières data connues par le serveur
                                //    //send_NETCP(it->second->getNE());
                                //}
                            }
                            else if (header == uti::Header::MATCHMAKING)
                            {
                                std::cout << "Demande de matchmaking received!" << std::endl;
                                bool found = false;
                                int i = -1;

                                for (Player* p : matchmaking)
                                {
                                    ++i;
                                    if (p == it->second)
                                    {
                                        found = true;
                                        break;
                                    }
                                }

                                mtx_matchmaking.lock();
                                if (!found)
                                {
                                    matchmaking.push_back(it->second);
                                    std::cout << "Un joueur a rejoint le matchmaking... " << matchmaking.size() << " actuellement dans le matchmaking" << std::endl;
                                }
                                else
                                {
                                    std::cout << "i = " << i << std::endl;
                                    matchmaking.erase(matchmaking.begin() + i);
                                    std::cout << "Un joueur a leave le matchmaking... " << matchmaking.size() << " actuellement dans le matchmaking" << std::endl;
                                }
                                mtx_matchmaking.unlock();
                            }
                            else if (header == uti::Header::STATE)
                            {
                                uti::NetworkState nst;
                                std::memcpy(&nst, it->second->recvBuffer.data(), sizeof(uti::NetworkState));

                                nst.header = ntohs(nst.header);
                                nst.inGame = ntohs(nst.inGame);

                                std::cout << "STATE RECEIVED: " << nst.header << " : " << nst.inGame << std::endl;
                            }
                            
                            it->second->recvBuffer.erase(it->second->recvBuffer.begin(), it->second->recvBuffer.begin() + dataSize);
                        }
                        else {
                            break; // We don't have enough data yet
                        }
                    }
                }
                else if (iResult == 0 || iResult == SOCKET_ERROR) 
                {
                    if (it->second && !it->second->connected) continue;
                    //cout << "ERROR: " << WSAGetLastError() << " : " << iResult << endl;

                    if (it->second) it->second->connected = false;

                    std::cout << "A client has been disconnected, " << players.size() << " left" << std::endl;

                    continue;
                }
            }
            ++it;
        }
    }

    std::cout << "t_listen_clientsTCP finished..." << std::endl;
}

void Server::listen_clientsUDP()
{
    //cout << "UDP LISTENING" << endl;

    fd_set readfds;
    timeval timeout;

    while (run)
    {
        FD_ZERO(&readfds);
        FD_SET(udpSocket, &readfds);

        // Vérifiez si le fd_set est vide
        if (readfds.fd_count == 0) {
            // Aucun socket à surveiller, attendez un peu avant de réessayer
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        timeout.tv_sec = 1;  // 1 seconde de timeout
        timeout.tv_usec = 0;

        int iResult = select(0, &readfds, NULL, NULL, &timeout);//Attend que l'un des sockets dans readfds soit prêt pour la lecture (ou jusqu'à ce que le délai d'attente expire)
        if (iResult == SOCKET_ERROR) {
            std::cerr << "Select failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        if (FD_ISSET(udpSocket, &readfds)) {
            sockaddr_in clientAddr;
            uti::NetworkPaddle np;
            int clientAddrLen = sizeof(clientAddr);
            int error = 0;

            int bytesReceived = recvfrom(udpSocket, (char*)&np, sizeof(np), 0, (sockaddr*)&clientAddr, &clientAddrLen);
            if (bytesReceived <= 0)
            {
                error = WSAGetLastError();
                if(error != 10054)//10054 = le client a fermé la connexion brutalement
                    std::cerr << "Erreur lors de la reception des donnees." << error << std::endl;
            }

            // Vérifiez si le message reçu est complet en comparant la taille des données reçues avec la taille attendue.
            if (bytesReceived != sizeof(np)) 
            {
                if(error != 10054) std::cerr << "Le message UDP received est incomplet." << std::endl;
                continue; // Si le message n'est pas complet, passez à l'itération suivante.
            }

            np.gameID = htons(np.gameID);
            np.id = htons(np.id);
            np.z = static_cast<int32_t>(ntohl(np.z));
            //if(np.id == 0) std::cout << "Received: " << np.gameID << " : " << np.id << " : " << (float)(np.z / 1000.0f) << std::endl;

            if (players[np.id] != nullptr && games[np.gameID] != nullptr)
            {
                //std::cout << "MSG sent to players in the game: " << np.gameID << std::endl;

                players[np.id]->setAddr(clientAddr);
                players[np.id]->update(np);

                games[np.gameID]->getOtherPlayer(np.id)->send_NPUDP(udpSocket, players[np.id]);
            }
            else
            {
                std::cout << "Msg UDP skip, player nullptr -> ID: " << np.id << std::endl;
            }
        }
    }  
}

void Server::run_games()
{
    using Clock = std::chrono::high_resolution_clock;
    auto lastTime = Clock::now();
    last_timestamp_send_ball = uti::getCurrentTimestampMs();

    while (run)
    {
        auto currentTime = Clock::now();
        std::chrono::duration<float> deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        if (uti::getCurrentTimestampMs() - last_timestamp_send_ball >= 17) //17 ~= 60fps
            last_timestamp_send_ball = uti::getCurrentTimestampMs();

        std::lock_guard<std::mutex> lock_players(mtx_players);
        std::lock_guard<std::mutex> lock_games(mtx_games);
        for (auto it = games.begin(); it != games.end();)
        {
            if (it->second)
            {
                std::cout << "P1 DC: "  << !it->second->getP1()->connected << std::endl;
                std::cout << "P2 DC: "  << !it->second->getP2()->connected << std::endl;
                std::cout << "ALL DC: " <<  it->second->allPlayersDisconnected() << std::endl;

                if (it->second->allPlayersDisconnected() || it->second->allPlayersLeftGame())
                {
                    Player* p1 = it->second->getP1();
                    if (p1) p1->inGame = false;

                    Player* p2 = it->second->getP2();
                    if (p2) p2->inGame = false;

                    Game* game = it->second;
                    delete game;
                    game = nullptr;
                    it = games.erase(it);

                    std::cout << "A game has been finished from disconnexion..." << std::endl;

                    continue;
                }

                if (it->second->roundStarted)//Si la game a démarré, on la joue
                {
                    it->second->run(udpSocket, deltaTime.count());
                    if(uti::getCurrentTimestampMs() - last_timestamp_send_ball >= 17) //(1s) 1000ms / 60(fps) = 16.66ms
                        it->second->sendBallToPlayersUDP(udpSocket);
                }
                else if (!it->second->roundStarted && uti::getCurrentTimestamp() - it->second->round_start_time >= 3)//si la game n'a pas démarré
                {
                    it->second->startRound(udpSocket);
                }
            }
            ++it;
        }

        //std::cout << "DeltaTime: " << deltaTime.count() << std::endl;
    }
}

void Server::run_matchmaking()
{
    while(run)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::unique_lock<std::mutex> lock_mm(mtx_matchmaking);//attention à l'ordre de lock
        std::unique_lock<std::mutex> lock_players(mtx_players);//doit tjrs être lock dans le même ordre

        int nb_players = matchmaking.size();

        if (nb_players < 2) continue;//s'il y a moins de 2 joueurs dans le matchmaking on skip

        int gameID = -1;

        Player* p1 = matchmaking[0];
        Player* p2 = matchmaking[1];

        if (!p1 || !p1->connected || !p1->isSocketValid())//si le joueur est déconnecté, on l'enlève du matchmaking et on recommence la boucle
        {
            matchmaking.erase(matchmaking.begin());
            std::cout << "A player left the matchmaking ! Players waiting for a game: " << matchmaking.size() << std::endl;
            continue;
        }

        for (int i = 0; i < MAX_GAME_NUMBER; i++)
        {
            if (games[i] == nullptr) //si on trouve un trou, on met id à i et on sort de la boucle
            { 
                gameID = i; 
                break; 
            }
        }

        if (gameID == -1) continue;//Si gameID == -1, pas d'ID de game a été trouvé, on recommence la boucle

        Game* game = new Game(gameID, p1, p2, &walls);

        if (!game)
        {
            std::cerr << "Error: Could not create a new game!" << std::endl;
            continue;
        }

        mtx_games.lock();
        games[gameID] = game;
        mtx_games.unlock();


        matchmaking.erase(matchmaking.begin());//on enlève le premier élément
        matchmaking.erase(matchmaking.begin());//on enlève le premier élément, qui était le second avant d'avoir enlever l'élément précédent

        std::cout << "Game is starting..." << std::endl;
    }
}
