#include "Board.h"

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

inline int Board::getIdxS(int row,int col){
	if( row<0 || col<0 || row>=BOARD_SIZE || col>=BOARD_SIZE) return -1;
	else return (row*BOARD_SIZE+col);
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
		set_nodes[i].clear();
	}
	#ifdef	GO_HISTORY	
		for( i=0; i<history_head; i++){
			history[i].clear();
		}
		history_head = 0;
	#endif
	num_black = num_white = 0;
	for ( i=0; i<BOARD_SIZE*BOARD_SIZE; i++){
		space_list[i][0]=i-1;
		space_list[i][1]=i+1;
		distance[i][0] = distance[i][1] = 4;
		reserve[i] = 0;
	}
	space_list[0][0] = 0;
	space_list[BOARD_SIZE*BOARD_SIZE-1][1] = BOARD_SIZE*BOARD_SIZE-1;
	space_head = 0;
	reserve_total = 0;
	true_eyes[0] = true_eyes[1] = 0;
	exist_compete = false;
	compete[0] = GO_NULL;
	compete[1] = compete[2] = -1;
	game_ending = false;
	#ifdef GO_BOARD_TIME
		time_put = time_kill = time_random1 = time_random2 = time_getset = 0;
	#endif
	last_move[0].clear();
	last_move[1].clear();
	use_dist = true;
}

bool Board::put(int agent,int row,int col){
	#ifdef GO_BOARD_TIME
		clock_t cl=clock();
	#endif
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
	exist_compete = false;

	// append to the history list
	#ifdef GO_HISTORY
		history[history_head] = Piece(agent,row,col);
		history_head++;
	#endif

	
	
	// init the setnode
	
	int idx = i * BOARD_SIZE + j;
	set_nodes[idx].init(idx,Piece(agent,row,col),0);

	// refresh the space_list, delete this node
	
	int space_prev = space_list[idx][0];
	int space_next = space_list[idx][1];
	if( num_white+num_black == BOARD_SIZE*BOARD_SIZE ) space_head = SPACE_HEAD_NULL;
	else if( space_prev == idx ){
		space_list[space_next][0] = space_next;
		space_head = space_next;
	}
	else if( space_next == idx ){
		space_list[space_prev][1] = space_prev;
	}
	else{
		space_list[space_prev][1] = space_next;
		space_list[space_next][0] = space_prev;
	}

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
	else{
		if(use_dist) updateDistance(agent,i,j);
	}
	last_move[agent-1] = Piece(agent,row,col);

	#ifdef GO_BOARD_TIME
		time_put += clock()-cl;
	#endif
	return true;
}

bool Board::put(const Piece &piece){
	return put(piece.agent, piece.row, piece.col);
}

inline Piece Board::getPiece(int row,int col){
	#ifdef GO_DEBUG
		if( row<0 || col<0 || row>BOARD_SIZE-1 || col>BOARD_SIZE-1 ){
			dprintf("error in get piece in %d %d, the position is illegal\n", row, col);
			AG_PANIC(0);
		}
	#endif
	return Piece(data[row][col],row,col);
}

inline bool Board::checkPiece(int agent, int row, int col){
	if( row<0 || col<0 || row>BOARD_SIZE-1 || col>BOARD_SIZE-1 ){
		return false;
	}
	return ( data[row][col] == agent );
}

void Board::pass(int agent){
	exist_compete = false;
	last_move[agent-1].clear();
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
	for ( i=0; i<BOARD_SIZE*BOARD_SIZE; i++){
		space_list[i][0] = board.space_list[i][0];
		space_list[i][1] = board.space_list[i][1];
		distance[i][0] = board.distance[i][0];
		distance[i][1] = board.distance[i][1];
		reserve[i] = board.reserve[i];
	}
	space_head = board.space_head;
	num_black = board.num_black;
	num_white = board.num_white;
	reserve_total = board.reserve_total;
	true_eyes[0] = board.true_eyes[0];
	true_eyes[1] = board.true_eyes[1];
	exist_compete = board.exist_compete;
	compete[0] = board.compete[0];
	compete[1] = board.compete[1];
	compete[2] = board.compete[2];
	game_ending = board.game_ending;
	last_move[0] = board.last_move[0];
	last_move[1] = board.last_move[1];
	use_dist = board.use_dist;
}

