#include "Player.h"

Player::Player(SOCKET* tcpSocket, short id)
{
    this->tcpSocket = tcpSocket;
    this->id        = id;

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

    if (paddle)
    {
        delete paddle;
        paddle = nullptr;
    }

    cout << "Player cleared !\n" << endl;
}

void Player::update(uti::NetworkPaddle& np)
{
    //this->z = (float)(np.z / 1000.0f);
    this->paddle->setZ((float)(np.z / 1000.0f));
}

void Player::setSide(short side)
{
    this->side = side;

    if (!paddle)
    {
        if      (side == -1) paddle = new Paddle(glm::vec3(-70.0f, 0.0f, 0.0f));
        else if (side ==  1) paddle = new Paddle(glm::vec3( 70.0f, 0.0f, 0.0f));
    }
    else { std::cout << "La paddle existe déjà..." << std::endl; }
}

void Player::sendVersionTCP(int version)
{
    uti::NetworkVersion nv;

    // Convertir les valeurs en big-endian avant l'envoi
    nv.header   = htons(nv.header);
    nv.version  = htonl(version);

    // Envoi sécurisé des données
    int totalSent = 0;
    int dataSize = sizeof(nv);
    const char* data = reinterpret_cast<const char*>(&nv);

    while (totalSent < dataSize) {
        int iResult = ::send(*tcpSocket, data + totalSent, dataSize - totalSent, 0);
        if (iResult == SOCKET_ERROR)
        {
            std::cerr << "send failed: " << WSAGetLastError() << std::endl;
            closesocket(*tcpSocket);
            return;
        }
        totalSent += iResult;
    }
}

void Player::sendNPSTCP(uti::NetworkPaddleStart nps)
{
    std::cout << "NPS SENT: " << nps.header << " : " << nps.gameID << " : " << nps.id << " : " << nps.side << std::endl;

    // Convertir les valeurs en big-endian avant l'envoi
    nps.header = htons(nps.header);
    nps.gameID = htons(nps.gameID);
    nps.id     = htons(nps.id);
    nps.side   = htons(static_cast<uint16_t>(nps.side));

    std::cout << "NPS SENT RESEAU: " << ntohs(nps.header) << " : " << ntohs(nps.gameID) << " : " << ntohs(nps.id) << " : " << static_cast<short>(ntohs(nps.side)) << std::endl;


    // Envoi sécurisé des données
    int totalSent = 0;
    int dataSize = sizeof(nps);
    const char* data = reinterpret_cast<const char*>(&nps);

    while (totalSent < dataSize) {
        int iResult = ::send(*tcpSocket, data + totalSent, dataSize - totalSent, 0);
        if (iResult == SOCKET_ERROR)
        {
            std::cerr << "send failed: " << WSAGetLastError() << std::endl;
            closesocket(*tcpSocket);
            return;
        }
        totalSent += iResult;
    }

    std::cout << "Size of NetworkPaddleStart: " << sizeof(uti::NetworkPaddleStart) << std::endl;
    std::cout << "Total sent: " << totalSent << std::endl;
}

void Player::send_NPUDP(SOCKET& udpSocket, Player* pData)
{
    if (!connected)
    {
        //std::cout << "CANT SEND UDP MSG, PLAYER DISCONNECTED" << std::endl;
        return;
    }

    //std::cout << "send_NPUDP" << std::endl;
    if (pData == nullptr) 
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
    //std::cout << "send_NPUDP" << std::endl;
    if (ball == nullptr)
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
    nb.timestamp    = htonl(uti::getCurrentTimestamp());

    //std::cout << addrLen << std::endl;

    int bytesSent = sendto(udpSocket, (const char*)&nb, sizeof(nb), 0, (sockaddr*)&addr, addrLen);
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

void Player::sendNBALLTCP(uti::NetworkBall nball)
{
    if (!connected) return;//si le joueur n'est pas connecté, on sort

    // Envoyer une réponse
    if (tcpSocket != nullptr)
    {
        //std::cout << "BALL SENT: " << nball.x << " : " << nball.z << " : " << nball.velocityX << " : " << nball.velocityZ << std::endl;
        
        nball.header    = htons(nball.header);
        nball.x         = htonl(nball.x);
        nball.z         = htonl(nball.z);
        nball.velocityX = htonl(static_cast<int32_t>(nball.velocityX));
        nball.velocityZ = htonl(static_cast<int32_t>(nball.velocityZ));
        nball.timestamp = htonl(nball.timestamp);

        std::cout << "BALL SENT: " << (float)(ntohl(nball.x) / 1000.0f) << " : " << (float)(ntohl(nball.z) / 1000.0f) << " : " << (float)(ntohl(nball.velocityX) / 1000.0f) << " : " << (float)(ntohl(nball.velocityZ) / 1000.0f) << std::endl;

        int iResult = ::send(*tcpSocket, reinterpret_cast<const char*>(&nball), sizeof(nball), 0);
        if (iResult == SOCKET_ERROR)
        {
            std::cerr << "send failed: " << WSAGetLastError() << std::endl;
            closesocket(*tcpSocket);
        }
    }
}
