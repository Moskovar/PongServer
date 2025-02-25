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

    //on reset la position de la balle dans le set, au moment d'envoyer les données de positions de la balle aux joueurs
    ball.setMoveSpeed(25);
    
    roundStarted    = false;
    availableInPool = true;
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

    p1->sendNPSTCP(nps1);//envoyer le joueur à lui même en premier
    p1->sendNPSTCP(nps2);

    p1->inGame.store(true, std::memory_order_relaxed);

    p2->sendNPSTCP(nps2);//envoyer le joueur à lui même en premier
    p2->sendNPSTCP(nps1);

    p2->inGame.store(true, std::memory_order_relaxed);

    std::cout << "Game created with ID: " << gameID << std::endl;

    uint64_t now = uti::getCurrentTimestamp();

    setGame_created_time(now);
    setRound_start_time(now);
    setBallSpeed_increase_time(now);

    availableInPool = false;

    return true;
}

Player* Game::getOtherPlayer(short id)
{
    if (p1->getID() != id)	return p1;
    else					return p2;
}

bool Game::isAvailableInPool()
{
    std::lock_guard<std::mutex> lock(mtx_states);
    return availableInPool;
}

bool Game::isRoundStarted()
{
    std::lock_guard<std::mutex> lock(mtx_states);
    return roundStarted;
}

void Game::setAvailableInPool(bool state)
{
    std::lock_guard<std::mutex> lock(mtx_states);
    availableInPool = state;
}

void Game::setRoundStarted(bool state)
{
    std::lock_guard<std::mutex> lock(mtx_states);
    roundStarted = state;
}

u_int64 Game::getGame_created_time()
{
    std::lock_guard<std::mutex> lock(mtx_states);
    return game_created_time;
}

u_int64 Game::getRound_start_time()
{
    std::lock_guard<std::mutex> lock(mtx_states);
    return round_start_time;
}

u_int64 Game::getBallSpeed_increase_time()
{
    std::lock_guard<std::mutex> lock(mtx_states);
    return ballSpeed_increase;
}

void Game::setGame_created_time(u_int64 time)
{
    std::lock_guard<std::mutex> lock(mtx_states);
    game_created_time = time;
}

void Game::setRound_start_time(u_int64 time)
{
    std::lock_guard<std::mutex> lock(mtx_states);
    round_start_time = time;
}

void Game::setBallSpeed_increase_time(u_int64 time)
{
    std::lock_guard<std::mutex> lock(mtx_states);
    ballSpeed_increase = time;
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

void Game::sendBallToPlayersUDP(SOCKET& udpSocket)
{
    p1->send_BALLUDP(udpSocket, &ball);
    p2->send_BALLUDP(udpSocket, &ball);
}

void Game::startRound(SOCKET& udpSocket)
{
    ball.start((lastWinner) ? -lastWinner->getSide() : 0);

    roundStarted = true;
    round_start_time = uti::getCurrentTimestamp();
    sendBallToPlayersUDP(udpSocket);
}

void Game::resetRound()
{
    resetBall();
    roundStarted = false;
    round_start_time = uti::getCurrentTimestamp();
}

void Game::increaseBallSpeed()
{
    ball.increaseMoveSpeed();
    //if (ball.increaseMoveSpeed())
    //    sendBallSpeedTCP();
}

void Game::run(SOCKET& udpSocket, float deltaTime)
{
    float distance;
    Element* element = nullptr;
    short isPaddle = true;//pour vérifier le type d'Element, plus rapide qu'un dynamic cast ?

    //Augmentation de la vitesse en fonction du temps
    if ((uti::getCurrentTimestamp() - getBallSpeed_increase_time()) >= 1)
    {
        setBallSpeed_increase_time(uti::getCurrentTimestamp());
        increaseBallSpeed();
    }

    float velocityX = ball.getVelocityX(),
          ballX     = ball.getX();

    if (velocityX < 0 && p1)
    {
        element = p1->getPPaddle();

        if (!element) return;

        if (ballX < element->getX() - 10)
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

        if (ballX > element->getX() + 10)
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
            //std::cout << "COLLISION AVEC PADDLE" << std::endl;

            //---- VelocityX ---//
            ball.turnback();

            //--- VelocityZ ---//
            float velocityZ = (ball.getZ() - element->getZ()) / (static_cast<Paddle*>(element)->width / 2);


            ball.setVelocityZ(velocityZ);
        }
        else
        {
            ball.setVelocityZ(-ball.getVelocityZ());
        }

        ball.lastElementHit = element;
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
