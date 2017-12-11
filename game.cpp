#include <stdio.h>
#include <vector>
#include <random>
#include <assert.h>
#include <time.h>

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
#define BALL_SPEED_LIMIT 0.04
#define BOARD_DISCRETIZATION_X 12
#define BOARD_DISCRETIZATION_Y 12
#define NUM_PADDLE_POS 12

#define EPSILON 0.2
#define GAMMA 0.9
#define LEARNING_RATE_CONST 200
#define TRAIN_TRIALS 1000

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
	OFF_LPADDLE,
	OFF_RPADDLE,
	AI_WIN,
	RL_AI_WIN,
	NUM_BOUNCE_TYPE
};

//RL_AI on the right side of board
enum PLAYER_TYPE
{
	RL_AI,
	AI,
	TRAINER_AI,
	NUM_AI
};

enum rlBallVelocityX
{
	BALL_HEADING_RIGHT,
	BALL_HEADING_LEFT,
	NUM_BALL_VELOCITY_X
};

enum rlBallVelocityY
{
	BALL_HEADING_UP,
	BALL_HEADING_DOWN,
	BALL_HEADING_HORIZONTAL,
	NUM_BALL_VELOCITY_Y
};

enum RlMode
{
	TRAINING,
	PLAYING,
	NUM_RL_MODE
};

//struct BallState
//{
//	float posX;
//	float posY;
//	float velocityX;
//	float velocityY;
//};


struct WorldState
{
	int ballPosX;
	int ballPosY;
	int ballVelocityX;
	int ballVelocityY;
	int paddlePosY;
};

struct WorldStatus
{
	float ballPosX;
	float ballPosY;
	float ballVelocityX;
	float ballVelocityY;
	float paddlePosY;

	void initialize()
	{
		ballPosX = 0;
		ballPosY = 0;
		ballVelocityX = 0;
		ballVelocityY = 0;
		paddlePosY = 0;
	}

	bool empty()
	{
		return (ballPosX == 0 && ballPosY == 0 && ballVelocityX == 0 && ballVelocityY == 0 && paddlePosY == 0);
	}

	void operator= (const WorldStatus& worldStatus)
	{
		ballPosX = worldStatus.ballPosX;
		ballPosY = worldStatus.ballPosY;
		ballVelocityX = worldStatus.ballVelocityX;
		ballVelocityY = worldStatus.ballVelocityY;
		paddlePosY = worldStatus.paddlePosY;
	}
};

class Paddle
{
private:
	float epsion; // epsilon percent of chance to choose random action in training
	float gamma;  // discount factor for each new utility at state S'
	WorldStatus oldWorldStatus; // used for training. Stores world status of previous time frame
	ACTION oldAction;  //// used for training. Stores the action taken in previous time frame
public:
	PLAYER_TYPE playerType;
	bool isTraining; //set this flag to train
	float posX;
	float posY;
	float****** Q; // utility table
	int  ****** N; // num of action taken under certain state (a table)

	void reset()
	{
		posY = 0.5 - (PADDLE_HEIGHT / 2) ;
	}

