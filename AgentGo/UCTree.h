#pragma once

#include "AgentGo.h"
#include "Board.h"


#define UCT_WIDTH 3

// when constructing a UCNode, one must assign its score,is_max
struct UCNode
{
	    Piece move;
		Board bd;
		double score;
		bool is_max;
		bool is_leaf;
		int player;
		UCNode* children[UCT_WIDTH];
		UCNode* parent;
		UCNode();
		double minMax();
		void expand();
		void clear();
		void initBoard();
};

class UCTree
{
public:
	UCNode root;
	UCTree(bool is_max, int player,const Board &bd);
	~UCTree(void);
	Piece getBestMove();
	void clear();

};

