#include "Player.h"

Player::Player(SOCKET* tcpSocket)
{
    this->tcpSocket = tcpSocket;

    //Socket mit en mode non bloquant
    u_long mode = 1;
    ioctlsocket(*tcpSocket, FIONBIO, &mode);//mode non bloquant
}

Player::~Player()
{
	if (tcpSocket != nullptr)
	{
		closesocket(*tcpSocket);
		delete tcpSocket;
		tcpSocket = nullptr;
	}

    //if (paddle)
    //{
    //    delete paddle;
    //    paddle = nullptr;
    //}

    cout << "Player cleared !\n" << endl;
}

void Player::setPlayer(SOCKET* tcpSocket)
{
    this->tcpSocket = tcpSocket;

    //Socket mit en mode non bloquant
    u_long mode = 1;
    ioctlsocket(*tcpSocket, FIONBIO, &mode);//mode non bloquant

    setStates();
}

void Player::resetPlayer()
{
    if (tcpSocket != nullptr)
    {
        closesocket(*tcpSocket);
        delete tcpSocket;
        tcpSocket = nullptr;
    }

    //paddle.setPosition(glm::vec3(0.0, 0.0, 0.0));

    resetStates();

    cout << "Player " << getID() << " has been reset !\n" << endl;
}

short Player::getID()
{
    //std::lock_guard<std::mutex> lock(mtx_states);
    return id;
}

short Player::getGameID()
{
    //std::lock_guard<std::mutex> lock(mtx_states);
    return gameID;
}

short Player::getSide()
{
    //std::lock_guard<std::mutex> lock(mtx_states);
    return side;
}

void Player::setID(short id)
{
    //std::lock_guard<std::mutex> lock(mtx_states);
    this->id = id;
}

void Player::setGameID(short gameID)
{
    //std::lock_guard<std::mutex> lock(mtx_states);
    this->gameID = gameID;
}

void Player::setSide(short side)
{
    {
        //std::lock_guard<std::mutex> lock_states(mtx_states);
        this->side = side;
    }

    std::lock_guard<std::mutex> lock_paddle(mtx_paddle);
    if      (side == -1) paddle.setPosition(glm::vec3(-70.0f, 0.0f, 0.0f));
    else if (side ==  1) paddle.setPosition(glm::vec3( 70.0f, 0.0f, 0.0f));
    //else { std::cout << "La paddle existe deja..." << std::endl; }
}

void Player::setStates()
{
    std::lock_guard<std::mutex> lock(mtx_states);
    availableInPool.store(false, std::memory_order_relaxed);
    connected.store(      true , std::memory_order_relaxed);
}

void Player::resetStates()
{
    std::lock_guard<std::mutex> lock(mtx_states);
    availableInPool.store(true , std::memory_order_relaxed);
    connected.store(      false, std::memory_order_relaxed);
    inGame.store(         false, std::memory_order_relaxed);
    inMatchmaking.store(  false, std::memory_order_relaxed);
}

void Player::update(uti::NetworkPaddle& np)
{
    std::lock_guard<std::mutex> lock(mtx_paddle);
    this->paddle.setZ((float)(np.z / 1000.0f));
}

SOCKET* Player::getTCPSocket()
{
    std::shared_lock<std::shared_mutex> lock(mtx_socket);
    return tcpSocket;
}

bool Player::isSocketValid()
{
    std::shared_lock<std::shared_mutex> lock(mtx_socket);
    return !tcpSocket || *tcpSocket != INVALID_SOCKET;
}

void Player::closeSocket()
{
    std::unique_lock<std::shared_mutex> lock(mtx_socket);
    closesocket(*tcpSocket);
    delete tcpSocket;
    tcpSocket = nullptr;
}

void Player::setZ(float z)
{
    std::lock_guard<std::mutex> lock(mtx_paddle);
    paddle.setZ(z);
}

void Player::setAddr(sockaddr_in addr)
{
    this->addr  = addr;
    addrLen     = sizeof(this->addr);
}

void Player::leaveGame()
{
    inGame          = false;
    inMatchmaking   = false;
}

