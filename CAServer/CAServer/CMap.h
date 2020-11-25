#pragma once
#include "stdafx.h"

class CMap
{
public:
	CMap() {};
	~CMap() {};


	bool Tile_Enable_Move[2][13][15];
	bool isBox[2][13][15];
	bool MoveBox[13][15], isTree[13][15], isBush[13][15];
	bool isBox1[13][15], isHouse0[13][15], isHouse1[13][15];
	bool  isStone[13][15], isSteel[13][15];
	RECT Tile[13][15], Box[13][15];
	const int Tile_CX = 40;
	const int Tile_CY = 40;
	const int Tile_CountX = 15;
	const int Tile_CountY = 13;

	const int StartX = 20;		// ¿Œ∞‘¿” ∏  Ω√¿€ X¡¬«•
	const int StartY = 40;		// ¿Œ∞‘¿” ∏  Ω√¿€ Y¡¬«•

	const int Tree_CX = 40;
	const int Tree_CY = 70;

	const int Bush_CX = 48;
	const int Bush_CY = 65;

	const int House_CX = 40;
	const int House_CY = 57;

	const int Box_CX = 40;
	const int Box_CY = 45;

	const int Bubble_CX = 40;
	const int Bubble_CY = 40;

	const int Player_CX = 34;
	const int Player_CY = 34;

	void Init_Map();

	BOOL Collision(RECT rect, int x, int y);
};

