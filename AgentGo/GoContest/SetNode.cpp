
#include "SetNode.h"
#include "../TinyOP.h"

struct PieceArray
{
	Piece ar[BOARD_SIZE*BOARD_SIZE];
};
TinyOP<PieceArray> PiecePool(20000);

SetNode::SetNode(void):pnode(UNKNOWN_IDX),hp(0),deepth(0),size(0),use_dyn(false),pieces(pieces_stat)
{
}

SetNode::~SetNode(void)
{
	if( use_dyn ){
		delete []pieces;
	}
}

const SetNode &SetNode::operator=(const SetNode &sn ){
	pnode = sn.pnode;
	hp = sn.hp;
	deepth = sn.deepth;
	size = sn.size;
	if( use_dyn ) delete []pieces;
	memcpy(pieces_stat, sn.pieces_stat, SET_STATIC_SIZE*sizeof(Piece) );
	if( sn.use_dyn ){
		pieces = new Piece[BOARD_SIZE*BOARD_SIZE];
		memcpy(pieces, sn.pieces, BOARD_SIZE*BOARD_SIZE*sizeof(Piece) );
	}
	use_dyn = sn.use_dyn;
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
	memset(pieces_stat, 0, SET_STATIC_SIZE*sizeof(Piece) );
	if( use_dyn && pieces!=pieces_stat ){
		//delete []pieces;
		//PiecePool.free((PieceArray*)pieces);
		pieces = pieces_stat;
	}
	use_dyn = false;
}

void SetNode::drop(){
	hp = 0;
	deepth = 0;
	size = 0;
	memset(pieces_stat, 0, SET_STATIC_SIZE*sizeof(Piece) );
	if( use_dyn && pieces!=pieces_stat){
		//delete []pieces;
		//PiecePool.free((PieceArray*)pieces);
		pieces = pieces_stat;
	}
	use_dyn = false;
}

void SetNode::push_back(Piece piece){
	if( size==SET_STATIC_SIZE )	initDyn();
	pieces[size] = piece;
	size++;
	return;
}

void SetNode::merge_pieces(const SetNode &sn){
	if( use_dyn == false && size+sn.size>SET_STATIC_SIZE) initDyn();
	memcpy(&(pieces[size]), &(sn.pieces[0]), sn.size*sizeof(Piece) );
	size += sn.size;
}

void SetNode::initDyn(){
	use_dyn = true;
	pieces = pieces_stat2;//(Piece*)PiecePool.alloc();//new Piece[BOARD_SIZE*BOARD_SIZE];
	memcpy(pieces, pieces_stat, SET_STATIC_SIZE*sizeof(Piece) );
	// it doesn't clear the pieces_stat[]
}