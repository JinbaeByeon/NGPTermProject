#pragma once


// ÀÌµ¿ = 1, ¹°Ç³¼± = 2
struct Packet
{
	Packet(int type) { this->type = type; }
	int type;
};

enum PacketType
{
	index,
	start,
	player,
	item,
	bubble,
	ready,
};

struct InputPacket : public Packet
{
	InputPacket(int idx, int x, int y, u_short stat) :Packet(player) { idx_player = idx; this->x = x; this->y = y; status = stat; }
	InputPacket(int idx, int x, int y) :Packet(bubble) { this->x = x; this->y = y; this->idx_player = idx; }

	int idx_player;
	int x, y;
	u_short status = 0;
};

struct PlayerPacket : public Packet
{
	PlayerPacket(int idx, int x, int y, u_short stat):Packet(player) { idx_player = idx; this->x = x; this->y = y; status = stat; }	
	int idx_player;
	int x, y;
	u_short status;
};

struct BubblePacket : public Packet
{
	int power;
	int x, y;
};


struct ItemPacket : public Packet
{
	int x, y;
	int value;
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