void Player::sendVersionTCP(int version)
{
    if (!connected.load(std::memory_order_relaxed)) return;//si le joueur n'est pas connecté, on sort

    uti::NetworkVersion nv;

    // Convertir les valeurs en big-endian avant l'envoi
    nv.header   = htons(nv.header);
    nv.version  = htonl(version);

    // Envoi sécurisé des données
    int totalSent = 0;
    int dataSize = sizeof(nv);
    const char* data = reinterpret_cast<const char*>(&nv);

    const int MAX_RETRIES = 3;
    int retries = 0;

    while (totalSent < dataSize) 
    {
        if (!isSocketValid()) 
        {
            std::cerr << "Error: Invalid socket!" << std::endl;
            return;
        }

        int iResult = -1;

        {//mtx_socket
            std::unique_lock<std::shared_mutex> lock(mtx_socket);
            iResult = ::send(*tcpSocket, data + totalSent, dataSize - totalSent, 0);
        }

        if (iResult == SOCKET_ERROR)
        {
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) {
                // Le socket est temporairement non disponible, réessayer
                std::cerr << "[sendVersionTCP] Send would block, retrying... (" << retries + 1 << "/" << MAX_RETRIES << ")" << std::endl;
                retries++;

                if (retries == MAX_RETRIES) 
                {
                    std::cerr << "[sendVersionTCP] Send failed after multiple retries, closing socket." << std::endl;
                    closeSocket();
                    return;
                }

                continue;
            }
            else 
            {
                std::cerr << "[sendVersionTCP] send failed: " << error << std::endl;
                closeSocket();
                return;
            }
        }
        else if (iResult == 0) 
        {
            std::cerr << "Connection closed by peer." << std::endl;
            closeSocket();
            return;
        }
        totalSent += iResult;
    }
}

void Player::sendNPSTCP(uti::NetworkPaddleStart nps)
{
    if (!connected.load(std::memory_order_relaxed)) return;//si le joueur n'est pas connecté, on sort
    //std::cout << "NPS SENT: " << nps.header << " : " << nps.gameID << " : " << nps.id << " : " << nps.side << std::endl;

    // Convertir les valeurs en big-endian avant l'envoi
    nps.header = htons(nps.header);
    nps.gameID = htons(nps.gameID);
    nps.id     = htons(nps.id);
    nps.side   = htons(static_cast<uint16_t>(nps.side));

    //std::cout << "NPS SENT RESEAU: " << ntohs(nps.header) << " : " << ntohs(nps.gameID) << " : " << ntohs(nps.id) << " : " << static_cast<short>(ntohs(nps.side)) << std::endl;

    // Envoi sécurisé des données
    int totalSent = 0;
    int dataSize = sizeof(nps);
    const char* data = reinterpret_cast<const char*>(&nps);

    const int MAX_RETRIES = 3;
    int retries = 0;

    while (totalSent < dataSize) 
    {
        if (!isSocketValid())
        {
            std::cerr << "Error: Invalid socket!" << std::endl;
            return;
        }

        int iResult = -1;

        {//mtx_socket
            std::unique_lock<std::shared_mutex> lock(mtx_socket);
            iResult = ::send(*tcpSocket, data + totalSent, dataSize - totalSent, 0);
        }

        if (iResult == SOCKET_ERROR)
        {
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) {
                // Le socket est temporairement non disponible, réessayer
                std::cerr << "[sendVersionTCP] Send would block, retrying... (" << retries + 1 << "/" << MAX_RETRIES << ")" << std::endl;
                retries++;

                if (retries == MAX_RETRIES) {
                    std::cerr << "[sendVersionTCP] Send failed after multiple retries, closing socket." << std::endl;
                    closeSocket();
                    return;
                }

                continue;
            }
            else 
            {
                std::cerr << "[sendVersionTCP] send failed: " << error << std::endl;
                closeSocket();
                return;
            }
        }
        else if (iResult == 0) 
        {
            std::cerr << "Connection closed by peer." << std::endl;
            closeSocket();
            return;
        }

        totalSent += iResult;
    }
    //std::cout << "Size of NetworkPaddleStart: " << sizeof(uti::NetworkPaddleStart) << std::endl;
    //std::cout << "Total sent: " << totalSent << std::endl;
}

void Player::send_NPUDP(SOCKET& udpSocket, Player* pData)
{
    if (!connected.load(memory_order_relaxed))
    {
        //std::cout << "CANT SEND UDP MSG, PLAYER DISCONNECTED" << std::endl;
        return;
    }

    //std::cout << "send_NPUDP" << std::endl;
    if (!pData) 
    { 
        std::cout << "pData nullptr" << std::endl;
        return; 
    }

    if (addr.sin_family != AF_INET)
    {
        std::cout << "addr.sin_family != AF_INET" << std::endl;
        return; //vérifie si l'adresse est initialisée
    }

    uti::NetworkPaddle np = pData->getNp();

    np.header    = htons(np.header);
    np.gameID    = htons(np.gameID);
    np.id        = htons(np.id);
    np.z         = htonl(np.z);
    np.timestamp = htonl(uti::getCurrentTimestamp());

    //std::cout << addrLen << std::endl;

    int bytesSent = sendto(udpSocket, (const char*)&np, sizeof(np), 0, (sockaddr*)&addr, addrLen);
    if (bytesSent == SOCKET_ERROR) 
    {
        std::cerr << "Erreur lors de l'envoi des donnees -> " << WSAGetLastError() << std::endl;
        return;
    }
    //else
    //{
    //    std::cout << "Msg UDP sent : bytesSent: " << bytesSent << std::endl;
    //}
}

