#include "Game.h"
#include <chrono>

Game::~Game()
{
    //if (ball)
    //{
    //	delete ball;
    //	ball = nullptr;
    //}
}

void Game::reset()
{
    p1          = nullptr;
    p2          = nullptr;
    walls       = nullptr;
    lastWinner  = nullptr;

    //on reset la position de la balle dans le set, au moment d'envoyer les donn�es de positions de la balle aux joueurs
    ball.setMoveSpeed(50);
    
    roundStarted.store(false, std::memory_order_relaxed);
    availableInPool.store(true, std::memory_order_relaxed);
}

bool Game::set(short gameID, Player* p1, Player* p2, std::vector<Wall>* walls)
{
    this->gameID = gameID;

    this->p1 = p1;
    this->p2 = p2;

    if (!p1 || !p2) return false;

    this->walls = walls;

    //ball.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));

    p1->inMatchmaking.store(false, std::memory_order_relaxed);
    p2->inMatchmaking.store(false, std::memory_order_relaxed);

    p1->setGameID(gameID);
    p2->setGameID(gameID);

    p1->setSide(-1);
    p2->setSide(1);

    resetBall();

    uti::NetworkPaddleStart nps1 = p1->getNps();
    uti::NetworkPaddleStart nps2 = p2->getNps();

    p1->sendNPSTCP(nps1);//envoyer le joueur � lui m�me en premier
    p1->sendNPSTCP(nps2);

    p1->inGame.store(true, std::memory_order_relaxed);

    p2->sendNPSTCP(nps2);//envoyer le joueur � lui m�me en premier
    p2->sendNPSTCP(nps1);

    p2->inGame.store(true, std::memory_order_relaxed);

    std::cout << "Game created with ID: " << gameID << std::endl;

    uint64_t now = uti::getCurrentTimestamp();

    game_created_time       = now;
    round_start_time        = now;
    ballSpeed_increase_time = now;

    availableInPool.store(false, std::memory_order_relaxed);

    return true;
}

Player* Game::getOtherPlayer(short id)
{
    if (p1->getID() != id)	return p1;
    else					return p2;
}

bool Game::allPlayersDisconnected()
{
    //return !p1 && !p2 && !p1->connected && !p2->connected && !p1->isSocketValid() && p2->isSocketValid();
    if (!p1 && !p2) return true;
    if (!p1)        return !p2->connected;
    if (!p2)        return !p1->connected;

    if (p1->connected) return false;
    if (p2->connected) return false;

    return true;
}

bool Game::allPlayersLeftGame()
{
    if (!p1 && !p2) return true;
    if (!p1)        return !p2->inGame;
    if (!p2)        return !p1->inGame;

    if (p1->inGame) return false;
    if (p2->inGame) return false;

    return true;
}

void Game::resetBall()
{
    ball.reset();
    sendBallToPlayersTCP();
}

void Game::sendBallToPlayersTCP()
{
    uti::NetworkBall nball = ball.getNball();
    nball.timestamp = ball.getTimestamp();

    p1->sendNBALLTCP(nball);
    p2->sendNBALLTCP(nball);
}

void Game::sendBallToPlayersUDP(SOCKET& udpSocket, uint32_t elapsedTime)
{
    p1->send_BALLUDP(udpSocket, &ball, elapsedTime);
    p2->send_BALLUDP(udpSocket, &ball, elapsedTime);
}

void Game::startRound(SOCKET& udpSocket, uint32_t elapsedTime)
{
    ball.start((lastWinner) ? -lastWinner->getSide() : 0);

    roundStarted.store(true, std::memory_order_relaxed);
    round_start_time = uti::getCurrentTimestamp();
    sendBallToPlayersUDP(udpSocket, elapsedTime);
}

void Game::resetRound()
{
    resetBall();
    roundStarted.store(false, std::memory_order_relaxed);
    round_start_time = uti::getCurrentTimestamp();
}

void Game::increaseBallSpeed()
{
    ball.increaseMoveSpeed();
    //if (ball.increaseMoveSpeed())
    //    sendBallSpeedTCP();
}

