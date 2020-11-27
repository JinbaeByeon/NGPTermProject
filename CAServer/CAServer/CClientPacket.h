#pragma once
#include"stdafx.h"

#include "CMap.h"

class Packet
{
public:
	int type;
};

class PlayerPacket : public Packet
{
public:
	int idx_player = 0;
	int left = 0, top = 0;
	unsigned short status;
};

class BubblePacket : public Packet
{
public:
	int power;
	int left = 0, top = 0;
};

enum ClientPacket {
	empty = 0,
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


	void PlayerPacketProcess(CMap M, ClientPacket CP, PlayerPacket *P, int idx);
	void BubblePacketProcess(CMap M, ClientPacket CP, BubblePacket *B);

};