	Paddle(float XPos, PLAYER_TYPE newPlayerType) :
		playerType(newPlayerType),
		posX(XPos),
		posY(0.5 - (PADDLE_HEIGHT / 2)),
		gamma(GAMMA),
		isTraining(false)
		{
			srand(time(NULL));
			epsion = EPSILON;
			oldWorldStatus.initialize();

			//allocate memory for Q and N
			Q = (float******)malloc(BOARD_DISCRETIZATION_X*sizeof(float*****));
			N = (int******)malloc(BOARD_DISCRETIZATION_X * sizeof(int*****));
			if (!Q)
			{
				printf("out of memory Q\n");
				exit(EXIT_FAILURE);
			}
			if (!N)
			{
				printf("out of memory N\n");
				exit(EXIT_FAILURE);
			}
			for (int ballX = 0; ballX < BOARD_DISCRETIZATION_X; ballX++)
			{
				Q[ballX] = (float*****)malloc(BOARD_DISCRETIZATION_Y * sizeof(float****));
				N[ballX] = (int  *****)malloc(BOARD_DISCRETIZATION_Y * sizeof(int  ****));
				if (!Q[ballX])
				{
					printf("out of memory Q[%d]\n", ballX);
					exit(EXIT_FAILURE);
				}
				if (!N[ballX])
				{
					printf("out of memory N[%d]\n", ballX);
					exit(EXIT_FAILURE);
				}
				for (int ballY = 0; ballY < BOARD_DISCRETIZATION_Y; ballY++)
				{
					Q[ballX][ballY] = (float****)malloc(NUM_BALL_VELOCITY_X * sizeof(float***));
					N[ballX][ballY] = (int  ****)malloc(NUM_BALL_VELOCITY_X * sizeof(int  ***));
					if (!Q[ballX][ballY])
					{
						printf("out of memory Q[%d][%d]\n", ballX, ballY);
						exit(EXIT_FAILURE);
					}
					if (!N[ballX][ballY])
					{
						printf("out of memory N[%d][%d]\n", ballX, ballY);
						exit(EXIT_FAILURE);
					}
					for (int ballVelocityX = 0; ballVelocityX < NUM_BALL_VELOCITY_X; ballVelocityX++)
					{
						Q[ballX][ballY][ballVelocityX] = (float***)malloc(NUM_BALL_VELOCITY_Y * sizeof(float**));
						N[ballX][ballY][ballVelocityX] = (int  ***)malloc(NUM_BALL_VELOCITY_Y * sizeof(int  **));
						if (!Q[ballX][ballY][ballVelocityX])
						{
							printf("out of memory Q[%d][%d][%d]\n", ballX, ballY, ballVelocityX);
							exit(EXIT_FAILURE);
						}
						if (!N[ballX][ballY][ballVelocityX])
						{
							printf("out of memory N[%d][%d][%d]\n", ballX, ballY, ballVelocityX);
							exit(EXIT_FAILURE);
						}
						for (int ballVelocityY = 0; ballVelocityY < NUM_BALL_VELOCITY_Y; ballVelocityY++)
						{
							Q[ballX][ballY][ballVelocityX][ballVelocityY] = (float**)malloc(NUM_PADDLE_POS * sizeof(float*));
							N[ballX][ballY][ballVelocityX][ballVelocityY] = (int  **)malloc(NUM_PADDLE_POS * sizeof(int  *));
							if (!Q[ballX][ballY][ballVelocityX][ballVelocityY])
							{
								printf("out of memory Q[%d][%d][%d][%d]\n", ballX, ballY, ballVelocityX, ballVelocityY);
								exit(EXIT_FAILURE);
							}
							if (!N[ballX][ballY][ballVelocityX][ballVelocityY])
							{
								printf("out of memory N[%d][%d][%d][%d]\n", ballX, ballY, ballVelocityX, ballVelocityY);
								exit(EXIT_FAILURE);
							}
							for (int paddlePos = 0; paddlePos < NUM_PADDLE_POS; paddlePos++)
							{
								Q[ballX][ballY][ballVelocityX][ballVelocityY][paddlePos] = (float*)malloc(NUM_ACTION * sizeof(float));
								N[ballX][ballY][ballVelocityX][ballVelocityY][paddlePos] = (int  *)malloc(NUM_ACTION * sizeof(int));
								if (!Q[ballX][ballY][ballVelocityX][ballVelocityY][paddlePos])
								{
									printf("out of memory Q[%d][%d][%d][%d][%d]\n", ballX, ballY, ballVelocityX, ballVelocityY, paddlePos);
									exit(EXIT_FAILURE);
								}
								if (!N[ballX][ballY][ballVelocityX][ballVelocityY][paddlePos])
								{
									printf("out of memory N[%d][%d][%d][%d][%d]\n", ballX, ballY, ballVelocityX, ballVelocityY, paddlePos);
									exit(EXIT_FAILURE);
								}
							}

						}
					}
				}
			}

			//zero memory for Q and N
			for (int ballX = 0; ballX < BOARD_DISCRETIZATION_X; ballX++)
			{
				for (int ballY = 0; ballY < BOARD_DISCRETIZATION_Y; ballY++)
				{
					for (int ballVelocityX = 0; ballVelocityX < int(NUM_BALL_VELOCITY_X); ballVelocityX++)
					{
						for (int ballVelocityY = 0; ballVelocityY < int(NUM_BALL_VELOCITY_Y); ballVelocityY++)
						{
							for (int paddlePos = 0; paddlePos < NUM_PADDLE_POS; paddlePos++)
							{
								for (int action = 0; action < (NUM_ACTION); action++)
								{
									Q[ballX][ballY][ballVelocityX][ballVelocityY][paddlePos][action] = 0;
									N[ballX][ballY][ballVelocityX][ballVelocityY][paddlePos][action] = 0;
								}
							}
						}
					}
				}
			}

			////zero memory for Q and N
			//for (int ballX = 0; ballX < 12; ballX++)
			//{
			//	for (int ballY = 0; ballY < 12; ballY++)
			//	{
			//		for (int ballVelocityX = 0; ballVelocityX < 2; ballVelocityX++)
			//		{
			//			for (int ballVelocityY = 0; ballVelocityY < 3; ballVelocityY++)
			//			{
			//				for (int paddlePos = 0; paddlePos < 12; paddlePos++)
			//				{
			//					for (int action = 0; action < 3; action++)
			//					{
			//						Q[ballX][ballY][ballVelocityX][ballVelocityY][paddlePos][action] = 0;
			//						N[ballX][ballY][ballVelocityX][ballVelocityY][paddlePos][action] = 0;
			//					}
			//				}
			//			}
			//		}
			//	}
			//}
		}

