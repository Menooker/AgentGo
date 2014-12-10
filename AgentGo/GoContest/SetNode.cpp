
#include "SetNode.h"


SetNode::SetNode(void):pnode(UNKNOWN_IDX),hp(0),deepth(0),pieces(NULL)
{
}

SetNode::SetNode(int idx, int agent, int row, int col, int hp):pnode(idx),hp(hp),deepth(1),pieces(NULL)
{
	pieces = new list<Piece>;
	pieces->push_back( Piece(agent,row,col) );
}

SetNode::SetNode(int idx, Piece piece, int hp):pnode(idx),hp(hp),deepth(1),pieces(NULL)
{
	pieces = new list<Piece>;
	pieces->push_back( piece );
}

SetNode::~SetNode(void)
{
	delete pieces;
}

const SetNode &SetNode::operator=(const SetNode &sn ){
	pnode = sn.pnode;
	hp = sn.hp;
	deepth = sn.deepth;
	if( pieces!=NULL ){
		delete pieces;
		pieces = NULL;
	}
	if( sn.pieces!=NULL ){
		pieces = new list<Piece>(*(sn.pieces));
		//*(pieces) = *(sn.pieces);
	}
	return *this;
}

void SetNode::init(int idx, Piece piece, int hp)
{
	pnode = idx;
	deepth = 1;
	hp = hp;
	if( pieces!=NULL ){
		delete pieces;
		pieces = NULL;
	}
	pieces = new list<Piece>;
	pieces->push_back( piece );
}

void SetNode::clear(){
	pnode = UNKNOWN_IDX;
	hp = 0;
	deepth = 0;
	if( pieces!=NULL ){
		delete pieces;
		pieces = NULL;
	}
}

void SetNode::drop(){
	hp = 0;
	deepth = 0;
	if( pieces!=NULL ){
		delete pieces;
		pieces = NULL;
	}
}