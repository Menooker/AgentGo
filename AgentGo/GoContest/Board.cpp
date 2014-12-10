#include "Board.h"
#include "..\DbgPipe.h"

Board::Board(void)
{
	//rcsv_flags = NULL;
	clear();
}

Board::Board(const Board *board){
	clone(*board);
}

Board::Board(const Board &board){
	clone(board);
}

Board::~Board(void)
{
	release();
}

void Board::clear(){
	int i,j;
	for( i=0; i<BOARD_SIZE; i++ ){
		for( j=0; j<BOARD_SIZE; j++ ){
			data[i][j] = GO_NULL;
			// hp_map[i][j] = 0;
		}
	}
	for ( i=0; i<BOARD_SIZE*BOARD_SIZE; i++){
		set_nodes[i].clear();
	}
	#ifdef	GO_HISTORY	
		for( i=0; i<history_head; i++){
			history[i].clear();
		}
		history_head = 0;
	#endif
	num_black = num_white = 0;
	for ( i=0; i<BOARD_SIZE; i++){
		space_split_sm[i] = BOARD_SIZE;
		reserve_split_sm[0][i] = reserve_split_sm[1][i] = 0;
	}
	for ( i=0; i<SPLIT_NUM_LARGE; i++){
		space_split_lg[i] = SPLIT_SIZE_LARGE * BOARD_SIZE;
		reserve_split_lg[0][i] = reserve_split_lg[1][i] = 0;
	}
}

bool Board::put(int agent,int row,int col){
	int i = row, j = col;

	#ifdef GO_DEBUG
		if ( data[i][j] != GO_NULL ){
			dprintf("error in put piece in %d %d, the position is already occupied\n", row, col);
			AG_PANIC(0);
		}
		if ( !Piece(agent,row,col).legal() ){
			dprintf("error in put piece in %d %d, the piece is illegal\n", row, col);
			AG_PANIC(0);
		}
	#endif

	data[i][j] = agent;
	if( agent == GO_BLACK ) num_black++;
	else num_white++;

	// append to the history list
	#ifdef GO_HISTORY
		history[history_head] = Piece(agent,row,col);
		history_head++;
	#endif

	// init the setnode
	int idx = i * BOARD_SIZE + j;
	set_nodes[idx].init(idx,Piece(agent,row,col),0);

	// refresh the splits
	space_split_sm[i] -= 1;
	int i_lg = i / SPLIT_SIZE_LARGE;
	space_split_lg[i_lg] -= 1;

	// calculating the hp of the new piece before calculating the sorroundings 
	// in case that if some pieces killed, hp will be added more than twice. 
	int hp = 0;	
	if ( i-1>=0 && data[i-1][j]==GO_NULL)	hp++;
	if ( j-1>=0 && data[i][j-1]==GO_NULL)	hp++;
	if ( i+1<BOARD_SIZE && data[i+1][j]==GO_NULL)	hp++;
	if ( j+1<BOARD_SIZE && data[i][j+1]==GO_NULL)	hp++;

	if ( i-1>=0 && data[i-1][j]!=GO_NULL){
		SetNode* sn = getSet(i-1,j);
		sn->hp -= 1;
		if( data[i-1][j]!=agent ){
			if( sn->hp == 0 ) killSetNode(sn);
		}else{
			unionSetNode(sn,getSet(i,j));
		}
	}
	if ( j-1>=0 && data[i][j-1]!=GO_NULL){
		SetNode* sn = getSet(i,j-1);
		sn->hp -= 1;
		if( data[i][j-1]!=agent ){
			if( sn->hp == 0 ) killSetNode(sn);
		}else{
			unionSetNode(sn,getSet(i,j));
		}
	}
	if ( i+1<BOARD_SIZE && data[i+1][j]!=GO_NULL){
		SetNode* sn = getSet(i+1,j);
		sn->hp -= 1;
		if( data[i+1][j]!=agent ){
			if( sn->hp == 0 ) killSetNode(sn);
		}else{
			unionSetNode(sn,getSet(i,j));
		}
	}
	if ( j+1<BOARD_SIZE && data[i][j+1]!=GO_NULL){
		SetNode* sn = getSet(i,j+1);
		sn->hp -= 1;
		if( data[i][j+1]!=agent ){
			if( sn->hp == 0 ) killSetNode(sn);
		}else{
			unionSetNode(sn,getSet(i,j));
		}
	}
	SetNode* snself = getSet(i,j);
	snself->hp += hp;
	if( snself->hp == 0 ) killSetNode(snself);

	/* old one with recursive
	updateHp(row,col);
	if ( i-1>=0 && data[i-1][j]!=agent && data[i-1][j]!=GO_NULL)	updateHp( i-1, j );
	if ( j-1>=0 && data[i][j-1]!=agent && data[i][j-1]!=GO_NULL)	updateHp( i, j-1 );
	if ( i+1<=BOARD_SIZE-1 && data[i+1][j]!=agent && data[i+1][j]!=GO_NULL)	updateHp( i+1, j );
	if ( j+1<=BOARD_SIZE-1 && data[i][j+1]!=agent && data[i][j+1]!=GO_NULL)	updateHp( i, j+1 );
	*/
	return true;
}