	~Paddle()
	{
		for (int ballX = 0; ballX < BOARD_DISCRETIZATION_X; ballX++)
		{
			for (int ballY = 0; ballY < BOARD_DISCRETIZATION_Y; ballY++)
			{
				for (int ballVelocityX = 0; ballVelocityX < NUM_BALL_VELOCITY_X; ballVelocityX++)
				{
					for (int ballVelocityY = 0; ballVelocityY < NUM_BALL_VELOCITY_Y; ballVelocityY++)
					{
						for (int paddlePos = 0; paddlePos < NUM_PADDLE_POS; paddlePos++)
						{
							free(Q[ballX][ballY][ballVelocityX][ballVelocityY][paddlePos]);
							free(N[ballX][ballY][ballVelocityX][ballVelocityY][paddlePos]);
						}
						free(Q[ballX][ballY][ballVelocityX][ballVelocityY]);
						free(N[ballX][ballY][ballVelocityX][ballVelocityY]);
					}
					free(Q[ballX][ballY][ballVelocityX]);
					free(N[ballX][ballY][ballVelocityX]);
				}
				free(Q[ballX][ballY]);
				free(N[ballX][ballY]);
			}
			free(Q[ballX]);
			free(N[ballX]);
		}
		free(Q);
		free(N);
	}

	inline int ballPosXState(float x)
	{
		int state = int(x / (float(BOARD_WIDTH) / float(BOARD_DISCRETIZATION_X)));
		if (state >= BOARD_DISCRETIZATION_X)
		{
			state = BOARD_DISCRETIZATION_X - 1;
		}
		return state;
	}

	inline int ballPosYState(float y)
	{
		int state = int(y / (float(BOARD_WIDTH) / float(BOARD_DISCRETIZATION_Y)));
		if (state >= BOARD_DISCRETIZATION_Y)
		{
			state = BOARD_DISCRETIZATION_Y - 1;
		}
		return state;
	}

	inline int ballVelXState(float x)
	{
		//should have been (x < 0) ? -1 : 1
		//but returned value is used to access array. -1 and 1 need to be mapped to 0 and 1
		return ((x < 0) ? 0 : 1);
	}

	inline int ballVelYState(float x)
	{
		//should have been (x < 0) ? -1 : ((x > 0) ? 1 : 0
		//but returned value is used to access array. -1, 0 and 1 need to be mapped to 0, 1 and 2
		return ((x < 0) ? 0 : ((x > 0) ? 2 : 1));
	}

	inline int paddlePosState(float y)
	{
		int state = int(y / (float(BOARD_WIDTH) / float(BOARD_DISCRETIZATION_Y)));
		if (state >= BOARD_DISCRETIZATION_Y)
		{
			state = BOARD_DISCRETIZATION_Y - 1;
		}
		return state;
	}