void Player::send_BALLUDP(SOCKET udpSocket, Ball* ball)
{
    if (!connected.load(std::memory_order_relaxed) || !inGame.load(std::memory_order_relaxed)) return;

    if (!ball)
    {
        std::cout << "ball nullptr" << std::endl;
        return;
    }

    if (addr.sin_family != AF_INET)
    {
        std::cout << "addr.sin_family != AF_INET" << std::endl;
        return; //vérifie si l'adresse est initialisée
    }

    uti::NetworkBall nb = ball->getNball();

    nb.header       = htons(nb.header);
    nb.x            = htonl(nb.x);
    nb.z            = htonl(nb.z);
    nb.velocityX    = htonl(nb.velocityX);
    nb.velocityZ    = htonl(nb.velocityZ);
    nb.speed        = htons(nb.speed);
    nb.timestamp    = htonl(ball->getTimestamp());

    //std::cout << "BALL SENT: " << ntohl(nb.timestamp) << std::endl;

    //std::cout << addrLen << std::endl;

    int bytesSent = sendto(udpSocket, (const char*)&nb, sizeof(nb), 0, (sockaddr*)&addr, addrLen);
    if (bytesSent == SOCKET_ERROR)
    {
        std::cerr << "Erreur lors de l'envoi des donnees -> " << WSAGetLastError() << std::endl;
        return;
    }
}

void Player::sendNBALLTCP(uti::NetworkBall nball)
{
    if (!connected.load(std::memory_order_relaxed)) return;//si le joueur n'est pas connecté, on sort

    // Envoyer une réponse
    if (tcpSocket)
    {
        //std::cout << "BALL SENT: " << nball.x << " : " << nball.z << " : " << nball.velocityX << " : " << nball.velocityZ << std::endl;
        
        nball.header    = htons(nball.header);
        nball.x         = htonl(nball.x);
        nball.z         = htonl(nball.z);
        nball.velocityX = htonl(static_cast<int32_t>(nball.velocityX));
        nball.velocityZ = htonl(static_cast<int32_t>(nball.velocityZ));
        nball.timestamp = htonl(nball.timestamp);

        //std::cout << "BALL SENT: " << (float)(ntohl(nball.x) / 1000.0f) << " : " << (float)(ntohl(nball.z) / 1000.0f) << " : " << (float)(ntohl(nball.velocityX) / 1000.0f) << " : " << (float)(ntohl(nball.velocityZ) / 1000.0f) << " -> " << nball.timestamp << std::endl;

        // Envoi sécurisé des données
        int totalSent = 0;
        int dataSize = sizeof(nball);
        const char* data = reinterpret_cast<const char*>(&nball);

        const int MAX_RETRIES = 3;
        int retries = 0;

        while (totalSent < dataSize)
        {
            if (!isSocketValid())
            {
                std::cerr << "Error: Invalid socket!" << std::endl;
                return;
            }

            int iResult = -1;

            {//mtx_socket
                std::unique_lock<std::shared_mutex> lock(mtx_socket);
                iResult = ::send(*tcpSocket, data + totalSent, dataSize - totalSent, 0);
            }

            if (iResult == SOCKET_ERROR)
            {
                int error = WSAGetLastError();
                if (error == WSAEWOULDBLOCK) {
                    // Le socket est temporairement non disponible, réessayer
                    std::cerr << "[sendVersionTCP] Send would block, retrying... (" << retries + 1 << "/" << MAX_RETRIES << ")" << std::endl;
                    retries++;

                    if (retries == MAX_RETRIES) {
                        std::cerr << "[sendVersionTCP] Send failed after multiple retries, closing socket." << std::endl;
                        closeSocket();
                        return;
                    }

                    continue;
                }
                else
                {
                    std::cerr << "[sendVersionTCP] send failed: " << error << std::endl;
                    closeSocket();
                    return;
                }
            }
            else if (iResult == 0)
            {
                std::cerr << "Connection closed by peer." << std::endl;
                closeSocket();
                return;
            }

            totalSent += iResult;
        }
    }
}
