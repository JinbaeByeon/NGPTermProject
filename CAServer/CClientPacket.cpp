#pragma once
#include "CClientPacket.h"


void PacketFunc::InitPlayer(CMap m_Map, InputPacket *Send_P, int idx)
{
    Send_P->type = PacketType::player;
    Send_P->idx_player = idx;
    if (idx == 0)
        Send_P->x = m_Map.Tile[0][0].left, Send_P->y = m_Map.Tile[0][0].top
        , Send_P->status = 0;
    else if (idx == 1)
        Send_P->x = m_Map.Tile[12][1].left, Send_P->y = m_Map.Tile[12][1].top
        , Send_P->status = 0;
    else if (idx == 2)
        Send_P->x = m_Map.Tile[1][13].left, Send_P->y = m_Map.Tile[1][13].top
        , Send_P->status = 0;
    else if (idx == 3)
        Send_P->x = m_Map.Tile[12][13].left, Send_P->y = m_Map.Tile[12][13].top
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

void PacketFunc::InitItem(ItemPacket* P, CMap m_Map)
{
    int ItemValue;
    srand((unsigned)time(NULL));
    for (int i = 0; i < m_Map.Tile_CountY; i++)
    {
        for (int j = 0; j < m_Map.Tile_CountX; j++) {
            ItemValue = rand() % 30;
            if (ItemValue != 0 && ItemValue != 7) {
                P->x = m_Map.Tile[i][j].left;
                P->y = m_Map.Tile[i][j].top;
                P->type = PacketType::item;
                P->value = ItemValue;
            }
        }
    }
}