#pragma once
#include "CClientPacket.h"

//void PacketFunc::PlayerPacketProcess(InputPacket Recv_P, InputPacket* P
//    ,int idx)
//{
//    P->idx_player = Recv_P.idx_player;
//    P->type = 1;
//    /*if (CP == ClientPacket::input_left)
//        P->left -= 5;
//    else if (CP == ClientPacket::input_right)
//        P->left += 5;
//    else if (CP == ClientPacket::input_top)
//        P->top -= 5;
//    else if (CP == ClientPacket::input_bottom)
//        P->top += 5;*/
//    P->x = Recv_P.x;
//    P->y = Recv_P.y;
//    P->status = Recv_P.status;
//}
//
//void PacketFunc::BubblePacketProcess(InputPacket Recv_P, InputPacket* P)
//{
//    P->type = 2;
//    P->power = Recv_P.power;
//    P->x = Recv_P.x;
//    P->y = Recv_P.y;
//}

void PacketFunc::InitPlayer(CMap m_Map, InputPacket *Send_P, int idx)
{
    printf("¿©±â\n");
    if (idx == 0)
        //Send_P = &InputPacket(idx, m_Map.Tile[0][0].left, m_Map.Tile[0][0].left, 1);
        Send_P->x = m_Map.Tile[0][0].left, Send_P->y = m_Map.Tile[0][0].top
        , Send_P->status = 0;
    else if (idx == 1)
        Send_P->x = m_Map.Tile[0][0].left, Send_P->y = m_Map.Tile[0][0].top
        , Send_P->status = 0;
    else if (idx == 2)
        Send_P->x = m_Map.Tile[0][0].left, Send_P->y = m_Map.Tile[0][0].top
        , Send_P->status = 0;
}

void PacketFunc::InitPacket(InputPacket* P)
{
    P->idx_player = NULL;
    P->power = NULL;
    P->status = NULL;
    P->x = NULL;
    P->y = NULL;
    P->power = NULL;
}