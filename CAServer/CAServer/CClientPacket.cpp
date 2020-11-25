#pragma once
#include "CClientPacket.h"


void PacketFunc::PlayerPacketProcess(CMap M,ClientPacket CP, PlayerPacket* P, int idx)
{
    P->idx_player = idx;
    P->type = 1;
    if (CP == ClientPacket::input_left)
    {
        P->left -= 5;
        for (int i = 0; i < M.Tile_CountY; i++)
            for (int j = 0; j < M.Tile_CountX; j++) {
                if ((M.Collision(M.Tile[i][j], P->left, P->top)
                    || M.Collision(M.Tile[i][j], P->left, P->top + (M.Player_CY))
                    && (M.isBox[0][i][j] || !M.Tile_Enable_Move[0][i][j]))) {
                    P->left += 5;
                }
            }
        if (P->left <= M.StartX + 10)
            P->left = M.StartX;
    }
    else if (CP == ClientPacket::input_right)
    {
        P->left += 5;
        for (int i = 0; i < M.Tile_CountY; i++)
            for (int j = 0; j < M.Tile_CountX; j++) {
                if ((M.Collision(M.Tile[i][j], P->left + (M.Player_CX), P->top)
                    || M.Collision(M.Tile[i][j], P->left + (M.Player_CX), P->top + (M.Player_CY))
                    && (M.isBox[0][i][j] || !M.Tile_Enable_Move[0][i][j]))) {
                    P->left -= 5;
                }
            }
        if ((P->left+ M.Player_CX) >= M.Tile[12][14].right - 10)
            P->left = M.Tile[12][14].right - M.Player_CX;
    }
    else if (CP == ClientPacket::input_top)
    {
        P->top -= 5;
        for (int i = 0; i < M.Tile_CountY; i++)
            for (int j = 0; j < M.Tile_CountX; j++) {
                if ((M.Collision(M.Tile[i][j], P->left, P->top)
                    || M.Collision(M.Tile[i][j], P->left + (M.Player_CX), P->top)
                    && (M.isBox[0][i][j] || !M.Tile_Enable_Move[0][i][j]))) {
                    P->top += 5;
                }
            }
        if (P->top <= M.StartY+5)
            P->top = M.StartY;
    }
    else if (CP == ClientPacket::input_bottom)
    {
        P->top += 5;
        for (int i = 0; i < M.Tile_CountY; i++)
            for (int j = 0; j < M.Tile_CountX; j++) {
                if ((M.Collision(M.Tile[i][j], P->left, P->top + (M.Player_CY))
                    || M.Collision(M.Tile[i][j], P->left + (M.Player_CX), P->top + (M.Player_CY))
                    && (M.isBox[0][i][j] || !M.Tile_Enable_Move[0][i][j]))) {
                    P->top -= 5;
                }
            }
        if ((P->top + M.Player_CY) >= M.Tile[12][14].bottom - 10)
            P->top = M.Tile[12][14].bottom - M.Player_CY;
    }
}

void PacketFunc::BubblePacketProcess(CMap M, ClientPacket CP, BubblePacket* B)
{
    B->power = 1;
    B->left = 1;
    B->top = 2;
    B->type = 2;
}