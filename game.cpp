#include <stdio.h>
#include <vector>
#include <random>
#include <assert.h>

#include <GLFW\glfw3.h>
#include "linmath.h"
#include <stdlib.h>


#define SCREEN_STRIDE 2
#define SCREEN_MIN -1
#define SCREEN_MAX 1
#define GAME_BOARD_STRIDE 1
#define PADDLE_WIDTH 0.03
#define PADDLE_HEIGHT_IN_WINDOW 0.4

#define BOARD_HEIGHT 1
#define BOARD_WIDTH 1
#define PADDLE_HEIGHT 0.2
#define AI_PADDLE_SPEED 0.02
#define RL_AI_PADDLE_SPEED 0.04
#define HI_RANDOM_VELOCITY_X 0.015
#define LO_RANDOM_VELOCITY_X -0.015
#define HI_RANDOM_VELOCITY_Y 0.03
#define LO_RANDOM_VELOCITY_Y -0.03
#define BALL_SPEED_LIMIT 0.02


enum ACTION
{
    STAY,
    UP,
    DOWN,
    NUM_ACTION
};

enum BOUNCE_TYPE
{
    NO_BOUNCE,
    OFF_WALL,
    OFF_PADDLE,
    AI_WIN,
    RL_AI_WIN,
    NUM_BOUNCE_TYPE
};

enum PLAYER_TYPE
{
    RL_AI,
    AI,
    TRAINER_AI,
    NUM_AI
};

struct BallState
{
    float posX;
    float posY;
    float velocityX;
    float velocityY;
};

class Paddle
{
public:
    Paddle(float XPos) :
        posX(XPos),
        posY(0.5 - (PADDLE_HEIGHT / 2))
        {}

    float posX;
    float posY;
    std::vector<std::vector<float>> Q;

    void initializeAgent(std::vector<std::vector<float>> new_Q)
    {
        Q = new_Q;
    }

    void chooseMove(PLAYER_TYPE playerType, BallState* ballState)
    {
        if (playerType == TRAINER_AI)
        {
            posY = ballState->posY - (PADDLE_HEIGHT / 2);
        }
        if (playerType == AI)
        {
            if (posY + (PADDLE_HEIGHT / 2) > ballState->posY)
            {
                move(UP, AI_PADDLE_SPEED);
            }
            else if (posY + (PADDLE_HEIGHT / 2) < ballState->posY)
            {
                move(DOWN, AI_PADDLE_SPEED);
            }
            else
            {
                move(STAY, AI_PADDLE_SPEED);
            }
        }
        // else if(playerType == RL_AI)
        // {

        // }
    }

private:
    void move(ACTION action, float velocity)
    {
        switch (action)
        {
        case(UP):
        {
            posY -= velocity;
            if (posY < 0)
            {
                posY = 0;
            }
            break;
        }
        case(DOWN):
        {
            posY += velocity;
            if (posY >(1 - PADDLE_HEIGHT))
            {
                posY = 1 - PADDLE_HEIGHT;
            }
            break;
        }
        default:
            return;
        }
    }
};

class Ball
{
public:
    float posX;
    float posY;
    float velocityX;
    float velocityY;
    Paddle* paddleL;
    Paddle* paddleR;

    Ball(Paddle* new_paddleL, Paddle* new_paddleR)
    {
        posX = 0.5;
        posY = 0.5;
        velocityX = 0.03;
        velocityY = 0.01;
        paddleL = new_paddleL;
        paddleR = new_paddleR;
    }

