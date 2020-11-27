#include "CMap.h"

void CMap::Init_Map()
{
    // ¸Ê ÁÂÇ¥ ¼³Á¤
    for (int i = 0; i < Tile_CountY; i++)
        for (int j = 0; j < Tile_CountX; j++) {
            Tile[i][j] = { StartX + j * Tile_CX,StartY + i * Tile_CY, StartX + (j + 1) * Tile_CX,StartY + (i + 1) * Tile_CY };
            if ((i == 0 || i == 2 || i == 4 || i == 6) && (j == 10 || j == 12 || j == 14)) {
                Tile_Enable_Move[0][i][j] = FALSE;
                if (rand() % 3 == 0)
                    isTree[i][j] = TRUE;
                if (rand() % 3)
                    isHouse0[i][j] = TRUE;
                else if (rand() % 2)
                    isHouse1[i][j] = TRUE;
            }
            else if ((i == 6 || i == 8 || i == 10 || i == 12) && (j == 0 || j == 2 || j == 4)) {
                Tile_Enable_Move[0][i][j] = FALSE;
                if (rand() % 3 == 0)
                    isTree[i][j] = TRUE;
                if (rand() % 3)
                    isHouse0[i][j] = TRUE;
                else if (rand() % 2)
                    isHouse1[i][j] = TRUE;
            }
            else if ((i == 1 || i == 3 || i == 5) && (j == 1 || j == 3 || j == 5)) {
                Tile_Enable_Move[0][i][j] = FALSE;
                if (rand() % 3 == 0)
                    isTree[i][j] = TRUE;
                if (rand() % 3)
                    isHouse0[i][j] = TRUE;
                else if (rand() % 2)
                    isHouse1[i][j] = TRUE;
            }
            else if ((i == 7 || i == 9 || i == 11) && (j == 9 || j == 11 || j == 13)) {
                Tile_Enable_Move[0][i][j] = FALSE;
                if (rand() % 3 == 0)
                    isTree[i][j] = TRUE;
                if (rand() % 3)
                    isHouse0[i][j] = TRUE;
                else if (rand() % 2)
                    isHouse1[i][j] = TRUE;
            }
            else if ((i == 1 || i == 3 || i == 9 || i == 11) && (j == 5 || j == 9)) {
                Tile_Enable_Move[0][i][j] = FALSE;
                if (rand() % 3 == 0)
                    isTree[i][j] = TRUE;
                if (rand() % 3)
                    isHouse0[i][j] = TRUE;
                else if (rand() % 2)
                    isHouse1[i][j] = TRUE;
            }
            else {
                Tile_Enable_Move[0][i][j] = TRUE;
                if ((j != 6 && j != 7 && j != 8) &&
                    !(j == 0 && (i == 0 || i == 1 || i == 11)) &&
                    !(j == 1 && (i == 0 || i == 11 || i == 12)) &&
                    !(j == 12 && i == 1) &&
                    !(j == 13 && (i == 0 || i == 1 || i == 12)) &&
                    !(j == 14 && (i == 1 || i == 11 || i == 12)) &&
                    !(j == 5 && i == 7) && !(j == 9 && i == 5)) {
                    Box[i][j] = { Tile[i][j].left,Tile[i][j].top - 4,Tile[i][j].right,Tile[i][j].bottom };
                    isBox[0][i][j] = TRUE;
                    if (rand() % 2)
                        isBox1[i][j] = TRUE;
                }
            }


            if ((i == 1 && j == 2) || (i == 2 && j == 1) || (i == 1 && j == 1) || (j == 13 && i == 1) || (j == 13 && i == 2) || (j == 12 && i == 1)) {
                Tile_Enable_Move[1][i][j] = FALSE;
                isSteel[i][j] = TRUE;
            }
            else if (i == 1 && (j == 3 || j == 4 || j == 5 || j == 9 || j == 10 || j == 11)
                || (j == 1 && (i == 3 || i == 4 || i == 5)) || (j == 13 && (i == 3 || i == 4 || i == 5))) {
                Tile_Enable_Move[1][i][j] = TRUE;
                isBox[1][i][j] = TRUE;
            }
            else if ((j == 2 || j == 12) && i == 5) {
                Tile_Enable_Move[1][i][j] = FALSE;
                isStone[i][j] = TRUE;
            }
            else
                Tile_Enable_Move[1][i][j] = TRUE;
        }
}

BOOL CMap::Collision(RECT rect, int x, int y)
{
    if (rect.left < x && x < rect.right && rect.top < y && y < rect.bottom)
        return TRUE;
    return FALSE;
}