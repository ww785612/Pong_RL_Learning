#include <stdio.h>
#include <vector>
#include <random>
#define BOARD_HEIGHT 1
#define BOARD_WIDTH 1
#define PADDLE_HEIGHT 0.2
#define AI_PADDLE_SPEED 0.02
#define RL_AI_PADDLE_SPEED 0.04
#define HI_RANDOM_VELOCITY_X 0.015
#define LO_RANDOM_VELOCITY_X -0.015
#define HI_RANDOM_VELOCITY_Y 0.03
#define LO_RANDOM_VELOCITY_Y -0.03
#define BALL_SPEED_LIMIT 0.5

enum ACTION
{
    STAY,
    UP,
    DOWN,
    NUM_ACTION
}

enum BOUNCE_TYPE
{
    NO_BOUNCE,
    OFF_WALL,
    OFF_PADDLE,
    AI_WIN,
    RL_AI_WIN,
    NUM_BOUNCE_TYPE
}

enum PLAYER_TYPE
{
    RL_AI,
    AI,
    NUM_AI
}

class Paddle
{
private:
    Ball* ball;
public:
    Paddle(float XPos):
    posX(XPos),
    posY(0.5 - PADDLE_HEIGHT / 2),
    {}

    float posX;
    float posY;
    std::vector<std::vector<float>> Q;

    void initialize(Ball* new_ball)
    {
        ball = new_ball;
    }

    void initializeAgent(std::vector<std::vector<float>> new_Q)
    {
        Q = new_Q;
    }

    void chooseMove(PLAYER_TYPE playerType)
    {
        if(playerType == AI)
        {
            if(posY > ball->posY)
            {
                move(UP, AI_PADDLE_SPEED);
            }
            else if(posY < ball->posY)
            {
                move(DOWN, AI_PADDLE_SPEED);
            }
            else
            {
                move(STAY, AI_PADDLE_SPEED);   
            }
        }
        else if(playerType == RL_AI)
        {
            
        }
    }

private:
    void move(ACTION action, float velocity)
    {
        switch(action)
        {
            case(UP):
            {
                posY -= velocity;
                if(posY < 0)
                {
                    posY = 0;   
                }
            }
            case(DOWN):
            {
                posY += velocity;
                if(posY > (1 - PADDLE_HEIGHT))
                {
                    posY = 1 - PADDLE_HEIGHT;   
                }
            }
            default:
                return;
        }
    }
}

class Ball
{
public:
    float posX;
    float posY;
    float velocityX;
    float velocityY;
    Paddle* paddleL;
    Paddle* paddleR;

    Ball(Paddle* new_paddleL, Paddle* new_paddleR;)
    {
        posX = 0.5;
        posY = 0.5;  
        velocityX = 0.03;
        velocityY = 0.01;
        paddleL = new_paddleL;
        paddleR = new_paddleR;
    }

    void move()
    {
        // if bounced both on wall and paddle in same time frame, OFF_WALL is ignored.
        bool bouncedOnWall = 0;

        posY += velocityY;
        if(posY < 0)
        {
            posY = -posY;
            velocityY = -velocityY;
            bouncedOnWall = true;
        }
        else if(posY > 1)
        {
            posY = 2 - posY;
            velocityY = -velocityY;   
            velocityY = -velocityY;
            bouncedOnWall = true;
        }

        posX += velocityX;
        // ball reaches left end
        if(posX < 0)
        {
            if((posY >= paddleL->posY) && (posY <= (paddleL->posY + PADDLE_HEIGHT)))
            {
                posX = - posX;
                float velocityVariationX =  LO_RANDOM_VELOCITY_X + 
                                            static_cast <float> (rand()) /
                                            (static_cast <float> (RAND_MAX/(HI_RANDOM_VELOCITY_X - LO_RANDOM_VELOCITY_X)));

                float velocityVariationY =  LO_RANDOM_VELOCITY_Y + 
                                            static_cast <float> (rand()) /
                                            (static_cast <float> (RAND_MAX/(HI_RANDOM_VELOCITY_Y - LO_RANDOM_VELOCITY_Y)));

                velocityX = -velocityX + velocityVariationX;
                assert(velocityX >= 0);
                // if ball is moving too slow on  X axis
                if(velocityX < HI_RANDOM_VELOCITY_X)
                {
                    velocityX = HI_RANDOM_VELOCITY_X;
                }
                // if ball is moving too fast on X axis
                else if(velocityX > BALL_SPEED_LIMIT)
                {
                    velocityX = BALL_SPEED_LIMIT;
                }
                velocityY = velocityY + velocityVariationY;
                // if ball is moving too fast on Y axis
                if(velocityY > BALL_SPEED_LIMIT || velocityY < -BALL_SPEED_LIMIT)
                {
                    if(velocityY > 0)
                    {
                        velocityY = BALL_SPEED_LIMIT;
                    }
                    else
                    {
                        velocityY = -BALL_SPEED_LIMIT;
                    }
                }
                return OFF_PADDLE;
            }
            else
            {
                return RL_AI_WIN;
            }
        }

        // ball reaches right end
        if(posX > 1)
        {
            if((posY >= paddleR->posY) && (posY <= (paddleR->posY + PADDLE_HEIGHT)))
            {
                posX = - posX;
                float velocityVariationX =  LO_RANDOM_VELOCITY_X + 
                                            static_cast <float> (rand()) /
                                            (static_cast <float> (RAND_MAX/(HI_RANDOM_VELOCITY_X - LO_RANDOM_VELOCITY_X)));

                float velocityVariationY =  LO_RANDOM_VELOCITY_Y + 
                                            static_cast <float> (rand()) /
                                            (static_cast <float> (RAND_MAX/(HI_RANDOM_VELOCITY_Y - LO_RANDOM_VELOCITY_Y)));

                velocityX = -velocityX + velocityVariationX;
                assert(velocityX <= 0);
                // if ball is moving too slow on  X axis
                if(velocityX > -HI_RANDOM_VELOCITY_X)
                {
                    velocityX = -HI_RANDOM_VELOCITY_X;
                }
                // if ball is moving too fast on X axis
                else if(velocityX < -BALL_SPEED_LIMIT)
                {
                    velocityX = -BALL_SPEED_LIMIT;
                }
                velocityY = velocityY + velocityVariationY;
                // if ball is moving too fast on Y axis
                if(velocityY > BALL_SPEED_LIMIT || velocityY < -BALL_SPEED_LIMIT)
                {
                    if(velocityY > 0)
                    {
                        velocityY = BALL_SPEED_LIMIT;
                    }
                    else
                    {
                        velocityY = -BALL_SPEED_LIMIT;
                    }
                }
                return OFF_PADDLE;
            }
            else
            {
                return AI_WIN;
            }
        }
        
        if(bouncedOnWall)
        {
            return OFF_WALL;
        }
        else
        {
            return NO_BOUNCE;
        }
    }
}

class Game
{
    Paddle* paddleL;
    Paddle* paddleR;
    Ball*   ball;

private:

public:
    Game()
    {
    }
    ~Game()
    {   
    }

    int play()
    {
        //initialize and start game
        bool end = 0;
        paddleL = new Paddle(0);
        paddleR = new Paddle(1);
        ball = new Ball(paddleL, paddleR);
        paddleL->initialize(ball);
        paddleR->initialize(ball);
        while(!end)
        {

        }

        //clean up objects
        delete paddleL;
        delete paddleR;
        delete ball;
    }
}
