#pragma once

#include "AgentGo.h"
#include "Piece.h"
#include "SetNode.h"
#include <memory.h>
#include <stack>
#include <list>
#include <random>

#define MAX_HISTORY_LENGTH	1000
#define SPLIT_SIZE_LARGE	6	// numbers of rows in a space_split_lg;
#define SPLIT_NUM_LARGE     ( BOARD_SIZE / SPLIT_SIZE_LARGE )	// length of space_split_lg;

using namespace std;

class Board
{
private:
	// Variables
	SetNode set_nodes[BOARD_SIZE*BOARD_SIZE];
	
	// Functions
	SetNode* getSet(int row, int col);
	void unionSetNode(SetNode* s1, SetNode* s2);
	void killSetNode(SetNode* sn);

public:
	// Variables
	int	   data[BOARD_SIZE][BOARD_SIZE];

	int    num_black;
	int    num_white;
	int    space_split_sm[BOARD_SIZE];	// records the space of every row
	int	   space_split_lg[SPLIT_NUM_LARGE];   // records the space of the 0-5 and 6-11 row,not including the 12th row.

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
	Piece getPiece(int row,int col);
	void print();
	void clone(const Board &board);
	Piece getRandomPiece(int agent);
	void release(); // delete the array pointers
	

	/* old ones
	int** rcsv_flags;	// the matrix used to mark points when runing DFS recursive functions and calculate hp
	int   hp_map[BOARD_SIZE][BOARD_SIZE];
	
	void initRcsvFlags();
	inline void updateHp(int row, int col);
	inline void updateHp(const Piece &piece);
	int getHpRecursive(int row, int col);
	void setHpRecursive(int row, int col, const int hp);
	*/
};

