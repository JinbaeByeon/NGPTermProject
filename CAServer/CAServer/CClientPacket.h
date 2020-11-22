#pragma once
#include"stdafx.h"

enum class CPacket : unsigned short
{
	left = 1,
	right = 2,
	top = 4,
	bottom = 8,
	space = 16,
	ctrl = 32,
	dead = 64,
	trapped = 128
};

struct Packet {
	int type;
};

struct PlayerPacket {
	int idx;
	int x, y;
	u_short status;
};

struct BubblePacket {
	int power;
	int x, y;
};

struct StartPacket {
	int x, y;
};