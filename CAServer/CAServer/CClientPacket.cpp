#pragma once
#include "CClientPacket.h"


void PacketFunc::InitPlayer(CMap m_Map, InputPacket *Send_P, int idx)
{
    Send_P->type = player;
    Send_P->idx_player = idx;
    if (idx == 0)
        Send_P->x = m_Map.Tile[0][0].left, Send_P->y = m_Map.Tile[0][0].top
        , Send_P->status = 0;
    else if (idx == 1)
        Send_P->x = m_Map.Tile[12][13].left, Send_P->y = m_Map.Tile[12][13].top
        , Send_P->status = 0;
    else if (idx == 2)
        Send_P->x = m_Map.Tile[0][0].left, Send_P->y = m_Map.Tile[0][0].top
        , Send_P->status = 0;
}

void PacketFunc::InitPacket(InputPacket* P)
{
    P->idx_player = NULL;
    P->status = NULL;
    P->x = NULL;
    P->y = NULL;
    P->status = NULL;
}