bool Board::put(const Piece &piece){
	return put(piece.agent, piece.row, piece.col);
}

Piece Board::getPiece(int row,int col){
	#ifdef GO_DEBUG
		if( row<0 || col<0 || row>BOARD_SIZE-1 || col>BOARD_SIZE-1 ){
			dprintf("error in get piece in %d %d, the position is illegal\n", row, col);
			AG_PANIC(0);
			return Piece(GO_NULL,0,0);
		}
	#endif
	return Piece(data[row][col],row,col);
}

void Board::print(){
	int i,j;
	for( i=0; i<BOARD_SIZE-1; i++){
		for( j=0; j<BOARD_SIZE-1; j++){
			dprintf("%d ",data[i][j]);
		}
		dprintf("\n");
	}
	dprintf("\n");
}

void Board::clone(const Board &board){
	clear();
	int i,j;
	for( i=0; i<BOARD_SIZE; i++ ){
		for( j=0; j<BOARD_SIZE; j++ ){
			data[i][j] = board.data[i][j];
			// hp_map[i][j] = board.hp_map[i][j];
		}
	}
	for( i=0; i<BOARD_SIZE*BOARD_SIZE; i++ ){
		set_nodes[i] = board.set_nodes[i];
	}
	#ifdef GO_HISTORY
		for( i=0; i<MAX_HISTORY_LENGTH; i++){
			history[i] = board.history[i];
		}
		history_head = board.history_head;
	#endif
	for( i=0; i<BOARD_SIZE; i++ ){
		space_split_sm[i] = board.space_split_sm[i] ;
		reserve_split_sm[0][i] = board.reserve_split_sm[0][i];
		reserve_split_sm[1][i] = board.reserve_split_sm[1][i];
	}
	for( i=0; i<SPLIT_NUM_LARGE; i++ ){
		space_split_lg[i] = board.space_split_lg[i] ;
		reserve_split_lg[0][i] = board.reserve_split_lg[0][i];
		reserve_split_lg[1][i] = board.reserve_split_lg[1][i];
	}
}

void Board::release(){
	for( int i=0; i<BOARD_SIZE*BOARD_SIZE; i++){
		set_nodes[i].clear();
	}
	/* old ones
	if ( rcsv_flags != NULL ){
		delete []rcsv_flags;
		for( int i=0; i<BOARD_SIZE; i++ ) delete []rcsv_flags[i];
	}
	*/
	return;
}

SetNode* Board::getSet(int row, int col){
	int idx = row * BOARD_SIZE + col;
	SetNode* ptr = &set_nodes[idx];
	if( ptr->pnode == idx )	return ptr;
	
	stack<SetNode*> st;
	st.push(ptr);
	SetNode* result;
	int result_idx;
	bool return_flag = false;
	// use the stack to update the parent of all nodes passed when turning back
	while( !st.empty() ){
		ptr = st.top();
		if( return_flag == true){
			ptr->pnode = result_idx;
			st.pop();
			continue;
		}
		if( ptr->pnode == idx ){
			return_flag = true;
			result_idx = idx;
			result = ptr;
			st.pop();
			continue;
		}
		idx = ptr->pnode;
		#ifdef GO_DEBUG
			if ( idx == UNKNOWN_IDX ){
				dprintf("error when getting parent set node: try to get a unknown index", idx);
				AG_PANIC(0);
				break;
			}
		#endif
		st.push(&set_nodes[idx]);
	}
	return result;
}

void Board::unionSetNode(SetNode* s1, SetNode* s2){
	if ( s1 == s2 ) return;
	int d1 = s1->deepth, d2 = s2->deepth;
	if( d1 > d2){
		s2->pnode = s1->pnode;
		s1->hp += s2->hp;
		(s1->pieces) -> merge( *(s2->pieces) );
		s2->drop();
	}else{
		s1->pnode = s2->pnode;
		s2->hp += s1->hp;
		(s2->pieces) -> merge( *(s1->pieces) );
		s1->drop();
		if( d1 == d2 ){
			s2->deepth++;
		}
	}
}

