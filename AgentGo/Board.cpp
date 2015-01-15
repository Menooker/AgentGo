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
		//updateDistance(agent,i,j);
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

	//v1
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
				  || checkSurvive(agent,i,j)
				  || k<7 && checkPattern(agent,i,j)
				  || k<6 &&  checkKill(agent,i,j)
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

	//v2
	//check the place around the last move
	//int enemy = 3 - agent;
	//int enmrow = last_move[enemy-1].row, enmcol = last_move[enemy-1].col;
	//if( !last_move[enemy-1].isEmpty() && data[enmrow][enmcol]==enemy ){
	//	Piece cand[(2*RAND_CAND_RANGE+1)*(2*RAND_CAND_RANGE+1)];
	//	int cand_size = 0;
	//	bool skip[(2*RAND_CAND_RANGE+1)*(2*RAND_CAND_RANGE+1)];
	//	memset(skip,0,sizeof(bool)*((2*RAND_CAND_RANGE+1)*(2*RAND_CAND_RANGE+1)));
	//	
	//	int mode = RAND_MODE_SURVIVE;
	//	while( mode<RAND_MODE_ANY ){
	//		// leave some probability that the step will be skipped
	//		if ( rand()%RAND_MODE_JUMP < mode ){
	//			mode++;
	//			continue;
	//		}
	//		// find candidates in this mode
	//		for (int i=enmrow-RAND_CAND_RANGE;i<=enmrow+RAND_CAND_RANGE;i++){
	//			for (int j=enmcol-RAND_CAND_RANGE;j<=enmcol+RAND_CAND_RANGE;j++){
	//				int idx = (i-(enmrow-RAND_CAND_RANGE))*(2*RAND_CAND_RANGE+1) + (j-(enmcol-RAND_CAND_RANGE));
	//				// true eyes will never be chosed, so don't worry
	//				if( skip[idx] == true
	//					|| data[i][j]!=GO_NULL
	//					|| ( i<0 || i>=BOARD_SIZE || j<0 || j>=BOARD_SIZE )
	//					|| checkDying(agent,i,j)
	//					|| checkTrueEye(agent,i,j)
	//					|| checkSuicide(agent,i,j)
	//					|| checkCompete(agent,i,j)
	//				){
	//					skip[idx] = true;
	//					continue;
	//				}
	//				if( false
	//					|| mode==RAND_MODE_SURVIVE && checkSurvive(agent,i,j)
	//					|| mode==RAND_MODE_PATTERN && checkPattern(agent,i,j)
	//					|| mode==RAND_MODE_KILL && checkKill(agent,i,j)
	//					|| mode==RAND_MODE_CHASE && checkChase(agent,i,j)
	//				){
	//					cand[cand_size] = Piece(agent,i,j);
	//					cand_size++;
	//				}
	//			}
	//		}
	//		if (cand_size>0){
	//			return cand[ rand()%cand_size ];
	//		}
	//		// else, in this mode no piece could be chosen
	//		cand_size = 0;
	//		mode++;
	//	}
	//}

	int count = 0;
	while(flag){
		flag = false;
		count ++;
		row = rand() % BOARD_SIZE;
		col = rand() % BOARD_SIZE;
		if( false
			|| data[row][col]!=GO_NULL
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
	
	// Survive is a place where the neighbours are dying but coud be saved here
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
	bool chase = false;
	// if the place will enlarge the total hp return true, if any enemy could be killed return true
	for( int m=0; m<4; m++){
		if( s[m]!=NULL && ( enemy[m]==true && (s[m]->hp)-minus_hp[m]==0 ) ) return true;
		else if( s[m]!=NULL && enemy[m]==false ){
			hp_new += s[m]->hp - minus_hp[m];
			if( s[m]->hp < hp_min ){
				hp_min = s[m]->hp;
			}
		}
	}
	if ( hp_min>1 ) return false;
	return ( hp_new>hp_min );
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

inline bool Board::checkDistFar(int agent,int row,int col){
	int enemy = 3 - agent;
	int threshold = -1;
	return ( distance[row*BOARD_SIZE+col][agent-1]-distance[row*BOARD_SIZE+col][enemy-1] >= threshold );
}

bool Board::checkNoSense(int agent, int row, int col){
	bool at_border = (row==0 || row==BOARD_SIZE-1 || col==0 || col==BOARD_SIZE-1);
	bool near_border = (row<=1 || row>=BOARD_SIZE-2 || col<=1 || col>=BOARD_SIZE-2);
	if( !near_border ) return false;
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
	bool at_islbd = ( nb_enemy_1+nb_self_1==0 ) && at_border;
	range = 2;
	for( int i = row-range; i<=row+range; i++ ){
		for( int j = col-range; j<=col+range; j++ ){
			if ( i==row && j==col ) continue;
			if( checkPiece(agent,i,j) ) nb_self_2++;
			else if ( checkPiece(enemy,i,j) ) nb_enemy_2++;
		}
	}
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

inline int Board::getColorS(int row, int col){
	if( row>=0 && row<BOARD_SIZE && col>=0 && col<BOARD_SIZE ) return data[row][col];
	else return GO_BORDER;
}
int Board::getPatternHash(int row, int col){
	int nb[8],hash[8];
	nb[0] = getColorS(row-1,col);
	nb[1] = getColorS(row-1,col+1);
	nb[2] = getColorS(row,col+1);
	nb[3] = getColorS(row+1,col+1);
	nb[4] = getColorS(row+1,col);
	nb[5] = getColorS(row+1,col-1);
	nb[6] = getColorS(row,col-1);
	nb[7] = getColorS(row-1,col-1);
	hash[0] = (nb[0]<<14)+(nb[1]<<12)+(nb[2]<<10)+(nb[3]<<8)+(nb[4]<<6)+(nb[5]<<4)+(nb[6]<<2)+nb[7];
	hash[1] = (nb[2]<<14)+(nb[3]<<12)+(nb[4]<<10)+(nb[5]<<8)+(nb[6]<<6)+(nb[7]<<4)+(nb[0]<<2)+nb[1];
	hash[2] = (nb[4]<<14)+(nb[5]<<12)+(nb[6]<<10)+(nb[7]<<8)+(nb[0]<<6)+(nb[1]<<4)+(nb[2]<<2)+nb[3];
	hash[3] = (nb[6]<<14)+(nb[7]<<12)+(nb[0]<<10)+(nb[1]<<8)+(nb[2]<<6)+(nb[3]<<4)+(nb[4]<<2)+nb[5];
	hash[4] = (nb[0]<<14)+(nb[7]<<12)+(nb[6]<<10)+(nb[5]<<8)+(nb[4]<<6)+(nb[3]<<4)+(nb[2]<<2)+nb[1];
	hash[5] = (nb[6]<<14)+(nb[5]<<12)+(nb[4]<<10)+(nb[3]<<8)+(nb[2]<<6)+(nb[1]<<4)+(nb[0]<<2)+nb[7];
	hash[6] = (nb[4]<<14)+(nb[3]<<12)+(nb[2]<<10)+(nb[1]<<8)+(nb[0]<<6)+(nb[7]<<4)+(nb[6]<<2)+nb[5];
	hash[7] = (nb[2]<<14)+(nb[1]<<12)+(nb[0]<<10)+(nb[7]<<8)+(nb[6]<<6)+(nb[5]<<4)+(nb[4]<<2)+nb[3];
	int hash_max = 0;
	for( int i=0; i<8; i++ ){
		if( hash[i]>hash_max ) hash_max = hash[i];
	}
	return hash_max;
}

bool Board::checkPattern(int agent, int row, int col){
	int ph = getPatternHash(row,col);
	if( row>0 && row<BOARD_SIZE-1 && col>0 && col<BOARD_SIZE-1){
		if(false
			|| ph==36865 || ph==36929|| ph==36993 || ph==37121 || ph==37185 || ph==37264|| ph==37377 || ph==37441 || ph==41360
			|| ph==37121 || ph==37264|| ph==37185 || ph==37393 || ph==41361 || ph==37457|| ph==37137 || ph==37265 || ph==37201
			|| ph==37377 || ph==37441|| ph==41360 || ph==37393 || ph==37457 || ph==41361|| ph==37409 || ph==37473 || ph==41362

			|| ph==24898 || ph==37472|| ph==24834 || ph==25184 || ph==41568 || ph==25090|| ph==24642 || ph==33376 || ph==24578
			|| ph==25185 || ph==41569|| ph==25106 || ph==25186 || ph==41570 || ph==25122|| ph==25184 || ph==41568 || ph==37201
			|| ph==24914 || ph==37473|| ph==24850 || ph==25185 || ph==41569 || ph==25090|| ph==24898 || ph==37472 || ph==24834

			|| ph==36880 || ph==37136|| ph==37392 || ph==36896 || ph==37152 || ph==37408|| ph==36864 || ph==37120 || ph==37376

			|| ph==24848 || ph==24592|| ph==24864 || ph==25120 || ph==24608 || ph==24832|| ph==25088 || ph==24576 || ph==25104

			|| ph==37920 || ph==37904|| ph==37888 || ph==37921 || ph==37905 || ph==37889|| ph==41477 || ph==41221 || ph==40965
			|| ph==38176 || ph==38160|| ph==38144 || ph==38177 || ph==38161 || ph==38145|| ph==41493 || ph==41237 || ph==40981
			|| ph==38432 || ph==38416|| ph==38400 || ph==38433 || ph==38417 || ph==38401|| ph==41509 || ph==41253 || ph==40997

			|| ph==42001 || ph==42017|| ph==41985 || ph==42513 || ph==42529 || ph==42497|| ph==42257 || ph==42273 || ph==42241
			|| ph==42002 || ph==42018|| ph==41986 || ph==42514 || ph==42530 || ph==42498|| ph==42258 || ph==42274 || ph==42242
			|| ph==42003 || ph==42019|| ph==41987 || ph==42515 || ph==42531 || ph==42499|| ph==42259 || ph==42275 || ph==42243

			|| (agent==GO_BLACK && (ph==40977 ||ph==40993 ||ph==40961 ||ph==41489 
			||ph==41505 ||ph==41473 ||ph==41233 ||ph==41249 ||ph==41217 )) 
			|| (agent==GO_WHITE && (ph==24609 ||ph==24593 ||ph==24577 ||ph==24865
			||ph==24849 ||ph==24833 ||ph==25121 ||ph==25105 ||ph==25089 )) 

			|| ph==25600|| ph==25616|| ph==25632|| ph==25856|| ph==25872|| ph==25888|| ph==26112|| ph==26128|| ph==26144|| 
			ph==34368|| ph==36964|| ph==41060|| ph==38464|| ph==38465|| ph==41061|| ph==42560|| ph==42561|| ph==42562||
			ph==25668|| ph==25684|| ph==25700|| ph==25924|| ph==25940|| ph==25956|| ph==26180|| ph==26196|| ph==26212|| 
			ph==34372|| ph==37988|| ph==42084|| ph==38468|| ph==38469|| ph==42085|| ph==42564|| ph==42565|| ph==42566|| 
			ph==34368|| ph==36964|| ph==41060|| ph==34384|| ph==37220|| ph==41316|| ph==34400|| ph==37476|| ph==41572|| 
			ph==34372|| ph==37988|| ph==42084|| ph==34388|| ph==38244|| ph==42340|| ph==34404|| ph==38500|| ph==42596|| 
			ph==34916|| ph==39012|| ph==43108|| ph==34384|| ph==39268|| ph==43364|| ph==42568|| ph==42569|| ph==43620|| 
			ph==25856|| ph==25872|| ph==25888|| ph==25857|| ph==25873|| ph==25889|| ph==26113|| ph==26129|| ph==26145|| 
			ph==34384|| ph==37220|| ph==41316|| ph==38480|| ph==38481|| ph==41317|| ph==42576|| ph==42577|| ph==42578|| 
			ph==25924|| ph==25940|| ph==25956|| ph==25925|| ph==25941|| ph==25957|| ph==26196|| ph==26197|| ph==26213|| 
			ph==34388|| ph==38244|| ph==42340|| ph==38484|| ph==38485|| ph==42341|| ph==42580|| ph==42581|| ph==42582|| 
			ph==38464|| ph==38465|| ph==41061|| ph==38480|| ph==38481|| ph==41317|| ph==38496|| ph==38497|| ph==41573|| 
			ph==38468|| ph==38469|| ph==42085|| ph==38484|| ph==38485|| ph==42341|| ph==38500|| ph==38501|| ph==42597|| 
			ph==38472|| ph==39268|| ph==43364|| ph==38488|| ph==39269|| ph==43365|| ph==42584|| ph==42585|| ph==43621|| 
			ph==26112|| ph==26128|| ph==26144|| ph==26113|| ph==26129|| ph==26145|| ph==26114|| ph==26130|| ph==26146|| 
			ph==34400|| ph==37476|| ph==41572|| ph==38496|| ph==38497|| ph==41573|| ph==42592|| ph==42593|| ph==42594|| 
			ph==26180|| ph==26192|| ph==26212|| ph==26196|| ph==26193|| ph==26213|| ph==26212|| ph==26213|| ph==26214|| 
			ph==34404|| ph==38500|| ph==42596|| ph==38500|| ph==38501|| ph==42597|| ph==42596|| ph==42597|| ph==42598|| 
			ph==42560|| ph==42561|| ph==42562|| ph==42576|| ph==42577|| ph==42578|| ph==42592|| ph==42593|| ph==42594|| 
			ph==42564|| ph==42565|| ph==42566|| ph==42580|| ph==42581|| ph==42582|| ph==42596|| ph==42597|| ph==42598|| 
			ph==42568|| ph==42569|| ph==43620|| ph==42584|| ph==42585|| ph==43621|| ph==42600|| ph==42601|| ph==43622

			|| ph==38928|| ph==41225|| ph==39184|| ph==38944|| ph==41481|| ph==39200|| ph==38912|| ph==40969|| ph==39168
			|| ph==38992|| ph==42249|| ph==39248|| ph==39008|| ph==42505|| ph==39264|| ph==38976|| ph==41993|| ph==39232
			|| ph==38992|| ph==41289|| ph==39188|| ph==39008|| ph==41545|| ph==39204|| ph==38976|| ph==41033|| ph==39172
			|| ph==38996|| ph==42313|| ph==39252|| ph==39012|| ph==42569|| ph==39268|| ph==38980|| ph==42057|| ph==39236
			|| ph==39060|| ph==43397|| ph==39316|| ph==42136|| ph==43593|| ph==42137|| ph==39044|| ph==43396|| ph==39300
			|| ph==39060|| ph==42377|| ph==39256|| ph==42136|| ph==42648|| ph==42392|| ph==39044|| ph==42121|| ph==39240
			|| ph==43160|| ph==39048|| ph==43400|| ph==39304|| ph==39184|| ph==41241|| ph==39185|| ph==39200|| ph==41497
			|| ph==39201|| ph==39168|| ph==40985|| ph==39169|| ph==39188|| ph==42265|| ph==39249|| ph==39204|| ph==42521
			|| ph==39265|| ph==39172|| ph==42009|| ph==39233|| ph==39252|| ph==42329|| ph==39253|| ph==39268|| ph==42585
			|| ph==39269|| ph==39236|| ph==42073|| ph==39237|| ph==39256|| ph==43413|| ph==39317|| ph==42392|| ph==43609
			|| ph==42393|| ph==39240|| ph==43412|| ph==39316|| ph==39248|| ph==41305|| ph==39249|| ph==39264|| ph==41561
			|| ph==39265|| ph==39232|| ph==41049|| ph==39233|| ph==39316|| ph==42393|| ph==39317|| ph==42137|| ph==42649
			|| ph==42393|| ph==39300|| ph==42137|| ph==39316|| ph==39320|| ph==43417|| ph==39321|| ph==43416|| ph==43673
			|| ph==43417|| ph==39304|| ph==43416|| ph==39320|| ph==41225|| ph==41257|| ph==41241|| ph==41481|| ph==41513
			|| ph==41497|| ph==40969|| ph==41001|| ph==40985|| ph==41289|| ph==42281|| ph==41305|| ph==41545|| ph==42537
			|| ph==41561|| ph==41033|| ph==42025|| ph==41049|| ph==42249|| ph==42281|| ph==42265|| ph==42505|| ph==42537
			|| ph==42521|| ph==41993|| ph==42025|| ph==42009|| ph==42313|| ph==42345|| ph==42265|| ph==42569|| ph==42601
			|| ph==42521|| ph==42057|| ph==42089|| ph==42073|| ph==42377|| ph==43429|| ph==42393|| ph==42648|| ph==43625
			|| ph==42648|| ph==43625|| ph==42649|| ph==42121|| ph==43428|| ph==42137|| ph==43397|| ph==43429|| ph==43413
			|| ph==43593|| ph==43561|| ph==43609|| ph==43396|| ph==43428|| ph==43412|| ph==43401|| ph==43672|| ph==43400
			|| ph==43432|| ph==43416


			|| ph==63747|| ph==63755|| ph==63751|| ph==63767|| ph==63771|| ph==63763|| ph==63783|| ph==64027|| ph==63779
			|| ph==62979|| ph==63527|| ph==62983|| ph==63783|| ph==62999|| ph==62995|| ph==63015|| ph==64039|| ph==63011
			|| ph==63559|| ph==63815|| ph==64071|| ph==63575|| ph==63831|| ph==64087|| ph==63591|| ph==63847|| ph==64103
			|| ph==63623|| ph==63655|| ph==63639|| ph==63895|| ph==63911|| ph==63879|| ph==64135|| ph==64151|| ph==64167

			|| ph==34816 || ph==17408
			|| ph==36865 || ph==24578
			|| ph==33920 || ph==33996 || ph==33924 || ph==38020 || ph==37252 || ph==38276
			|| ph==33796 || ph==34308 || ph==42116 || ph==34340 || ph==42118
			|| ph==36868 || ph==34304 || ph==41985 || ph==38400
		){
			return true;
		}
		return false;
	}
	else{
		if(false
			|| ph==63747 || ph==62979
			|| ph==63559 || ph==63623
			|| (agent==GO_BLACK && ph==62019) || (agent==GO_WHITE && ph==61827)
			|| (agent==GO_BLACK && ph==63043) || (agent==GO_WHITE && ph==63875)
			|| (agent==GO_BLACK && ph==63879) || (agent==GO_WHITE && ph==63591)
		){
			return true;
		}
		return false;
	}
}

void Board::initZobristRand(){
	for( int i=0; i<BOARD_SIZE; i++ ){
		for( int j=0; j<BOARD_SIZE; j++ ){
			for( int a=0; a<3; a++ ){
				__int64 r1 = rand();
				__int64 r2 = rand();
				__int64 r3 = rand();
				__int64 r4 = rand();
				zobrist_rand[a][i][j] = r1<<48 + r2<<32 + r3<<16 + r4;
			}
		}
	}
}


__int64 Board::getZobristHash(){
	__int64 hash = 0;
	for( int i=0; i<BOARD_SIZE; i++ ){
		for( int j=0; j<BOARD_SIZE; j++ ){
			int a = data[i][j];
			hash = hash ^ zobrist_rand[a][i][j];
		}
	}
	return hash;
}