
#include "Piece.h"


Piece::Piece():agent(GO_NULL),row(0),col(0)
{
}

Piece::Piece(int agent, int row, int col):agent(agent),row(row),col(col)
{
}

Piece::~Piece(void)
{
}

bool Piece::isEmpty()
{
	return (agent==GO_NULL && row==0 && col==0);
}

void Piece::clear()
{
	agent = GO_NULL;
	row = col = 0;
}

bool Piece::legal(){
	return ( agent!=GO_BLACK || agent!=GO_WHITE || row<1 || col<1 || row>BOARD_SIZE || col>BOARD_SIZE );
}

Piece &Piece::operator =(const Piece &piece){
	agent = piece.agent;
	row = piece.row;
	col = piece.col;
	return *this;
}

bool Piece::operator <(const Piece &piece){
	return (row*BOARD_SIZE+col) < (piece.row*BOARD_SIZE+piece.col);
}

