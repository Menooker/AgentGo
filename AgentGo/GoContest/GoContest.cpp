// GoContest.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "Board.h"
#include <iostream>

using namespace std;

int _tmain(int argc, _TCHAR* argv[])
{
	Board board;
	board.put(GO_BLACK,1,0);
	board.put(GO_BLACK,1,1);
	board.put(GO_BLACK,0,1);
	board.put(GO_BLACK,0,0);
	board.print();
	cin.get();
	return 0;
}

