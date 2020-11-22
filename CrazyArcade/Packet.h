#pragma once
class Packet
{
public:
	int type;
};

class PlayerPacket : public Packet
{
	int idx_player;
	int x, y;
	unsigned short status;
};

class BubblePacket : public Packet
{
	int power;
	int x, y;
};

enum ClientPacket {
	input_left = 1,
	input_right = 2,
	input_top = 4,
	input_bottom = 8,
	input_space = 16,
	input_ctrl = 32,
	state_dead = 64,
	state_trapped = 128,
};