    BOUNCE_TYPE move()
    {
        // if bounced both on wall and paddle in same time frame, OFF_WALL is ignored.
        bool bouncedOnWall = 0;

        posY += velocityY;
        if (posY < 0)
        {
            posY = -posY;
            velocityY = -velocityY;
            bouncedOnWall = true;
        }
        else if (posY > 1)
        {
            posY = 2 - posY;
            velocityY = -velocityY;
            bouncedOnWall = true;
        }

        posX += velocityX;
        // ball reaches left end
        if (posX < 0)
        {
            if ((posY >= paddleL->posY) && (posY <= (paddleL->posY + PADDLE_HEIGHT)))
            {
                posX = -posX;
                float velocityVariationX = LO_RANDOM_VELOCITY_X +
                    static_cast <float> (rand()) /
                    (static_cast <float> (RAND_MAX / (HI_RANDOM_VELOCITY_X - LO_RANDOM_VELOCITY_X)));

                float velocityVariationY = LO_RANDOM_VELOCITY_Y +
                    static_cast <float> (rand()) /
                    (static_cast <float> (RAND_MAX / (HI_RANDOM_VELOCITY_Y - LO_RANDOM_VELOCITY_Y)));

                velocityX = -velocityX + velocityVariationX;
                assert(velocityX >= 0);
                // if ball is moving too slow on  X axis
                if (velocityX < HI_RANDOM_VELOCITY_X)
                {
                    velocityX = HI_RANDOM_VELOCITY_X;
                }
                // if ball is moving too fast on X axis
                else if (velocityX > BALL_SPEED_LIMIT)
                {
                    velocityX = BALL_SPEED_LIMIT;
                }
                velocityY = velocityY + velocityVariationY;
                // if ball is moving too fast on Y axis
                if (velocityY > BALL_SPEED_LIMIT || velocityY < -BALL_SPEED_LIMIT)
                {
                    if (velocityY > 0)
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
        if (posX > 1)
        {
            if ((posY >= paddleR->posY) && (posY <= (paddleR->posY + PADDLE_HEIGHT)))
            {
                posX = 2 - posX;
                float velocityVariationX = LO_RANDOM_VELOCITY_X +
                    static_cast <float> (rand()) /
                    (static_cast <float> (RAND_MAX / (HI_RANDOM_VELOCITY_X - LO_RANDOM_VELOCITY_X)));

                float velocityVariationY = LO_RANDOM_VELOCITY_Y +
                    static_cast <float> (rand()) /
                    (static_cast <float> (RAND_MAX / (HI_RANDOM_VELOCITY_Y - LO_RANDOM_VELOCITY_Y)));

                velocityX = -velocityX + velocityVariationX;
                assert(velocityX <= 0);
                // if ball is moving too slow on  X axis
                if (velocityX > -HI_RANDOM_VELOCITY_X)
                {
                    velocityX = -HI_RANDOM_VELOCITY_X;
                }
                // if ball is moving too fast on X axis
                else if (velocityX < -BALL_SPEED_LIMIT)
                {
                    velocityX = -BALL_SPEED_LIMIT;
                }
                velocityY = velocityY + velocityVariationY;
                // if ball is moving too fast on Y axis
                if (velocityY > BALL_SPEED_LIMIT || velocityY < -BALL_SPEED_LIMIT)
                {
                    if (velocityY > 0)
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

        if (bouncedOnWall)
        {
            return OFF_WALL;
        }
        else
        {
            return NO_BOUNCE;
        }
    }
};

class Game
{

public:
    bool gameOver;
    Paddle* paddleL;
    Paddle* paddleR;
    Ball*   ball;

    Game()
    {
    }
    ~Game()
    {
    }

    void initialize()
    {
        //initialize and start game
        paddleL = new Paddle(0);
        paddleR = new Paddle(1);
        ball = new Ball(paddleL, paddleR);
        gameOver = false;
    }

    void destroy()
    {
        //clean up objects
        delete paddleL;
        delete paddleR;
        delete ball;
    }

    BOUNCE_TYPE playOneFrame()
    {
        BOUNCE_TYPE frameResult;
        BallState ballState = {ball->posX, ball->posY, ball->velocityX, ball->velocityY};
        //while (!gameOver)
        //{
            paddleL->chooseMove(TRAINER_AI, &ballState);
            paddleR->chooseMove(AI, &ballState);
            frameResult = ball->move();
            if (frameResult == AI_WIN )
            {
                gameOver = true;
                printf("left Win\n");
            }

            if (frameResult == RL_AI_WIN)
            {
                gameOver = true;
                printf("right Win\n");
            }
        //}
            return frameResult;
    }
};

float gameBoardToWindowPosX(float gameBoardPos)
{
    float windowPos = (gameBoardPos / GAME_BOARD_STRIDE) * SCREEN_STRIDE + SCREEN_MIN;
    return windowPos;
}

float gameBoardToWindowPosY(float gameBoardPos)
{
    float windowPos = -((gameBoardPos / GAME_BOARD_STRIDE) * SCREEN_STRIDE + SCREEN_MIN);
    return windowPos;
}

void renderRPaddle(float paddlePosY)
{
    float screenRPaddlePosY = gameBoardToWindowPosY(paddlePosY);
    glBegin(GL_POLYGON);
    //lower right
    glVertex2f(SCREEN_MAX, screenRPaddlePosY - PADDLE_HEIGHT_IN_WINDOW);
    //lower left
    glVertex2f(SCREEN_MAX - PADDLE_WIDTH, screenRPaddlePosY - PADDLE_HEIGHT_IN_WINDOW);
    //upper left
    glVertex2f(SCREEN_MAX - PADDLE_WIDTH, screenRPaddlePosY);
    //upper right
    glVertex2f(SCREEN_MAX, screenRPaddlePosY);
    glEnd();
}

void renderLPaddle(float paddlePos)
{
    float screenLPaddlePosY = gameBoardToWindowPosY(paddlePos);
    glBegin(GL_POLYGON);
    //lower right
    glVertex2f(SCREEN_MIN + PADDLE_WIDTH, screenLPaddlePosY - PADDLE_HEIGHT_IN_WINDOW);
    //lower left
    glVertex2f(SCREEN_MIN, screenLPaddlePosY - PADDLE_HEIGHT_IN_WINDOW);
    //upper left
    glVertex2f(SCREEN_MIN, screenLPaddlePosY);
    //upper right
    glVertex2f(SCREEN_MIN + PADDLE_WIDTH, screenLPaddlePosY);
    glEnd();
}

void renderBall(float x, float y)
{
    const float DEG2RAD = 3.14159 / 180;
    float ballRadius = 0.02;

    glBegin(GL_POLYGON);
    //for each degree of the circle, we draw a point
    for (int i = 0; i < 360; i++)
    {
        //convert degree to radians
        float degInRad = i * DEG2RAD; //DEG2RAD : how much radians is 1 degree
                                      //draw a dot on screen given x(1st argument) and y(second argument) coordinat
        glVertex2f(cos(degInRad) * ballRadius + gameBoardToWindowPosX(x), sin(degInRad) * ballRadius + gameBoardToWindowPosY(y));
    }
    glEnd();
}

int main(void) 
{
    Game* game = new Game();
    game->initialize();
    // initialize glfw and exit if failed
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    //telling he program the contextual information about openGL version
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    GLFWwindow* window = glfwCreateWindow(480, 480, "OpenGLtest", NULL, NULL);
    if (!window) 
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);
    // the refresh between each frame. Setting it to 1 introduces a little delay for vSync
    glfwSwapInterval(1);

    while (!game->gameOver && !glfwWindowShouldClose(window))
    {
        game->playOneFrame();
        //Setup View
        float ratio;
        int width, height;
        //this function figures out the width and height of the window and set these variables
        glfwGetFramebufferSize(window, &width, &height);
        ratio = (float)width / height;
        //set up view port
        glViewport(0, 0, width, height);
        // GL_COLOR_BUFFER_BIT stores the color info of what's drawn on screen. glClear() clears this info
        glClear(GL_COLOR_BUFFER_BIT);

        //drawing.
        //from left to right of the window, the interval is [-1,1] . Similar for up and down
        
        // To draw a n sides GL_POLYGON on 2D surface, need to call glVertex2f() n times specifying the vertices of the polygon in some order
        //This example shows an example of a rectangle
        renderBall(game->ball->posX, game->ball->posY);
        renderLPaddle(game->paddleL->posY);
        renderRPaddle(game->paddleR->posY);

        //swap the front and back buffers of the specified window
        glfwSwapBuffers(window);
        //check for events
        glfwPollEvents();
    }
    game->destroy();
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}