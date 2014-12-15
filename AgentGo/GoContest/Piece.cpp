
#include "Piece.h"


Piece::Piece():agent(GO_NULL),row(PIECE_EMPTY_IDX),col(PIECE_EMPTY_IDX)
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
	return (agent==GO_NULL && row==PIECE_EMPTY_IDX && col==PIECE_EMPTY_IDX);
}

void Piece::clear()
{
	agent = GO_NULL;
	row = col = PIECE_EMPTY_IDX;
}

bool Piece::legal(){
	return ( ( agent==GO_BLACK || agent==GO_WHITE ) && row>=0 && col>=0 && row<BOARD_SIZE && col<BOARD_SIZE );
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

