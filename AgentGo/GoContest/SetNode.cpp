
#include "SetNode.h"
#include "../TinyOP.h"

struct PieceArray
{
	Piece ar[BOARD_SIZE*BOARD_SIZE];
};
//TinyOP<PieceArray> PiecePool(20000);

SetNode::SetNode(void):pnode(UNKNOWN_IDX),hp(0),deepth(0),size(0)
{
}

SetNode::~SetNode(void)
{

}

const SetNode &SetNode::operator=(const SetNode &sn ){
	pnode = sn.pnode;
	hp = sn.hp;
	deepth = sn.deepth;
	size = sn.size;
	memcpy(pieces, sn.pieces, SET_STATIC_SIZE*sizeof(Piece) );
	return *this;
}

void SetNode::init(int idx, Piece piece, int hp)
{
	pnode = idx;
	deepth = 1;
	hp = hp;
	#ifdef GO_DEBUG
		if(size>0){
			dprintf("error in SetNode::init, the node is not cleared");
			AG_PANIC(0);
		}
	#endif
	size = 1;
	pieces[0] = piece;
}

void SetNode::clear(){
	pnode = UNKNOWN_IDX;
	hp = 0;
	deepth = 0;
	size = 0;
	memset(pieces, 0, SET_STATIC_SIZE*sizeof(Piece) );
}

void SetNode::drop(){
	hp = 0;
	deepth = 0;
	size = 0;
	memset(pieces, 0, SET_STATIC_SIZE*sizeof(Piece) );
}

void SetNode::push_back(Piece piece){
	if( size==SET_STATIC_SIZE )	initDyn();
	pieces[size] = piece;
	size++;
	return;
}

void SetNode::merge_pieces(const SetNode &sn){
	
	memcpy(&(pieces[size]), &(sn.pieces[0]), sn.size*sizeof(Piece) );
	size += sn.size;
}

void SetNode::initDyn(){


	// it doesn't clear the pieces_stat[]
}