#pragma once

#include "AgentGo.h"
#include "Piece.h"
#include "SetNode.h"
#include <memory.h>
#include <time.h>
#include <stack>

#define MAX_HISTORY_LENGTH	1000

#define SPACE_HEAD_NULL		(-1)
#define ENDING_THRESHOLD    100

using namespace std;

class Board
{
private:
	// Variables
	inline int getIdxS(int row,int col);  // safely get idx
	SetNode set_nodes[BOARD_SIZE*BOARD_SIZE];
	int distance[BOARD_SIZE*BOARD_SIZE][2];
	bool game_ending;

	// Functions
	inline SetNode* getSet(int row, int col);
	void unionSetNode(SetNode* s1, SetNode* s2);
	void killSetNode(SetNode* sn);
	inline void addReserve(int agent, int row, int col);
	inline void resetReserve();
	inline bool checkKeyPlace(int agent,int row,int col);
	inline bool checkGoodPlace(int agent,int row,int col);
	inline bool checkNeighbour(int agent,int row,int col);
	inline bool checkChase(int agent,int row,int col);
	void updateDistance(int agent,int row,int col);
	void setDistance(int agent,int row,int col,int dist);
	inline bool checkDistFar(int agent,int row,int col);

public:
	// Variables
	int	   data[BOARD_SIZE][BOARD_SIZE];
	int    num_black;
	int    num_white;
	int    space_head;
	int    space_list[BOARD_SIZE*BOARD_SIZE][2];
	bool   reserve[BOARD_SIZE*BOARD_SIZE];
	int    reserve_total;	// total number of the reserved place for each
	int    true_eyes[2];
	bool   exist_compete;
	int    compete[3];	// agent, row, col, agent is the one who has just been killed i.e. the one who could not put in this place
	Piece  last_move[2];


#ifdef GO_BOARD_TIME
	long   time_put;
	long   time_kill;
	long   time_random1;
	long   time_random2;
	long   time_getset;
#endif

#ifdef GO_HISTORY
	Piece  history[MAX_HISTORY_LENGTH]; // pointers point to the head piece in data
	int    history_head;
#endif

	// Functions
	Board(void);
	Board(const Board *board);
	Board(const Board &board);
	~Board(void);
	void clear();	// delete all the pieces
	bool put(int agent,int row,int col);
	bool put(const Piece &piece);
	void pass(int agent);
	inline Piece getPiece(int row, int col);
	inline bool checkPiece(int agent, int row, int col);
	void print();
	void clone(const Board &board);
	Piece getRandomPiece(int agent);
	Piece getRandomPieceComplex(int agent);
	bool checkTrueEye(int agent, int row, int col);
	bool checkSuicide(int agent, int row, int col);
	bool checkSurvive(int agent, int row, int col);
	bool checkDying(int agent, int row, int col);
	bool checkKill(int agent, int row, int col);
	bool checkNoSense(int agent, int row, int col);
	int countScore(int agent);
	inline bool checkCompete(int agent, int row, int col);	// returns true if this place not not allowed for the agent to put ( reserved fot the enemy )
	void release(); // delete the array pointers
	
};

