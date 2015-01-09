#pragma once

#include "AgentGo.h"
#include "Board.h"


#define UCT_WIDTH 5

struct UCNode
{
	    Piece move;
		Board bd;
		int score;
		bool is_max;
		UCNode* children[UCT_WIDTH];
		UCNode(){
			for( int i=0; i<UCT_WIDTH; i++ ){
				children[i] = 0;
			}
		};
		void evaluate(){};
		void minMax() {};
}

class UCTree
{
public:
	int deepth;
	UCNode root;
	UCTree(void);
	~UCTree(void);
	void init(const Board &bd, int width, int depth);
};