	inline float getUtility(WorldStatus* worldStatus, ACTION action)
	{
		WorldState worldState = {	ballPosXState(worldStatus->ballPosX),
									ballPosYState(worldStatus->ballPosY),
									ballVelXState(worldStatus->ballVelocityX),
									ballVelYState(worldStatus->ballVelocityY),
									paddlePosState(worldStatus->paddlePosY) };
		return Q[ballPosXState(worldStatus->ballPosX)]
				[ballPosYState(worldStatus->ballPosY)]
				[ballVelXState(worldStatus->ballVelocityX)]
				[ballVelYState(worldStatus->ballVelocityY)]
				[paddlePosState(worldStatus->paddlePosY)]
				[int(action)];
	}

	inline float getStateActionCount(WorldStatus* worldStatus, ACTION action)
	{
		return N[ballPosXState(worldStatus->ballPosX)]
				[ballPosYState(worldStatus->ballPosY)]
				[ballVelXState(worldStatus->ballVelocityX)]
				[ballVelYState(worldStatus->ballVelocityY)]
				[paddlePosState(worldStatus->paddlePosY)]
				[int(action)];
	}

	inline void incrementStateActionCount(WorldStatus* worldStatus, ACTION action)
	{
		 N[ballPosXState(worldStatus->ballPosX)]
			[ballPosYState(worldStatus->ballPosY)]
			[ballVelXState(worldStatus->ballVelocityX)]
			[ballVelYState(worldStatus->ballVelocityY)]
			[paddlePosState(worldStatus->paddlePosY)]
			[int(action)]++;
	}

	void makeMove(WorldStatus* worldStatus)
	{

		if (playerType == TRAINER_AI)
		{
			posY = worldStatus->ballPosY - (PADDLE_HEIGHT / 2);
		}
		else if (playerType == AI)
		{
			if (posY + (PADDLE_HEIGHT / 2) > worldStatus->ballPosY)
			{
				move(UP, AI_PADDLE_SPEED);
			}
			else if (posY + (PADDLE_HEIGHT / 2) < worldStatus->ballPosY)
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
			 
			 if (!isTraining)
			 {
				 ACTION newAction;
				 newAction = rlChooseAction(worldStatus, PLAYING);
				 move(newAction, RL_AI_PADDLE_SPEED);
			 }
			 else
			 {
				 ACTION newAction;
				 //new action is the actual action taken, may be a random action depending on exploration policy. This action is used as the oldAction for training in the next time frame 
				 newAction = rlChooseAction(worldStatus, TRAINING);
				 // we have record of old world status, can train using this new world status. oldWorldStatus is empty in the very first iteration
				 if (!oldWorldStatus.empty())
				 {
					 rlLearn(worldStatus);
				 }
				 move(newAction, RL_AI_PADDLE_SPEED);
				 incrementStateActionCount(worldStatus, newAction);
				 oldAction = newAction;
				 oldWorldStatus = *worldStatus;
			 }
			 
		 }
	}

	float getReward(WorldStatus* worldStatus)
	{
		float newBallPosX = worldStatus->ballPosX + worldStatus->ballVelocityX;
		float newBallPosY = worldStatus->ballPosY + worldStatus->ballVelocityY;
		if (newBallPosX > 1)
		{
			if ((newBallPosY >= posY) && (newBallPosY <= (posY + PADDLE_HEIGHT)))
			{
				return 1;
			}
			else
			{
				return -1;
			}
		}
		else
		{
			return 0;
		}
	}

	//happens when we are in state S' already.
	void rlLearn(WorldStatus* newWorldStatus)
	{
		float reward = getReward(&oldWorldStatus);
		// use max-utility action as Q(s',a') to train
		ACTION knownBestAction = rlChooseAction(newWorldStatus, PLAYING);
		float newQ = getUtility(newWorldStatus, knownBestAction); //Q(s',a')
		float oldQ = getUtility(&oldWorldStatus, oldAction);	  //Q(s,a)
		float alpha = LEARNING_RATE_CONST/(LEARNING_RATE_CONST + getStateActionCount(&oldWorldStatus,oldAction));  // learning rate factor. Changes during the training

		Q[ballPosXState(oldWorldStatus.ballPosX)]
		[ballPosYState(oldWorldStatus.ballPosY)]
		[ballVelXState(oldWorldStatus.ballVelocityX)]
		[ballVelYState(oldWorldStatus.ballVelocityY)]
		[paddlePosState(oldWorldStatus.paddlePosY)]
		[int(oldAction)] = oldQ + alpha * (reward + gamma * newQ - oldQ);
	}

