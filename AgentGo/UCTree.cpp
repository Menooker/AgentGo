#include "UCTree.h"

UCNode::UCNode(){
	for( int i=0; i<UCT_WIDTH; i++ ){
		children[i] = NULL;
	}
	is_leaf = true;
}

double UCNode::minMax(){
	if( is_leaf ) return score;
	score = children[0]->minMax();
	for( int i=1; i<UCT_WIDTH; i++ ){
		double score_child = children[i]->minMax();
		if( is_max && score_child>score) score = score_child;
		else if ( !is_max && score_child<score) score = score_child;
	}
	return score;
}

void UCNode::expand(){
	for( int i=0; i<UCT_WIDTH; i++ ){
		children[i] = new UCNode;
		children[i]->is_max = !is_max;
		children[i]->player = 1 - player;
		children[i]->parent = this;
	}
	is_leaf = false;
}

void UCNode::clear(){
	for( int i=0; i<UCT_WIDTH; i++ ){
		if( children[i]!=NULL ) children[i]->clear();
	}
}

void UCNode::initBoard(){
	bd.clone(parent->bd);
	if(move.isEmpty()) bd.pass(player+1);
	else bd.put(move);
}

UCTree::UCTree(bool is_max, int player, const Board &bd)
{
	root.is_max = is_max;
	root.player = player;
	root.parent = &root;
	root.bd.clone(bd);
}


UCTree::~UCTree(void)
{
}

Piece UCTree::getBestMove(){
	root.minMax();
	int best_index = 0;
	double best_score = root.children[0]->score;
	for( int i=0; i<UCT_WIDTH ; i++ ){
		if( root.is_max && (root.children[i]->score)>best_score ){
			best_score = root.children[i]->score;
			best_index = i;
		}
		else if( !root.is_max && (root.children[i]->score)<best_score ){
			best_score = root.children[i]->score;
			best_index = i;
		}
	}
	return (root.children[best_index]->move);
}

void UCTree::clear(){
	root.clear();
}