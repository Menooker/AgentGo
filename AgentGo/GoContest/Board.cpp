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
	for ( i=0; i<BOARD_SIZE*BOARD_SIZE; i++){
		space_list[i][0]=i-1;
		space_list[i][1]=i+1;
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

	#ifdef GO_BOARD_TIME
		time_put += clock()-cl;
	#endif
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
			printf("%d ",data[i][j]);
		}
		printf("\n");
	}
	printf("\n");
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
	if( game_ending ){
		//return Piece();
		return getRandomPieceComplex(agent);
	}
	bool flag = true;
	int count = 0;
	while(flag){
		flag = false;
		count ++;
		row = rand() % BOARD_SIZE;
		col = rand() % BOARD_SIZE;
		if( data[row][col]!=GO_NULL
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