void Board::killSetNode(SetNode* sn){
	list<Piece>pieces = *(sn->pieces);
	list<Piece>::iterator itr=(sn->pieces)->begin();
	list<Piece>::iterator it;
	for( it=pieces.begin(); it!=pieces.end(); it++ ){
		Piece p = *it;
		int i = p.row, j = p.col, agent = p.agent;

		data[i][j] = GO_NULL;
		set_nodes[i*BOARD_SIZE+j].clear();	// this clear has also cleared the *sn itself;
		if( agent == GO_BLACK) num_black--;
		else num_white--;
		space_split_sm[i] += 1;
		int i_lg = i / SPLIT_SIZE_LARGE;
		space_split_lg[i_lg] += 1;

		if ( i-1>=0 && data[i-1][j]!=GO_NULL && data[i-1][j]!=agent)	( getSet(i-1,j)->hp ) ++;
		if ( j-1>=0 && data[i][j-1]!=GO_NULL && data[i][j-1]!=agent)	( getSet(i,j-1)->hp ) ++;
		if ( i+1<BOARD_SIZE && data[i+1][j]!=GO_NULL && data[i+1][j]!=agent)	( getSet(i+1,j)->hp ) ++;
		if ( j+1<BOARD_SIZE && data[i][j+1]!=GO_NULL && data[i][j+1]!=agent)	( getSet(i,j+1)->hp ) ++;
	}
}

Piece Board::getRandomPiece(int agent){
	int range = BOARD_SIZE*BOARD_SIZE - num_black - num_white;
	#ifdef GO_DEBUG
		if (range<=0)	{
			dprintf("error in random piece: try to get random piece when there is no space");
			AG_PANIC(0);
		}
	#endif
	int idx = rand() % range;
	int sp_idx_sm=0, sp_idx_lg=0;
	for( int i=0; i<SPLIT_NUM_LARGE; i++){
		if( idx - space_split_lg[i] < 0 )	break;
		idx -= space_split_lg[i];
		sp_idx_lg ++;
	}
	int sp_start_sm = sp_idx_lg * SPLIT_SIZE_LARGE;
	int sp_end_sm = sp_start_sm + SPLIT_SIZE_LARGE;
	if (sp_end_sm > BOARD_SIZE ) sp_end_sm = BOARD_SIZE;
	sp_idx_sm = sp_start_sm;
	for( int i=sp_start_sm; i<sp_end_sm; i++){
		if( idx - space_split_sm[i] < 0 )	break;
		idx -= space_split_sm[i];
		sp_idx_sm ++;
	}
	#ifdef GO_DEBUG
		if (sp_idx_sm>=BOARD_SIZE){
			dprintf("error in random piece: split index out of range");
			AG_PANIC(0);
		}
	#endif
	int col;
	for( int j=0; j<BOARD_SIZE; j++){
		if( data[sp_idx_sm][j] == GO_NULL ){
			if( idx==0 ){
				col = j;
				break;
			}
			else idx --;
		}
	}
	return Piece(agent,sp_idx_sm,col);
}

bool Board::checkSuicide(int agent, int row, int col){
	int i = row, j = col;
	int true_hp = 0;
	int minus_hp = 0;
	SetNode* s[4] = {0,0,0,0};
	if( i-1>=0 ){
		if( data[i-1][j]!=GO_NULL && data[i-1][j]==agent ){
			s[0] = getSet(i-1,j);
			minus_hp++;
		}
		else if( data[i-1][j]==GO_NULL ) minus_hp--;
	}
	if( j-1>=0 ){
		if( data[i][j-1]!=GO_NULL && data[i][j-1]==agent ){
			s[1] = getSet(i,j-1);
			minus_hp++;
		}
		else if( data[i][j-1]==GO_NULL ) minus_hp--;
	}
	if( i+1<BOARD_SIZE ){
		if( data[i+1][j]!=GO_NULL && data[i+1][j]==agent ){
			s[2] = getSet(i+1,j);
			minus_hp++;
		}
		else if( data[i+1][j]==GO_NULL ) minus_hp--;
	}
	if( j+1<BOARD_SIZE ){
		if( data[i][j+1]!=GO_NULL && data[i][j+1]==agent ){
			s[3] = getSet(i,j+1);
			minus_hp++;
		}
		else if( data[i][j+1]==GO_NULL ) minus_hp--;
	}
	// remove repeated setnodes;
	for( int m=0; m<3; m++){
		if( s[m] == NULL ) continue;
		for( int n=m+1; n<4; n++){
			if( s[m] == s[n] ) s[n] = NULL;
		}
	}
	for( int m=0; m<4; m++){
		if( s[m]!=NULL) true_hp += s[m]->hp;
	}
	if( true_hp - minus_hp <=0 ) return true;
	else{
		return false;
	}
}

