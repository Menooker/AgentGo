#pragma once
#include "AgentGo.h"
#include "Piece.h"
#include <memory.h>

#define UNKNOWN_IDX  999
#define SET_STATIC_SIZE (BOARD_SIZE*BOARD_SIZE)		// enlarging this number will improve efficiency but requires much more space


struct SetNode
{
public:
	int pnode;	// index of parent node
	int hp;
	int deepth;
	int size;
	
	Piece pieces[SET_STATIC_SIZE]; // do not use this one publically; 
		// initally pointo to pieces_stat[], but if use dynamic, new Piece[169]

	SetNode(void);
	~SetNode(void);
	const SetNode &operator=(const SetNode &sn );
	void init(int idx, Piece piece, int hp);
	void clear();	// a setnode is cleared when th piece is killed
	void drop();	// a setnode is dropped when it is unioned to another
	void push_back(Piece piece);
	void merge_pieces(const SetNode &sn);
};

