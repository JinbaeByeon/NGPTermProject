#pragma once
#include"stdafx.h"
#include "CMap.h"

// ÀÌµ¿ = 1, ¹°Ç³¼± = 2
struct Packet
{
public:
	Packet() {}
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
	end,
};

struct InputPacket : public Packet
{
public:
	InputPacket() { }
	InputPacket(int idx, int x, int y, u_short stat) :Packet(player) { idx_player = idx; this->x = x; this->y = y; status = stat; }
	InputPacket(int idx, int x, int y) :Packet(bubble) { this->x = x; this->y = y; this->idx_player = idx; }

	int idx_player;
	int x , y;
	u_short status;
};

struct PlayerPacket : public Packet
{
public:
	PlayerPacket() {}
	PlayerPacket(int idx, int x, int y, u_short stat) :Packet(player) { idx_player = idx; this->x = x; this->y = y; status = stat; }
	int idx_player = 0;
	int x = 0, y = 0;
	u_short status = 0;
};

struct BubblePacket : public Packet
{
public:
	int power;
	int x, y;
};

struct ItemPacket : public Packet
{
public:
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

class PacketFunc {
public:
	PacketFunc() {}
	~PacketFunc() {}


	/*void PlayerPacketProcess(CMap M, InputPacket Recv_P, InputPacket*P
		, int idx);
	void BubblePacketProcess(CMap* M, InputPacket Recv_P, InputPacket* P);*/

	void InitPlayer(CMap m_Map, InputPacket* Send_P, int idx);
	void InitPacket(InputPacket* P);
	void InitItem(ItemPacket* P, CMap m_Map);
};

enum Item { 
	Ball = 1, 
	OnePower = 6, 
	Speed = 11, 
	MaxPower = 16, 
	RedDevil = 21 
};

enum Status {
	MOVE_LEFT = 3,
	MOVE_RIGHT = 2,
	MOVE_UP = 0,
	MOVE_DOWN = 1,
	STOP = 4,
	IN_BUBBLE = 5,
	DEAD = 6
};