	ACTION rlChooseAction(WorldStatus* worldStatus, RlMode rlMode)
	{
		ACTION action;
		if (rlMode == TRAINING)
		{
			// epsilon percent of chance to choose random action
			float randomNum = (float)rand() / (float)RAND_MAX;
			if (randomNum < epsion)
			{
				action = (ACTION)(rand() % 3);
			}
			else
			{
				action = rlMaxAction(worldStatus);
			}
			
			return action;
		}
		else
		{
			return rlMaxAction(worldStatus);
		}
	}

	

	ACTION rlMaxAction(WorldStatus* worldStatus)
	{
		ACTION maxAction = STAY;
		float maxUtility = -9999;
		for (int action = 0; action < NUM_ACTION; action++)
		{
			if (getUtility(worldStatus, ACTION(action)) > maxUtility)
			{
				maxAction = (ACTION)action;
				maxUtility = getUtility(worldStatus, ACTION(action));
			}
		}
		return maxAction;
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

	void reset()
	{
		posX = 0.5;
		posY = 0.5;
		velocityX = 0.03;
		velocityY = 0.01;
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
				return OFF_LPADDLE;
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
				return OFF_RPADDLE;
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
		srand(time(NULL));
	}
	~Game()
	{
	}

	void initialize(PLAYER_TYPE lPlayerType, PLAYER_TYPE rPlayerType)
	{
		//initialize and start game
		paddleL = new Paddle(0, lPlayerType);
		paddleR = new Paddle(1, rPlayerType);
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

	void reset()
	{
		gameOver = false;
		paddleL->reset();
		paddleR->reset();
		ball->reset();
	}

	void trainRPaddle()
	{
		// BOUNCE_TYPE frameResult = NO_BOUNCE;
		paddleR->isTraining = true;
		for (int trainTrial = 0; trainTrial < TRAIN_TRIALS; trainTrial++)
		{
			reset();
			while(!gameOver)
			{ 
				playOneFrame();
			}
		}
		paddleR->isTraining = false;
	}

	BOUNCE_TYPE playOneFrame()
	{
		BOUNCE_TYPE frameResult;
		WorldStatus worldStatusL = {ball->posX, ball->posY, ball->velocityX, ball->velocityY, paddleL->posY};
		WorldStatus worldStatusR = {ball->posX, ball->posY, ball->velocityX, ball->velocityY, paddleR->posY};
		//while (!gameOver)
		//{
			paddleL->makeMove(&worldStatusL);
			paddleR->makeMove(&worldStatusR);
			frameResult = ball->move();
			if (frameResult == AI_WIN )
			{
				gameOver = true;
				//printf("left Win\n");
			}

			if (frameResult == RL_AI_WIN)
			{
				gameOver = true;
				//printf("right Win\n");
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
	float numRounds = 1;
	float avgRebounce = 0;
	float numRebounceThisRound = 0;
	BOUNCE_TYPE frameResult;
	Game* game = new Game();
	game->initialize(TRAINER_AI, RL_AI);
	game->trainRPaddle();
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
	glfwSwapInterval(2);

	while (!glfwWindowShouldClose(window))
	{
		game->reset();
		numRebounceThisRound = 0;
		while (!game->gameOver && !glfwWindowShouldClose(window))
		{
			frameResult = game->playOneFrame();
			if (frameResult == OFF_RPADDLE)
			{
				numRebounceThisRound++;
			}
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
		avgRebounce = avgRebounce + float(1 / numRounds)*(numRebounceThisRound - avgRebounce);
		numRounds++;
	}
	printf("avg Rebounce: %f\n", avgRebounce);
	game->destroy();
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