void Game::run(SOCKET& udpSocket, float deltaTime, uint32_t elapsedTime)
{
    float distance;
    Element* element = nullptr;
    short isPaddle = true;//pour v�rifier le type d'Element, plus rapide qu'un dynamic cast ?

    //Augmentation de la vitesse en fonction du temps
    if ((uti::getCurrentTimestamp() - ballSpeed_increase_time) >= 1)
    {
        ballSpeed_increase_time = uti::getCurrentTimestamp();
        increaseBallSpeed();
    }

    glm::vec3 paddlePosition;

    float velocityX = ball.getVelocityX(),
          ballX     = ball.getX();

    if (velocityX < 0 && p1)
    {
        element = p1->getPPaddle();

        if (!element) return;

        paddlePosition = element->getPosition();
        //Mise � jour de la matrice mod�le des paddles (le thread udp met � jour la position mais pas la matrice pour que le mutex ne bloque que la position)
        p1->updatePaddleModelMatrix(paddlePosition);

        if (ballX < paddlePosition.x - 10)
        {
            std::cout << "OUT!" << std::endl;
            resetRound();
            lastWinner = p2;
        }
    }
    else if (velocityX > 0 && p2)
    {
        element = p2->getPPaddle();

        if (!element) return;

        paddlePosition = element->getPosition();
        //Mise � jour de la matrice mod�le des paddles (le thread udp met � jour la position mais pas la matrice pour que le mutex ne bloque que la position)
        p2->updatePaddleModelMatrix(paddlePosition);

        if (ballX > paddlePosition.x + 10)
        {
            std::cout << "OUT!" << std::endl;
            resetRound();
            lastWinner = p1;
        }
    }
    else
    {
        std::cout << "Paddle Element Nullptr!" << std::endl;
        return;
    }

    //std::cout << paddlePosition.x << " : " << paddlePosition.y << " : " << paddlePosition.z << std::endl;

    distance = distanceBetweenHitboxes(&ball, element);

    //if(distance == 0) 
        //std::cout << distance << std::endl;
    //if (distance == 0 && isPaddle) 
    //    std::cout << "VERIF1: 0 && paddle" << std::endl;


    //Si la distance avec le joueur est > 0, alors on v�rifie la distance avec les murs
    if (distance > 0)
    {
        for (Element& wall : *walls)
        {
            distance = distanceBetweenHitboxes(&wall, &ball);

            if (distance == 0)//Si distance avec le mur == 0, on sort
            {
                element = &wall;
                isPaddle = false;//passe � un si on est au contact d'un wall, sinon reste � 0
                break;
            }
        }
    }

    //if (distance == 0 && isPaddle)
    //    std::cout << "VERIF2: 0 && paddle" << std::endl;
    
    //Si la distance avec le dernier �l�ment compar� est > 0, on d�place la balle, sinon on traite la collision en fonction de l'�l�ment
    if (distance > 0 || ball.lastElementHit == element)//ou si le dernier �l�ment touch� != element actuel -> �vite de bloquer la balle dans le mur si elle se d�place pas assez apr�s un rebond
    {
        ball.move(deltaTime);
    }
    else
    {
        if(isPaddle)
        {
            //std::cout << "COLLISION AVEC PADDLE" << std::endl;
            //if (distance == 0 && isPaddle)
            //    std::cout << "VERIF3: 0 && paddle" << std::endl;
            //---- VelocityX ---//
            ball.turnback();

            //--- VelocityZ ---//
            float velocityZ = (ball.getZ() - paddlePosition.z) / (static_cast<Paddle*>(element)->width / 2);


            ball.setVelocityZ(velocityZ);
        }
        else
        {
            ball.setVelocityZ(-ball.getVelocityZ());
        }

        ball.lastElementHit = element;

        sendBallToPlayersUDP(udpSocket, elapsedTime);
    }
}

float Game::distanceBetweenHitboxes(Element* e1, Element* e2)
{
   float distanceX = 0.0f;
   float distanceY = 0.0f;
   float distanceZ = 0.0f;

   if (!e1)
   {
       std::cout << "E1 NULLPTR !!!!" << std::endl;
       return 1000.0f;
   }

   if (!e2)
   {
       std::cout << "E2 NULLPTR !!!!" << std::endl;
       return 1000.0f;
   }

    glm::vec3 e1_maxPoint = e1->getRHitbox().maxPoint;
    glm::vec3 e1_minPoint = e1->getRHitbox().minPoint;
    glm::vec3 e2_maxPoint = e2->getRHitbox().maxPoint;
    glm::vec3 e2_minPoint = e2->getRHitbox().minPoint;

    // Axe X
    if (e1_maxPoint.x < e2_minPoint.x)
    {
        distanceX = e2_minPoint.x - e1_maxPoint.x;  // � gauche
    }
    else if (e1_minPoint.x > e2_maxPoint.x)
    {
        distanceX = e1_minPoint.x - e2_maxPoint.x;  // � droite
    }

    // Axe Y
    if (e1_maxPoint.y < e2_minPoint.y)
    {
        distanceY = e2_minPoint.y - e1_maxPoint.y;  // En-dessous
    }
    else if (e1_minPoint.y > e2_maxPoint.y)
    {
        distanceY = e1_minPoint.y - e2_maxPoint.y;  // Au-dessus
    }

    // Axe Z
    if (e1_maxPoint.z < e2_minPoint.z)
    {
        distanceZ = e2_minPoint.z - e1_maxPoint.z;  // Derri�re
    }
    else if (e1_minPoint.z > e2_maxPoint.z)
    {
        distanceZ = e1_minPoint.z - e2_maxPoint.z;  // Devant
    }

    // Si les hitboxes se chevauchent, retourner 0
    if (distanceX <= 0.0f && distanceY <= 0.0f && distanceZ <= 0.0f)
    {
        return 0.0f;  // Collision
    }

    // Calculer la distance totale entre les deux hitboxes en 3D
    return glm::sqrt(distanceX * distanceX + distanceY * distanceY + distanceZ * distanceZ);
}