void Board::release(){
	for( int i=0; i<BOARD_SIZE*BOARD_SIZE; i++){
		set_nodes[i].clear();
	}
	return;
}

inline SetNode* Board::getSet(int row, int col){
	
	int idx = row * BOARD_SIZE + col;
	return &set_nodes[set_nodes[idx].pnode];

	/*
	return ptr;
	#ifdef GO_BOARD_TIME
		clock_t cl=clock();
	#endif
	#ifdef GO_BOARD_TIME
		time_getset += clock()-cl;
	#endif
	*/
}

void Board::unionSetNode(SetNode* s1, SetNode* s2){
	if ( s1 == s2 ) return;
	s2->pnode = s1->pnode;
	s1->hp += s2->hp;
	s1->merge_pieces(*s2);
	for( int k=0; k<(s2->size); k++ ){
		Piece p = s2->pieces[k];
		int i = p.row, j = p.col, agent = p.agent;
		int idx = i*BOARD_SIZE+j;
		set_nodes[idx].pnode = s1->pnode;
	}
	s2->drop();
}

void Board::killSetNode(SetNode* sn){
	#ifdef GO_BOARD_TIME
		clock_t cl=clock();
	#endif
	bool kill_single = false;
	if( sn->size == 1 ) kill_single = true;
	for( int k=0; k<(sn->size); k++ ){
		Piece p = sn->pieces[k];
		int i = p.row, j = p.col, agent = p.agent;

		data[i][j] = GO_NULL;
		if( agent == GO_BLACK) num_black--;
		else num_white--;

		int idx = i*BOARD_SIZE+j;
		if( &set_nodes[idx] != sn ){
			set_nodes[idx].clear();	// not clear the *sn itself;
		}
		if( space_head != SPACE_HEAD_NULL ){
			space_list[idx][0] = idx; 
			space_list[idx][1] = space_head; 
			space_list[space_head][0] = idx;
			space_head = idx;
		}
		else{
			space_list[idx][0] = idx; 
			space_list[idx][1] = idx; 
			space_head = idx;
		}

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
	sn->clear();
	#ifdef GO_BOARD_TIME
		time_kill += clock()-cl;
	#endif
}

Piece Board::getRandomPiece(int agent){
	#ifdef GO_BOARD_TIME
		clock_t cl=clock();
	#endif
	int row, col;
	bool flag = true;

	if( game_ending ){
		if( num_black+num_white<=120) game_ending = false;
		else return getRandomPieceComplex(agent);
	}
	if( num_black + num_white > RANDOM_IMPROVE_POINT ){
		//check the place around the last move with some probability
		int enemy = 3 - agent;
		int enmrow = last_move[enemy-1].row, enmcol = last_move[enemy-1].col;
		if( !last_move[enemy-1].isEmpty() && data[enmrow][enmcol]==enemy ){
			int range = 1;
			// the probability of a sorrounding place to be check: 1-(1-1/(2*range+1)^2)^k
			for( int k=0; k<9; k++){
				int i = enmrow - range + ( rand() % (2*range+1));
				int j = enmcol - range + ( rand() % (2*range+1));
				if( (i>=0 && i<BOARD_SIZE && j>=0 && j<BOARD_SIZE)
					&& data[i][j] == GO_NULL
					&& ( false
					  || checkKill(agent,i,j)
					  || k<6 && checkSurvive(agent,i,j)
					  || k<4 && checkChase(agent,i,j)
					)
					&& !checkDying(agent,i,j)
					&& !checkTrueEye(agent,i,j) 
					&& !checkSuicide(agent,i,j)
					&& !checkCompete(agent,i,j)
				){
					row = i;
					col = j;
					flag = false;
					break;
				}
			}
		}
	}

	int count = 0;
	while(flag){
		flag = false;
		count ++;
		row = rand() % BOARD_SIZE;
		col = rand() % BOARD_SIZE;
		if( data[row][col]!=GO_NULL
			//|| count<20 && !checkKeyPlace(agent,row,col)
			//|| count<20 && !checkGoodPlace(agent,row,col)
			//|| !checkNeighbour(agent,row,col)
			//|| !checkDistFar(agent,row,col)
			|| checkNoSense(agent,row,col)
			|| checkDying(agent,row,col)
			|| checkTrueEye(agent,row,col) 
			|| checkSuicide(agent,row,col)
			|| checkCompete(agent,row,col)
		)
		{
			flag = true;
		}
		if( count == ENDING_THRESHOLD ){
			game_ending = true;
			return getRandomPieceComplex(agent);
		}
	}
	#ifdef GO_BOARD_TIME
		time_random1 += clock()-cl;
	#endif
	return Piece(agent,row,col);
}

Piece Board::getRandomPieceComplex(int agent)
{
	#ifdef GO_BOARD_TIME
		clock_t cl=clock();
	#endif
	int row, col;
	int spaces = BOARD_SIZE*BOARD_SIZE - num_black - num_white;
	if ( spaces == 0) return Piece();
	int idx = space_head;
	while( true ){
		row = idx / BOARD_SIZE;
		col = idx % BOARD_SIZE;
		if( checkTrueEye(agent,row,col)
			|| checkSuicide(agent,row,col)
			|| checkCompete(agent,row,col)
		){
			addReserve(agent,row,col);
		}
		if( space_list[idx][1] == idx ) break;
		else idx = space_list[idx][1];
	}

	int range = BOARD_SIZE*BOARD_SIZE - num_black - num_white - reserve_total;
	if ( range == 0 ){
		resetReserve();
		return Piece();
	}
	
	#ifdef GO_DEBUG
		if (range<0 || range>BOARD_SIZE*BOARD_SIZE)	{
			dprintf("error in random piece: range out of index");
			AG_PANIC(0);
		}
	#endif
	int rnd = rand() % range;
	idx = space_head;
	while( true ){
		#ifdef GO_DEBUG
			if ( space_list[idx][1]==idx && rnd>0 )	{
				dprintf("error in random piece: space-enumerating ends before rnd=0");
				AG_PANIC(0);
			}
			if ( space_list[idx][1]==idx && reserve[idx] )	{
				dprintf("error in random piece: space-enumerating ends when the last space is reserved");
				AG_PANIC(0);
			}
		#endif

		if( reserve[idx] ){
			idx = space_list[idx][1];
			continue;
		}
		if( rnd==0 ){
			row = idx / BOARD_SIZE;
			col = idx % BOARD_SIZE;
			break;
		}
		idx = space_list[idx][1];
		rnd--;
	}
	resetReserve();
	#ifdef GO_BOARD_TIME
		time_random2 += clock()-cl;
	#endif
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

bool Board::checkKill(int agent, int row, int col){
	int i = row, j = col;
	bool enemy[4] = {0,0,0,0};
	int minus_hp[4] = {1,1,1,1};
	SetNode* s[4] = {0,0,0,0};
	if(i-1>=0 && data[i-1][j]!=GO_NULL ){
		if( data[i-1][j]!=agent ) enemy[0] = true;
		s[0] = getSet(i-1,j);
	}
	if(j-1>=0 && data[i][j-1]!=GO_NULL ){
		if( data[i][j-1]!=agent ) enemy[1] = true;
		s[1] = getSet(i,j-1);
	}
	if(i+1<BOARD_SIZE && data[i+1][j]!=GO_NULL ){
		if( data[i+1][j]!=agent ) enemy[2] = true;
		s[2] = getSet(i+1,j);
	}
	if(j+1<BOARD_SIZE && data[i][j+1]!=GO_NULL ){
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
	// if any adjacent enemy node could be killed return true
	for( int m=0; m<4; m++){
		if( s[m]!=NULL && ( enemy[m]==true && (s[m]->hp)-minus_hp[m]==0 ) ) return true;
	}
	return false;
}

bool Board::checkSurvive(int agent, int row, int col){
	int rival = 3 - agent;
	
	// Survive is a place where the enemy could kill you but you can save yourself by putting here.
	if( !checkKill(rival,row,col) ) return false;
	int i = row, j = col;
	int hp_new = 0;
	bool enemy[4] = {0,0,0,0};
	int minus_hp[4] = {1,1,1,1};
	SetNode* s[4] = {0,0,0,0};
	if(i-1>=0){
		if( data[i-1][j]==GO_NULL )	hp_new++;
		else{
			if( data[i-1][j]!=agent ) enemy[0] = true;
			s[0] = getSet(i-1,j);
		}
	}
	if(j-1>=0){
		if( data[i][j-1]==GO_NULL )	hp_new++;
		else{
			if( data[i][j-1]!=agent ) enemy[1] = true;
			s[1] = getSet(i,j-1);
		}
	}
	if(i+1<BOARD_SIZE){
		if( data[i+1][j]==GO_NULL )	hp_new++;
		else{
			if( data[i+1][j]!=agent ) enemy[2] = true;
			s[2] = getSet(i+1,j);
		}
	}
	if(j+1<BOARD_SIZE){
		if( data[i][j+1]==GO_NULL )	hp_new++;
		else{
			if( data[i][j+1]!=agent ) enemy[3] = true;
			s[3] = getSet(i,j+1);
		}
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
	int hp_min = 9999;
	// if the place will enlarge the total hp return true, if any enemy could be killed return true
	for( int m=0; m<4; m++){
		if( s[m]!=NULL && ( enemy[m]==true && (s[m]->hp)-minus_hp[m]==0 ) ) return true;
		if( s[m]!=NULL && enemy[m]==false ){
			hp_new += s[m]->hp - minus_hp[m];
			if( s[m]->hp < hp_min ){
				hp_min = s[m]->hp;
			}
		}
	}
	return (hp_new>hp_min);
}

bool Board::checkDying(int agent, int row, int col){
	int i = row, j = col;
	int hp_new = 0;
	bool enemy[4] = {0,0,0,0};
	int minus_hp[4] = {1,1,1,1};
	SetNode* s[4] = {0,0,0,0};
	if(i-1>=0){
		if( data[i-1][j]==GO_NULL )	hp_new++;
		else{
			if( data[i-1][j]!=agent ) enemy[0] = true;
			s[0] = getSet(i-1,j);
		}
	}
	if(j-1>=0){
		if( data[i][j-1]==GO_NULL )	hp_new++;
		else{
			if( data[i][j-1]!=agent ) enemy[1] = true;
			s[1] = getSet(i,j-1);
		}
	}
	if(i+1<BOARD_SIZE){
		if( data[i+1][j]==GO_NULL )	hp_new++;
		else{
			if( data[i+1][j]!=agent ) enemy[2] = true;
			s[2] = getSet(i+1,j);
		}
	}
	if(j+1<BOARD_SIZE){
		if( data[i][j+1]==GO_NULL )	hp_new++;
		else{
			if( data[i][j+1]!=agent ) enemy[3] = true;
			s[3] = getSet(i,j+1);
		}
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
	// if any enemy could be killed return false
	for( int m=0; m<4; m++){
		if( s[m]!=NULL && ( enemy[m]==true && (s[m]->hp)-minus_hp[m]==0 ) ) return false;
		if( s[m]!=NULL && enemy[m]==false ) hp_new += s[m]->hp - minus_hp[m];
	}
	return ( hp_new == 1 );
}

inline bool Board::checkChase(int agent,int row,int col){
	int i = row, j = col;
	int hp_new = 0;
	bool enemy[4] = {0,0,0,0};
	int minus_hp[4] = {1,1,1,1};
	SetNode* s[4] = {0,0,0,0};
	if(i-1>=0){
		if( data[i-1][j]==GO_NULL )	hp_new++;
		else{
			if( data[i-1][j]!=agent ) enemy[0] = true;
			s[0] = getSet(i-1,j);
		}
	}
	if(j-1>=0){
		if( data[i][j-1]==GO_NULL )	hp_new++;
		else{
			if( data[i][j-1]!=agent ) enemy[1] = true;
			s[1] = getSet(i,j-1);
		}
	}
	if(i+1<BOARD_SIZE){
		if( data[i+1][j]==GO_NULL )	hp_new++;
		else{
			if( data[i+1][j]!=agent ) enemy[2] = true;
			s[2] = getSet(i+1,j);
		}
	}
	if(j+1<BOARD_SIZE){
		if( data[i][j+1]==GO_NULL )	hp_new++;
		else{
			if( data[i][j+1]!=agent ) enemy[3] = true;
			s[3] = getSet(i,j+1);
		}
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
	// if any enemy could be killed return true but make sure itself's safety
	bool chase = false;
	for( int m=0; m<4; m++){
		if( s[m]!=NULL && ( enemy[m]==true && (s[m]->hp)-minus_hp[m]==1 ) ) chase = true;
		if( s[m]!=NULL && enemy[m]==false ) hp_new += s[m]->hp - minus_hp[m];
	}
	return ( chase && hp_new>1 );
}

bool Board::checkTrueEye(int agent, int row, int col){
	int i = row, j = col;
	if( i-1>=0 && data[i-1][j]!=agent )	return false;
	if( j-1>=0 && data[i][j-1]!=agent )	return false;
	if( i+1<BOARD_SIZE && data[i+1][j]!=agent )	return false;
	if( j+1<BOARD_SIZE && data[i][j+1]!=agent )	return false;
	int cnt = 0;  // cnt the number of the shoulders occupied by the rival
	if( i-1>=0 && j-1>=0 && data[i-1][j-1]!=GO_NULL && data[i-1][j-1]!=agent )	cnt++;
	if( i-1>=0 && j+1<BOARD_SIZE && data[i-1][j+1]!=GO_NULL && data[i-1][j+1]!=agent )	cnt++;
	if( i+1<BOARD_SIZE && j-1>=0 && data[i+1][j-1]!=GO_NULL && data[i+1][j-1]!=agent )	cnt++;
	if( i+1<BOARD_SIZE && j+1<BOARD_SIZE && data[i+1][j+1]!=GO_NULL && data[i+1][j+1]!=agent )	cnt++;
	if( i==0 || i==BOARD_SIZE-1 || j==0 || j==BOARD_SIZE-1 ){
		if( cnt>=1 ) return false;
	}
	else if( cnt>=2 ) return false;
	return true;
}

inline bool Board::checkCompete(int agent, int row, int col){
	return (exist_compete && row==compete[1] && col==compete[2] && agent==compete[0]);
}

inline void Board::addReserve(int agent, int row, int col){
	int idx = row*BOARD_SIZE+col;
	reserve[idx] = true;
	reserve_total ++;
}


inline void Board::resetReserve(){
	reserve_total = 0;
	memset(reserve,0,BOARD_SIZE*BOARD_SIZE*sizeof(bool));
	return;
}

inline bool Board::checkKeyPlace(int agent,int row,int col){
	int enemy = 3 - agent;

	bool at_chase = checkChase(agent, row, col);
	bool at_kill = checkKill(agent, row, col);
	bool at_survive = checkSurvive(agent, row, col);
	if ( at_chase || at_kill || at_survive ) return true;
	return false;
}

inline bool Board::checkGoodPlace(int agent,int row,int col){
	int enemy = 3 - agent;
	
	int idx = row*BOARD_SIZE+col;
	bool at_front =( distance[idx][agent-1]-distance[idx][enemy-1] <= 1 && distance[idx][enemy-1] <= 3);

	if ( at_front ) return true;
	return false;
}

inline bool Board::checkNeighbour(int agent,int row,int col){
	if( false
	// chang or peng
	|| checkPiece(agent,row-1,col)
	|| checkPiece(agent,row,col-1)
	|| checkPiece(agent,row+1,col)
	|| checkPiece(agent,row,col+1)
	// jian
	|| checkPiece(agent,row-1,col-1)
	|| checkPiece(agent,row-1,col+1)
	|| checkPiece(agent,row+1,col-1)
	|| checkPiece(agent,row+1,col+1)
	// xiaofei or gua
	|| checkPiece(agent,row-2,col-1)
	|| checkPiece(agent,row-1,col-2)
	|| checkPiece(agent,row+1,col-2)
	|| checkPiece(agent,row+2,col-1)
	|| checkPiece(agent,row+2,col+1)
	|| checkPiece(agent,row+1,col+2)
	|| checkPiece(agent,row-1,col+2)
	|| checkPiece(agent,row-2,col+1)
	// dafei or dagua
	//|| checkPiece(agent,row-3,col-1)
	//|| checkPiece(agent,row-1,col-3)
	//|| checkPiece(agent,row+1,col-3)
	//|| checkPiece(agent,row+3,col-1)
	//|| checkPiece(agent,row+3,col+1)
	//|| checkPiece(agent,row+1,col+3)
	//|| checkPiece(agent,row-1,col+3)
	//|| checkPiece(agent,row-3,col+1)
	// guan
	|| checkPiece(agent,row-2,col)
	|| checkPiece(agent,row,col-2)
	|| checkPiece(agent,row+2,col)
	|| checkPiece(agent,row,col+2)
	){
		return true;
	}
	return false;
}

void Board::updateDistance(int agent,int row,int col){
	setDistance(agent,row-1,col,1);
	setDistance(agent,row,col-1,1);
	setDistance(agent,row+1,col,1);
	setDistance(agent,row,col+1,1);
	// jian tiao fei
	setDistance(agent,row-1,col-1,2);
	setDistance(agent,row+1,col-1,2);
	setDistance(agent,row+1,col+1,2);
	setDistance(agent,row-1,col+1,2);
	setDistance(agent,row-2,col,2);
	setDistance(agent,row,col-2,2);
	setDistance(agent,row+2,col,2);
	setDistance(agent,row,col+2,2);
	setDistance(agent,row-2,col-1,2);
	setDistance(agent,row-1,col-2,2);
	setDistance(agent,row+1,col-2,2);
	setDistance(agent,row+2,col-1,2);
	setDistance(agent,row+2,col+1,2);
	setDistance(agent,row+1,col+2,2);
	setDistance(agent,row-1,col+2,2);
	setDistance(agent,row-2,col+1,2);
	// xiangya datiao dafei
	setDistance(agent,row-2,col-2,3);
	setDistance(agent,row+2,col-2,3);
	setDistance(agent,row+2,col+2,3);
	setDistance(agent,row+2,col+2,3);
	setDistance(agent,row-3,col,3);
	setDistance(agent,row,col-3,3);
	setDistance(agent,row+3,col,3);
	setDistance(agent,row,col+3,3);
	setDistance(agent,row-3,col-1,3);
	setDistance(agent,row-1,col-3,3);
	setDistance(agent,row+1,col-3,3);
	setDistance(agent,row+3,col-1,3);
	setDistance(agent,row+3,col+1,3);
	setDistance(agent,row+1,col+3,3);
	setDistance(agent,row-1,col+3,3);
	setDistance(agent,row-3,col+1,3);
}

void Board::setDistance(int agent,int row,int col,int dist){
	if( row<0 || col<0 || row>=BOARD_SIZE || col>=BOARD_SIZE ) return;
	if( distance[row*BOARD_SIZE+col][agent-1]>dist ) distance[row*BOARD_SIZE+col][agent-1] = dist;
}

bool Board::checkDistFar(int agent,int row,int col){
	int enemy = 3 - agent;
	int threshold_1 = -1;
	int threshold_2 = 2;
	return ( distance[row*BOARD_SIZE+col][agent-1]-distance[row*BOARD_SIZE+col][enemy-1] >= threshold_1
		&& distance[row*BOARD_SIZE+col][agent-1]-distance[row*BOARD_SIZE+col][enemy-1] <= threshold_2
		);
}

bool Board::checkNoSense(int agent, int row, int col){
	int enemy = 3 - agent;
	int nb_self_1 = 0, nb_enemy_1 = 0, nb_self_2 = 0, nb_enemy_2 = 0;
	int range = 1;
	for( int i = row-range; i<=row+range; i++ ){
		for( int j = col-range; j<=col+range; j++ ){
			if ( i==row && j==col ) continue;
			if( checkPiece(agent,i,j) ) nb_self_1++;
			else if ( checkPiece(enemy,i,j) ) nb_enemy_1++;
		}
	}
	bool at_border = (row==0 || row==BOARD_SIZE-1 || col==0 || col==BOARD_SIZE-1);
	bool at_islbd = ( nb_enemy_1+nb_self_1==0 ) && at_border;
	range = 2;
	for( int i = row-range; i<=row+range; i++ ){
		for( int j = col-range; j<=col+range; j++ ){
			if ( i==row && j==col ) continue;
			if( checkPiece(agent,i,j) ) nb_self_2++;
			else if ( checkPiece(enemy,i,j) ) nb_enemy_2++;
		}
	}
	bool near_border = (row<=1 || row>=BOARD_SIZE-2 || col<=1 || col>=BOARD_SIZE-2);
	bool near_islbd = ( nb_enemy_2+nb_self_2==0 ) && near_border;
	
	if( at_islbd || near_islbd ) return true;
}

int Board::countScore(int agent){
	int count = 0;
	for(int i=0; i<BOARD_SIZE; i++){
		for(int j=0; j<BOARD_SIZE; j++){
			if( data[i][j]==agent ){
				count++;
			}
			else if( data[i][j]==GO_NULL ){
				if( (j>0 && data[i][j-1]==agent) || (j==0 && data[i][j+1]==agent) ){
					count++;
				}
			}
		}
	}
	return count;
}