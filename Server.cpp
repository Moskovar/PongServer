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
    walls.push_back(Wall(glm::vec3(0.0f, 0.0f, -40.0f)));
    walls.push_back(Wall(glm::vec3(0.0f, 0.0f,  40.0f)));


    //t_check_connections = new thread(&Server::check_connections, this);
    //cout << "- Check connections thread runs !" << endl;

    t_listen_clientsTCP = new thread(&Server::listen_clientsTCP, this);
    cout << "- Listen TCP thread runs !" << endl;

    t_listen_clientsUDP = new thread(&Server::listen_clientsUDP, this);
    cout << "- Listen UDP thread runs !" << endl;

    t_run_games = new thread(&Server::run_games, this);
    cout << "- Run games thread runs !" << endl;
    
    //t_send_clientsUDP   = new thread(&Server::send_NEUDP, this);
    //cout << "- Send UDP thread runs !" << endl;

    //t_move_players = new thread(&Server::move_players, this);
    //cout << "- Move players thread runs !" << endl;
}

Server::~Server()
{
    if (t_listen_clientsTCP != nullptr && t_listen_clientsTCP->joinable())  t_listen_clientsTCP->join(); 
    if (t_listen_clientsUDP != nullptr && t_listen_clientsUDP->joinable())  t_listen_clientsUDP->join();
    if (t_send_clientsTCP   != nullptr && t_send_clientsTCP->joinable())    t_send_clientsTCP->join();
    if (t_send_clientsUDP   != nullptr && t_send_clientsUDP->joinable())    t_send_clientsUDP->join();
    if (t_run_games         != nullptr && t_run_games->joinable())          t_run_games->join();

    delete t_listen_clientsTCP;
    delete t_listen_clientsUDP;
    delete t_send_clientsTCP;
    delete t_send_clientsUDP;
    delete t_run_games;

    t_listen_clientsTCP = nullptr;
    t_listen_clientsUDP = nullptr;
    t_send_clientsTCP   = nullptr;
    t_send_clientsUDP   = nullptr;
    t_run_games         = nullptr;

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
            int     gameID = -1, id      = -1;//pour donner un id au joueur
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
        
            v_players.push_back(p);
            players[id] = p;

            if (v_players.size() % 2 == 0)//si 2 joueurs alors on lance une game
            {   
                if (!v_players[0]->connected)
                {
                    v_players.erase(v_players.begin());
                    std::cout << "Player 1 disconnected ! Waiting for another player..." << std::endl;
                    continue;
                }

                for (int i = 0; i < MAX_GAME_NUMBER; i++)//<= pour parcourir une case de plus et si tout est full alors la game sera placé à la fin (id = size)
                {
                    if (games[i] == nullptr) { gameID = i; break; }//si on trouve un trou, on met id à i et on sort de la boucle
                }
                mtx_games.lock();
                games[gameID] = new Game(gameID, v_players[0], v_players[1], &walls);
                mtx_games.unlock();
                v_players.erase(v_players.begin());//on enlève le premier élément
                v_players.erase(v_players.begin());//on enlève le premier élément, qui était le second avant d'avoir enlever l'élément précédent

                std::cout << "Game is starting..." << std::endl;
            }

            cout << "Connection succeed !" << endl << players.size() << " clients " << id << " connected !" << endl;
        }
        else std::cerr << "Connexion échoué -> erreur: " << WSAGetLastError() << std::endl;
        std::cout << "Waiting for new connection..." << std::endl;
    }
}

