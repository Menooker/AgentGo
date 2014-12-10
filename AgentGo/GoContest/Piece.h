#pragma once

#include "AgentGo.h"

struct Piece
{
public:
	int agent;
	int row;	// row, col from 0 to 12
	int col;
	Piece();
	Piece(int agent, int row, int col); // need to add: Board* Group*
	~Piece(void);
	bool isEmpty();
	void clear();
	bool legal();
	Piece &operator =(const Piece &piece);
	bool operator <(const Piece &piece);
};

