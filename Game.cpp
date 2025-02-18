#include "Game.h"
#include <chrono>

Game::Game(short gameID, Player* p1, Player* p2, std::vector<Wall>* walls)
{
    this->gameID = gameID;
    this->p1 = p1;
    this->p2 = p2;

    this->walls = walls;

    p1->setGameID(gameID);
    p2->setGameID(gameID);

    p1->setSide(-1);
    p2->setSide(1);

    uti::NetworkPaddleStart nps1 = p1->getNps();
    uti::NetworkPaddleStart nps2 = p2->getNps();

    p1->sendNPSTCP(nps1);//envoyer le joueur à lui même en premier
    p1->sendNPSTCP(nps2);

    p2->sendNPSTCP(nps2);//envoyer le joueur à lui même en premier
    p2->sendNPSTCP(nps1);

    sendBallSpeedTCP();

    //ball = new Ball(glm::vec3(0.0f, 0.0f, 0.0f));


    //std::cout << "P1: " << p1->getPaddle()->getX() << " : " << p1->getPaddle()->getZ() << std::endl;
    //std::cout << "P2: " << p2->getPaddle()->getX() << " : " << p2->getPaddle()->getZ() << std::endl;



    std::cout << "Game created with ID: " << gameID << std::endl;

    uint64_t now = uti::getCurrentTimestamp();

    game_created_time   = now;
    round_start_time    = now;
    ballSpeed_increase  = now;
}

Game::~Game()
{
    //if (ball)
    //{
    //	delete ball;
    //	ball = nullptr;
    //}
}

Player* Game::getOtherPlayer(short id)
{
    if (p1->getID() != id)	return p1;
    else					return p2;
}

void Game::resetBall()
{
    ball.reset();
    sendBallToPlayersTCP();
}

void Game::sendBallToPlayersTCP()
{
    uti::NetworkBall nball = ball.getNball();
    p1->sendNBALLTCP(nball);
    p2->sendNBALLTCP(nball);
}

void Game::sendBallSpeedTCP()
{
    uti::NetworkBallSpeed nbs;
    nbs.speed = ball.moveSpeed;
    p1->sendNBALLSPEEDTCP(nbs);
    p2->sendNBALLSPEEDTCP(nbs);
}

void Game::sendBallToPlayersUDP(SOCKET& udpSocket)
{
    p1->send_BALLUDP(udpSocket, &ball);
    p2->send_BALLUDP(udpSocket, &ball);
}

void Game::startRound()
{
    ball.start((lastWinner) ? -lastWinner->getSide() : 0);

    roundStarted = true;
    round_start_time = uti::getCurrentTimestamp();
    sendBallToPlayersTCP();
}

void Game::resetRound()
{
    resetBall();
    roundStarted = false;
    round_start_time = uti::getCurrentTimestamp();
}

void Game::increaseBallSpeed()
{
    if (ball.increaseMoveSpeed())
        sendBallSpeedTCP();
}

void Game::run(SOCKET& udpSocket, float deltaTime)
{
    float distance;
    Element* element = nullptr;
    short isPaddle = true;//pour vérifier le type d'Element, plus rapide qu'un dynamic cast ?

    //Augmentation de la vitesse en fonction du temps
    if ((uti::getCurrentTimestamp() - ballSpeed_increase) >= 1)
    {
        ballSpeed_increase = uti::getCurrentTimestamp();
        increaseBallSpeed();
    }

    if (ball.velocityX < 0 && p1)
    {
        element = p1->getPaddle();

        if (!element) return;

        if (ball.getX() < element->getX() - 10)
        {
            std::cout << "OUT!" << std::endl;
            resetRound();
            lastWinner = p2;
        }
    }
    else if (ball.velocityX > 0 && p2)
    {
        element = p2->getPaddle();

        if (!element) return;

        if (ball.getX() > element->getX() + 10)
        {
            std::cout << "OUT!" << std::endl;
            resetRound();
            lastWinner = p1;
        }
    }
    //else return;

    distance = distanceBetweenHitboxes(&ball, element);


    //Si la distance avec le joueur est > 0, alors on vérifie la distance avec les murs
    if (distance > 0)
    {
        for (Element& wall : *walls)
        {
            distance = distanceBetweenHitboxes(&wall, &ball);

            if (distance == 0)//Si distance avec le mur == 0, on sort
            {
                element = &wall;
                isPaddle = false;//passe à un si on est au contact d'un wall, sinon reste à 0
                break;
            }
        }
    }
    
    //Si la distance avec le dernier élément comparé est > 0, on déplace la balle, sinon on traite la collision en fonction de l'élément
    if (distance > 0 || ball.lastElementHit == element)//ou si le dernier élément touché != element actuel -> évite de bloquer la balle dans le mur si elle se déplace pas assez après un rebond
    {
        ball.move(deltaTime);
    }
    else
    {
        if(isPaddle)
        {
            std::cout << "COLLISION AVEC PADDLE" << std::endl;

            //---- VelocityX ---//
            ball.turnback();

            //--- VelocityZ ---//
            float velocityZ = (ball.getZ() - element->getZ()) / (static_cast<Paddle*>(element)->width / 2);


            ball.velocityZ = velocityZ;
        }
        else
        {
            ball.velocityZ = -ball.velocityZ;
        }

        ball.lastElementHit = element;

        //--- Send data ---//
        //uti::NetworkBall nb = ball.getNball();
        //p1->sendNBALLTCP(nb);
        //p2->sendNBALLTCP(nb);
        //p1->send_BALLUDP(udpSocket, &ball);
        //p2->send_BALLUDP(udpSocket, &ball);

        //while (distanceBetweenHitboxes(&ball, element) == 0)
        //{
        //    ball.move(deltaTime);
        //}
    }
}

float Game::distanceBetweenHitboxes(Element* e1, Element* e2)
{
   float distanceX = 0.0f;
   float distanceY = 0.0f;
   float distanceZ = 0.0f;

   if (!e1 || !e2) std::cout << "E1 OR E2 NULLPTR !!!!" << std::endl;

    glm::vec3 e1_maxPoint = e1->getRHitbox().maxPoint;
    glm::vec3 e1_minPoint = e1->getRHitbox().minPoint;
    glm::vec3 e2_maxPoint = e2->getRHitbox().maxPoint;
    glm::vec3 e2_minPoint = e2->getRHitbox().minPoint;

    // Axe X
    if (e1_maxPoint.x < e2_minPoint.x)
    {
        distanceX = e2_minPoint.x - e1_maxPoint.x;  // À gauche
    }
    else if (e1_minPoint.x > e2_maxPoint.x)
    {
        distanceX = e1_minPoint.x - e2_maxPoint.x;  // À droite
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
        distanceZ = e2_minPoint.z - e1_maxPoint.z;  // Derrière
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
