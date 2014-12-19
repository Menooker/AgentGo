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
			reserve[i][j] = GO_NULL;
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
	reserve_total[0] = reserve_total[1] =0;
	true_eyes[0] = true_eyes[1] = 0;
	to_reset_reserve = true;
	exist_compete = false;
	compete[0] = GO_NULL;
	compete[1] = compete[2] = -1;
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
	if( to_reset_reserve ) resetReserve();
	else if( reserve[i][j] != GO_NULL ) removeReserve(i,j);
	if( agent == GO_BLACK ) num_black++;
	else num_white++;
	exist_compete = false;

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
	if(i_lg<SPLIT_NUM_LARGE) space_split_lg[i_lg] -= 1;

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
	/*
	int white = 0;
	int black = 0;
	for (int m = 0;m<BOARD_SIZE;m++){
		for (int n = 0;n<BOARD_SIZE;n++){
			if(data[m][n]==2) white ++;
			if(data[m][n]==1) black++;
		}
	}
	dprintf("white:%d black:%d num_white:%d num_black:%d\n",white,black,num_white,num_black);
	*/
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

void Board::pass(int agent){
	exist_compete = false;
}

void Board::print(){
	int i,j;
	for( i=0; i<BOARD_SIZE; i++){
		for( j=0; j<BOARD_SIZE; j++){
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
			reserve[i][j] = board.reserve[i][j];
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
	num_black = board.num_black;
	num_white = board.num_white;
	reserve_total[0] = board.reserve_total[0];
	reserve_total[1] = board.reserve_total[1];
	true_eyes[0] = board.true_eyes[0];
	true_eyes[1] = board.true_eyes[1];
	to_reset_reserve = board.to_reset_reserve;
	exist_compete = board.exist_compete;
	compete[0] = board.compete[0];
	compete[1] = board.compete[1];
	compete[2] = board.compete[2];

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
	bool kill_single = false;
	if( pieces.size() == 1 ) kill_single = true;
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
		if(i_lg<SPLIT_NUM_LARGE) space_split_lg[i_lg] += 1;

		if ( i-1>=0 && data[i-1][j]!=GO_NULL && data[i-1][j]!=agent)	( getSet(i-1,j)->hp ) ++;
		if ( j-1>=0 && data[i][j-1]!=GO_NULL && data[i][j-1]!=agent)	( getSet(i,j-1)->hp ) ++;
		if ( i+1<BOARD_SIZE && data[i+1][j]!=GO_NULL && data[i+1][j]!=agent)	( getSet(i+1,j)->hp ) ++;
		if ( j+1<BOARD_SIZE && data[i][j+1]!=GO_NULL && data[i][j+1]!=agent)	( getSet(i,j+1)->hp ) ++;
		
		if( kill_single == true ){
			exist_compete = true;
			compete[0] = agent;
			compete[1] = i;
			compete[2] = j;
		}
	}
}

Piece Board::getRandomPiece(int agent){
	int row, col;
	bool flag = true;
	while(flag){
		flag = false;
		int range = BOARD_SIZE*BOARD_SIZE - num_black - num_white - reserve_total[agent-1];
		if (range==0) return Piece();
		#ifdef GO_DEBUG
			if (range<0 || range>BOARD_SIZE*BOARD_SIZE)	{
				dprintf("error in random piece: range out of index");
				AG_PANIC(0);
			}
		#endif
		int idx = rand() % range;
		int sp_idx_sm=0, sp_idx_lg=0;
		for( int i=0; i<SPLIT_NUM_LARGE; i++){
			if( idx - space_split_lg[i] + reserve_split_lg[agent-1][i] < 0 )	break;
			idx -= space_split_lg[i] - reserve_split_lg[agent-1][i];
			sp_idx_lg ++;
		}
		int sp_start_sm = sp_idx_lg * SPLIT_SIZE_LARGE;
		int sp_end_sm = sp_start_sm + SPLIT_SIZE_LARGE;
		if (sp_end_sm > BOARD_SIZE ) sp_end_sm = BOARD_SIZE;
		sp_idx_sm = sp_start_sm;
		for( int i=sp_start_sm; i<sp_end_sm; i++){
			if( idx - space_split_sm[i] + reserve_split_sm[agent-1][i] < 0 )	break;
			idx -= space_split_sm[i] - reserve_split_sm[agent-1][i];
			sp_idx_sm ++;
		}
		#ifdef GO_DEBUG
			if (sp_idx_sm>=BOARD_SIZE){
				dprintf("error in random piece: split index out of range");
				AG_PANIC(0);
			}
		#endif
		row = sp_idx_sm;
		for( int j=0; j<BOARD_SIZE; j++){
			if( data[row][j] == GO_NULL && reserve[row][j]!=agent && reserve[row][j]!=GO_BOTH ){
				if( idx==0 ){
					col = j;
					break;
				}
				else idx --;
			}
		}
		if( checkSuicide(agent,row,col) ){
			addReserve(agent,row,col);
			flag = true;
		}
		else if( checkTrueEye(agent,row,col) ){
			if(to_reset_reserve) true_eyes[agent-1] += 1;
			addReserve(agent,row,col);
			flag = true;
		}
		else if( exist_compete && row==compete[1] && col==compete[2] && agent==compete[0]){
			addReserve(agent,row,col);
			flag = true;
		}
	}
	return Piece(agent,row,col);
}

bool Board::checkSuicide(int agent, int row, int col){
	int i = row, j = col;
	bool enemy[4] = {0,0,0,0};
	int minus_hp[4] = {1,1,1,1};
	SetNode* s[4] = {0,0,0,0};
	if(i-1>=0){
		if( data[i-1][j]==GO_NULL )	return false;
		if( data[i-1][j]!=agent ) enemy[0] = true;
		s[0] = getSet(i-1,j);

	}
	if(j-1>=0){
		if( data[i][j-1]==GO_NULL )	return false;
		if( data[i][j-1]!=agent ) enemy[1] = true;
		s[1] = getSet(i,j-1);
	}
	if(i+1<BOARD_SIZE){
		if( data[i+1][j]==GO_NULL )	return false;
		if( data[i+1][j]!=agent ) enemy[2] = true;
		s[2] = getSet(i+1,j);
	}
	if(j+1<BOARD_SIZE){
		if( data[i][j+1]==GO_NULL )	return false;
		if( data[i][j+1]!=agent ) enemy[3] = true;
		s[3] = getSet(i,j+1);
	}


	// remove repeated setnodes;
	for( int m=0; m<3; m++){
		if( s[m] == NULL ) continue;
		for( int n=m+1; n<4; n++){
			if( s[m] == s[n] ){
				s[n] = NULL;
				minus_hp[m] += minus_hp[n];
			}
		}
	}
	// if any adjacent enemy node could be killed or adjacent self node could not be killed, not suicide.
	for( int m=0; m<4; m++){
		if( s[m]!=NULL && (
			( enemy[m]==true && (s[m]->hp)-minus_hp[m]==0 ) ||
			( enemy[m]==false && (s[m]->hp)-minus_hp[m]>0 )
			)) return false;
	}
	return true;
}

bool Board::checkTrueEye(int agent, int row, int col){
	int i = row, j = col;
	if( i-1>=0 && data[i-1][j]!=agent )	return false;
	if( j-1>=0 && data[i][j-1]!=agent )	return false;
	if( i+1<BOARD_SIZE && data[i+1][j]!=agent )	return false;
	if( j+1<BOARD_SIZE && data[i][j+1]!=agent )	return false;
	int count = 0;  // count the number of the shoulders occupied by the rival
	if( i-1>=0 && j-1>=0 && data[i-1][j-1]!=GO_NULL && data[i-1][j-1]!=agent )	count++;
	if( i-1>=0 && j+1>=0 && data[i-1][j+1]!=GO_NULL && data[i-1][j+1]!=agent )	count++;
	if( i+1<BOARD_SIZE && j-1>=0 && data[i+1][j-1]!=GO_NULL && data[i+1][j-1]!=agent )	count++;
	if( i+1<BOARD_SIZE && j+1<BOARD_SIZE && data[i+1][j+1]!=GO_NULL && data[i+1][j+1]!=agent )	count++;
	if( i==0 || i==BOARD_SIZE-1 || j==0 || j==BOARD_SIZE-1 ){
		if( count>=1 ) return false;
	}
	else if( count>=2 ) return false;
	return true;
}

void Board::addReserve(int agent, int row, int col){
	int i = row, j = col;
	int i_lg = i / SPLIT_SIZE_LARGE;
	if( reserve[i][j]!=GO_NULL && reserve[i][j]!=agent){
		reserve[i][j] = GO_BOTH;
	}
	else reserve[i][j] = agent;
	reserve_split_sm[agent-1][i] += 1;
	if(i_lg<SPLIT_NUM_LARGE) reserve_split_lg[agent-1][i_lg] += 1;
	reserve_total[agent-1] += 1;
}

void Board::removeReserve(int row, int col){
	#ifdef GO_DEBUG
		if ( data[row][col] == GO_NULL ){
			dprintf("error in removeReserve in %d %d, the position is not reserved\n", row, col);
			AG_PANIC(0);
		}
	#endif
	int i = row, j = col;
	int i_lg = i / SPLIT_SIZE_LARGE;
	int agent = reserve[i][j];
	reserve[i][j] = GO_NULL;
	if( agent!= GO_BOTH ){
		reserve_split_sm[agent-1][i] -= 1;
		if(i_lg<SPLIT_NUM_LARGE) reserve_split_lg[agent-1][i_lg] -= 1;
		reserve_total[agent-1] -= 1;
	}
	else{
		reserve_split_sm[0][i] -= 1;
		reserve_split_sm[1][i] -= 1;
		if(i_lg<SPLIT_NUM_LARGE){
			reserve_split_lg[0][i_lg] -= 1;
			reserve_split_lg[1][i_lg] -= 1;
		}
		reserve_total[0] -= 1;
		reserve_total[1] -= 1;
	}
}

void Board::resetReserve(){
	if( reserve_total[0]>0 || reserve_total[1]>0 ) reserve_total[0] = reserve_total[1] = 0;
	else return;
	for(int i_lg=0; i_lg<SPLIT_NUM_LARGE; i_lg++){
		if( reserve_split_lg[0][i_lg]>0 || reserve_split_lg[1][i_lg]>0 ){
			memset(reserve[i_lg*SPLIT_SIZE_LARGE],0,sizeof(int)*BOARD_SIZE*SPLIT_SIZE_LARGE);
		}
	}
	int remains = BOARD_SIZE - SPLIT_SIZE_LARGE * SPLIT_NUM_LARGE;
	if( remains>0 ){
		memset(reserve[SPLIT_NUM_LARGE*SPLIT_SIZE_LARGE],0,sizeof(int)*BOARD_SIZE*remains);
	}
	memset(reserve_split_sm[0],0,sizeof(int)*BOARD_SIZE);
	memset(reserve_split_sm[1],0,sizeof(int)*BOARD_SIZE);
	memset(reserve_split_lg[0],0,sizeof(int)*SPLIT_NUM_LARGE);
	memset(reserve_split_lg[1],0,sizeof(int)*SPLIT_NUM_LARGE);
	if(to_reset_reserve) true_eyes[0] = true_eyes[1] = 0;
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