/* old ones of recursive version

inline void Board::updateHp(int row, int col){
	int hp = getHpRecursive(row,col);
	setHpRecursive(row,col,hp);
}

inline void Board::updateHp(const Piece &piece){
	int hp = getHpRecursive(piece.row,piece.col);
	setHpRecursive(piece.row,piece.col,hp);
}

int Board::getHpRecursive(int row, int col){
	initRcsvFlags();
	struct pos{
		int i,j;
		pos(int i,int j):i(i),j(j){};
	};
	stack<pos> st;
	st.push( pos(row,col) );
	int i,j;
	int hp = 0;
	while(!st.empty()){
		i = st.top().i;
		j = st.top().j;
		st.pop();
		if ( rcsv_flags[i][j]!=0 ) continue;
		rcsv_flags[i][j] = 1;
		int agent = data[i][j];
		if ( agent == GO_NULL ) continue;
		// to see there is empty space to add hp
		if ( i-1>=0 && rcsv_flags[i-1][j]==0 ){
			if( data[i-1][j] == GO_NULL ){
				rcsv_flags[i-1][j] = 2;
				hp ++;
			}
			else if( data[i-1][j] == agent ){
				st.push( pos(i-1,j) );
			}
		}
		if ( j-1>=0 && rcsv_flags[i][j-1]==0 ){
			if( data[i][j-1] == GO_NULL ){
				rcsv_flags[i][j-1] = 2;
				hp ++;
			}
			else if( data[i][j-1] == agent ){
				st.push( pos(i,j-1) );
			}
		}
		if ( i+1<BOARD_SIZE && rcsv_flags[i+1][j]==0 ){
			if( data[i+1][j] == GO_NULL ){
				rcsv_flags[i+1][j] = 2;
				hp ++;
			}
			else if( data[i+1][j] == agent ){
				st.push( pos(i+1,j) );
			}
		}
		if ( j+1<BOARD_SIZE && rcsv_flags[i][j+1]==0 ){
			if( data[i][j+1] == GO_NULL ){
				rcsv_flags[i][j+1] = 2;
				hp ++;
			}
			else if( data[i][j+1] == agent ){
				st.push( pos(i,j+1) );
			}
		}
	}
	return hp;
}



void Board::setHpRecursive(int row, int col, const int hp){
	initRcsvFlags();
	struct pos{
		int i,j;
		pos(int i,int j):i(i),j(j){};
	};
	stack<pos> st;
	st.push( pos(row,col) );
	int i,j;
	while(!st.empty()){
		i = st.top().i;
		j = st.top().j;
		st.pop();
		if ( rcsv_flags[i][j]!=0 ) continue;
		rcsv_flags[i][j] = 1;
		hp_map[i][j] = hp;
		int agent = data[i][j];
		if ( agent == GO_NULL ) continue;
		if ( hp == 0) data[i][j] = GO_NULL;
		if ( i-1>=0 && data[i-1][j] == agent && rcsv_flags[i-1][j]==0 ){
			st.push( pos(i-1,j) );
		}
		if ( j-1>=0 && data[i][j-1] == agent && rcsv_flags[i][j-1]==0 ){
			st.push( pos(i,j-1) );
		}
		if ( i+1<BOARD_SIZE && data[i+1][j] == agent && rcsv_flags[i+1][j]==0 ){
			st.push( pos(i+1,j) );
		}
		if ( j+1<BOARD_SIZE && data[i][j+1] == agent && rcsv_flags[i][j+1]==0 ){
			st.push( pos(i,j+1) );
		}
	}
}

void Board::initRcsvFlags(){
	if ( rcsv_flags == NULL ){
		rcsv_flags = new int*[BOARD_SIZE];
		for( int i=0; i<BOARD_SIZE; i++ ){
			rcsv_flags[i] = new int[BOARD_SIZE];
		}
	}
	for( int i=0; i<BOARD_SIZE; i++ ){
		memset(rcsv_flags[i], 0, sizeof(int)*BOARD_SIZE);	// set every element as false
	}
	return;
}

*/