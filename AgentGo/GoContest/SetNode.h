#pragma once
#include "AgentGo.h"
#include "Piece.h"
#include <list>

#define UNKNOWN_IDX  999

using namespace::std;

struct SetNode
{
public:
	int pnode;	// index of parent node
	int hp;
	int deepth;
	list<Piece>* pieces;

	SetNode(void);
	SetNode(int idx, int agent, int row, int col, int hp);
	SetNode(int idx, Piece piece, int hp);
	~SetNode(void);
	const SetNode &operator=(const SetNode &sn );
	void init(int idx, Piece piece, int hp);
	void clear();	// a setnode is cleared when th piece is killed
	void drop();	// a setnode is dropped when it is unioned to another
};