//--- Convertie un timestamp en string ---//
std::string timestampToTimeString(uint64_t timestamp) {
    time_t rawTime = static_cast<time_t>(timestamp);
    struct tm localTime;
    localtime_s(&localTime, &rawTime);

    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &localTime);

    return buffer;
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
        for (auto it = players.begin(); it != players.end(); ++it)// inutile de lock, la partie qui supprime est après dans la même fonction donc pas de concurrence
            if (it->second && it->second->isSocketValid()) 
                FD_SET(*it->second->getTCPSocket(), &readfds);

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

        mtx_players.lock();

        for (auto it = players.begin(); it != players.end();) 
        {
            if (!it->second) 
            {
                it = players.erase(it);//si le pointeur est nullptr, on supprime le player de la liste des joueurs
                continue;
            }

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
                        //if      (header == uti::Header::NE)   dataSize = sizeof(uti::NetworkEntity);

                        if (it->second->recvBuffer.size() >= dataSize)
                        {
                            // On gère les données reçu en fonction du header
                            //if      (header == uti::Header::NE)
                            //{
                            //    uti::NetworkEntity ne;
                            //    std::memcpy(&ne, it->second->recvBuffer.data(), sizeof(uti::NetworkEntity));

                            //    ne.header    = ntohs(ne.header);
                            //    ne.id        = ntohs(ne.id);
                            //    ne.hp        = ntohs(ne.hp);
                            //    ne.countDir  = ntohs(ne.countDir);
                            //    ne.xMap      = ntohl(ne.xMap);
                            //    ne.yMap      = ntohl(ne.yMap);
                            //    ne.timestamp = ntohll(ne.timestamp);

                            //    uint64_t now = static_cast<uint64_t>(std::time(nullptr));

                            //    if (ne.timestamp > (now - 5)) {//Si les données ne datent pas trop on les gèrent
                            //        //--- AJOUTER UNE VERIF DISTANCE PARCOURUE / TEMPS ---//
                            //        it->second->update(ne);//On met à jour le joueur en fonction des données reçues
                            //        send_NETCP(ne, it->second);//On envoie la nouvelle position du joueur à tous les joueurs
                            //    }
                            //    else {//Si les données datent trop, on cancel l'action en renvoyant les dernières data connues par le serveur
                            //        send_NETCP(it->second->getNE());
                            //    }
                            //}

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
                    //short gameID = -1;
                    //Game* game = nullptr;
                    //if (it->second && it->second->getGameID() != -1)
                    //{
                    //    game = games[it->second->getGameID()];
                    //    if (game)
                    //    {
                    //        game->mtx.lock();
                    //        std::cout << "MTX LOCK BEFORE DELETE" << std::endl;
                    //    }
                    //}
                    //if (it->second) { delete it->second; it->second = nullptr; }
                    //it = players.erase(it);

                    if (it->second) it->second->connected = false;

                    std::cout << "A client has been disconnected, " << players.size() << " left" << std::endl;
                    std::cout << it->second->isSocketValid() << std::endl;

                    //if (game)
                    //{
                    //    std::cout << "MTX UNLOCK AFTER DELETE" << std::endl;
                    //    game->mtx.unlock();
                    //}
                    continue;
                }
            }
            ++it;
        }
        
        mtx_players.unlock();
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

            int bytesReceived = recvfrom(udpSocket, (char*)&np, sizeof(np), 0, (sockaddr*)&clientAddr, &clientAddrLen);
            if (bytesReceived <= 0) std::cerr << "Erreur lors de la reception des donnees." << WSAGetLastError() << std::endl;

            // Vérifiez si le message reçu est complet en comparant la taille des données reçues avec la taille attendue.
            if (bytesReceived != sizeof(np)) {
                std::cerr << "Le message UDP reçu est incomplet." << std::endl;
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

                //if(np.gameID != 0) 
                //ne.spell = htons(ne.spell);
                //float xMap = (float)np.xMap / 100;
                //float yMap = (float)np.yMap / 100;
                //players[np.id]->setPos(xMap, yMap);
                ////players[ne.id]->spell = ne.spell;
                //sockaddr_in cli = *players[np.id]->getPAddr();
                //char clientIp[INET_ADDRSTRLEN];
                //inet_ntop(AF_INET, &(cli.sin_addr), clientIp, INET_ADDRSTRLEN);
                //cout << players[np.id]->getID() << " : " << players[np.id]->getXMap() << " : " << players[np.id]->getYMap() << " : " << clientIp << endl;
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

    while (run)
    {
        auto currentTime = Clock::now();
        std::chrono::duration<float> deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        //mtx_games.lock();
        for (auto it = games.begin(); it != games.end();)
        {
            if (it->second)
            {
                if (it->second->allPlayersDisconnected())
                {
                    mtx_players.lock();
                    mtx_games.lock();

                    Player* p1 = it->second->getP1();
                    if (p1)
                    {
                        players.erase(p1->getID());
                        delete p1;
                        p1 = nullptr;
                    }

                    Player* p2 = it->second->getP2();
                    if (p2)
                    {
                        players.erase(p2->getID());
                        delete p2;
                        p2 = nullptr;
                    }

                    Game* game = it->second;
                    it = games.erase(it);
                    delete game;
                    game = nullptr;


                    mtx_games.unlock();
                    mtx_players.unlock();

                    continue;
                }

                //std::cout << "ALL PLAYERS DC: " << it->second->allPlayersDisconnected() << std::endl;
                //std::cout << "P1 CONNECTED: " << it->second->getP1()->connected << std::endl;
                //std::cout << "P2 CONNECTED: " << it->second->getP2()->connected << std::endl;

                if (it->second->roundStarted)//Si la game a démarré, on la joue
                {
                    it->second->run(udpSocket, deltaTime.count());
                }
                else if (!it->second->roundStarted && uti::getCurrentTimestamp() - it->second->round_start_time >= 3)//si la game n'a pas démarré
                {
                    it->second->startRound();
                }
                
            }
            ++it;
        }
        //mtx_games.unlock();


        //std::cout << "DeltaTime: " << deltaTime.count() << std::endl;

        //std::this_thread::sleep_for(std::chrono::microseconds(1)); // Pause de 1 µs
        //std::this_thread::sleep_for(std::chrono::seconds(1)); // Pause de 1 µs
    }
}