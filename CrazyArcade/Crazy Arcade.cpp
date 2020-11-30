#include "stdafx.h"

#include "Constant.h"
#include "resource.h"
#include <time.h>

#include "SoundMgr.h"
#include "Socket_Programming.h"
#include "Packet.h"


HINSTANCE hInst;
HWND hwnd;

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
void CALLBACK TimeProc_Bubble_BfBoom(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime);
void CALLBACK TimeProc_Bubble_Flow(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime);
void CALLBACK TimeProc_P1_Move(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime);
void CALLBACK TimeProc_InBubble(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime);
void CALLBACK TimeProc_Die(HWND hWnd, UINT uMsg, UINT ideEvent, DWORD dwTime);
void CALLBACK TimeProc_Text(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime);
void SetPos();
void Animation();
void SetBitmap();
void ChainBomb(RECT Bubble, int Power);
BOOL InBubble_Collision(RECT rt, int t, int l, int b, int r);
BOOL Collision(RECT rect, int x, int y);
void KEY_DOWN_P1(HWND hWnd);
void KEY_UP_P1(WPARAM wParam, HWND hWnd);
void KEY_DOWN_P2(HWND hWnd);
void KEY_UP_P2(WPARAM wParam, HWND hWnd);

// 소켓 프로그래밍 변수
// 이벤트
HANDLE hRecvEvent, hSendEvent, hConnectEvent, hBubbleEvent, hPlayerEvent, hInputEvent;
// 소켓
SOCKET sock;
// 패킷
InputPacket *Recv_Player_Packet;
InputPacket* Send_Client_Packet = 0;
int Client_Idx;
int nPlayer = 0;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
	HWND hwnd;
	MSG msg;
	WNDCLASS WndClass;
	RECT rcWindow = { 0,0,WINDOW_WIDTH,WINDOW_HEIGHT };
	AdjustWindowRect(&rcWindow, WS_OVERLAPPEDWINDOW, false);

	hInst = hInstance;
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	WndClass.lpfnWndProc = WndProc;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = hInstance;
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
	WndClass.lpszMenuName = NULL;
	WndClass.lpszClassName = L"Window Class Name";
	RegisterClass(&WndClass);
	hwnd = CreateWindow(L"Window Class Name", L"Crazy Arcade", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		rcWindow.right - rcWindow.left,
		rcWindow.bottom - rcWindow.top,
		NULL, NULL, hInstance, NULL);
	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	// 이벤트 생성
	hRecvEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hRecvEvent == NULL) return 1;
	hSendEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hSendEvent == NULL) return 1;
	hConnectEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hConnectEvent == NULL) return 1;
	hPlayerEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hPlayerEvent == NULL) return 1;
	hBubbleEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hBubbleEvent == NULL) return 1;
	hInputEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hInputEvent == NULL) return 1;

	// 소켓 통신 스레드 생성
	CreateThread(NULL, 0, SendClient, NULL, 0, NULL);
	CreateThread(NULL, 0, RecvClient, NULL, 0, NULL);


	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}


// 선언
enum Player_Position { LEFT = 3, RIGHT = 2, UP = 0, DOWN = 1 };
enum Monster_Position { M_DOWN, M_LEFT, M_RIGHT, M_UP, M_DIE, M_REMOVE };
enum GAME_BG { MENU = 1, ROBBY, INGAME };
enum Item { Ball = 1, OnePower = 6, Speed = 11, MaxPower = 16, RedDevil = 21 };
enum Timer { P1 = 0, P2 = 1, P3 = 2, P4 = 3, Bubble_BfBoom, Bubble_Flow, In_Bubble, Die, Monster_Move, Recv_Bubble };

int GameState = MENU;
int ItemValue;
int Itemset[2][13][15];

HDC hdc;
HBITMAP hBit;         // 여기에 그려서 memdc -> hdc 에 넣음
HBITMAP P1_Bit, P2_Bit, BGBit_InGame;
HBITMAP Player_Bit[4];

HBITMAP TileBit, Tile_Enable, Tile_Disable;
HBITMAP Box_Bit0, Box_Bit1, Box_Bit2;
HBITMAP House_Bit0, House_Bit1, House_Bit2, TreeBit;
HBITMAP Mon1Bit, Mon2Bit;
HBITMAP Bubble, Bubble_Bomb;
HBITMAP LogoMenu, LogoStart;
HBITMAP LOBBY, VIL, BOSS, MAP1, MAP2;
HBITMAP P1_NIDDLE_ON;
HBITMAP P1_NIDDLE_OFF;

HBITMAP Lobby_Start;

HBITMAP P1_On, P2_On;
HBITMAP Player_On;
int Player_RobbyX[4];
int Player_RobbyY[4];

HBITMAP P1_nOn;


HBITMAP Items;
HBITMAP Tile2, Steel2, Stone2, Block2;

RECT Crect;

RECT Player1, Player2;
RECT Player[4];

RECT Tile[13][15], Box[13][15];
RECT Tile_Bubble1[7], Tile_Bubble2[7];	//P1의 물풍선 좌표저장, P2의 물풍선 좌표저장
RECT Tile_Bubble[4][7];

RECT Temp;	//Intersectrect에 사용 할 임시 RECT	


bool Tile_Enable_Move[2][13][15];		// 0: 맵1, 1: 맵2
bool isBox[2][13][15];					// 0: 맵1, 1: 맵2

// 맵1
bool MoveBox[13][15], isTree[13][15], isBush[13][15];
bool isBox1[13][15], isHouse0[13][15], isHouse1[13][15];

// 맵2
bool  isStone[13][15], isSteel[13][15];


// 캐릭터 비트맵 설정
int xPos_P1, yPos_P1;									// (0 : 위, 1 : 아래, 2 : 오른쪽, 3 : 왼쪽, 4 : 물방울, 5 : 바늘, 6 : 죽음)
int xPos_P2, yPos_P2;									// (0 : 위, 1 : 아래, 2 : 오른쪽, 3 : 왼쪽, 4 : 물방울, 5 : 바늘, 6 : 죽음)

int xPos_Player[4];
int yPos_Player[4];




int xPos_Tile;
int xPos_Bubble;
int P1_Bubble_cnt[7], P2_Bubble_cnt[7];

int Bubble_cnt[4][7];


BOOL P1_Bubble[7];   //최대 폭탄을 놓을 수 있는 갯수는 7개
BOOL P2_Bubble[7];
BOOL Player_Bubble[4][7];


BOOL P1_Bubble_Boom[7];
BOOL P2_Bubble_Boom[7];
BOOL Player_Bubble_Boom[4][7];



BOOL Player_Bubble_Flow[4][7];
int MoveResource[4][7];
int Power[4];
BOOL bInBubble[4] = { false, };
BOOL bEscape[4] = { false, };
BOOL bDie[4] = { false, };
int BubbleResource[4];
int BubbleCount[4];



BOOL Player_Move[4];


BOOL Box_Break[13][15];
int Box_cnt[13][15];

int M_X, M_Y; //마우스 x,y좌표
int P1_bCount, P2_bCount;
int Player_bCount[4];



RECT GameLogo;									//게임로고와 마우스 좌표 충돌체크를 위해서 선언.
RECT GameStart, GameMap1, GameMap2;				//게임시작버튼, 게임맵 고르는 버튼.
BOOL SelectMap1, SelectMap2;					//맵선택 

RECT P1_NIDDLE;						//바늘과 핀의 좌표값
BOOL P1_N;										//바늘과 핀의 작동여부

RECT P1_Name, P2_Name;
RECT Player_Name[4];

BOOL P1_Live = TRUE, P2_Live = TRUE;
BOOL Player_Live[4] = { false, };

int P1_Speed, P2_Speed;
int Player_Speed[4];


int P1_tSpeed, P2_tSpeed;
int Player_tSpeed[4];


int P1_Dying, P2_Dying;
int Player_Dying[4];

BOOL P1_Remove, P2_Remove;
BOOL Player_Remove[4];


int Sel_Map;


bool bSceneChange = true;
//static bool bEffect[7] = { true,true,true,true,true,true,true };
static bool bEffect2[7] = { true,true,true,true,true,true,true };
static bool bEffect[4][7] = { true, };

static bool Helper = false;
HBITMAP Exit;
RECT ePos;
BOOL Exit_On = true;

HBITMAP Texture;
BOOL TextOn = false;
BOOL Ending = false;
bool mEffect[2] = { true,true };
HBITMAP Help;

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM  lParam)
{
	hwnd = hWnd;

	static HDC mem1dc;
	PAINTSTRUCT ps;
	HBITMAP oldBit1;
	//TransparentBlt(mem1dc, 500, 487, 192, 55, mem2dc, 192, 0,192, 55,RGB(0, 255,0));
	switch (iMsg)
	{
	case WM_CREATE:
		GetClientRect(hWnd, &Crect);
		// 좌표 설정
		SetPos();
		// 비트맵 로드
		SetBitmap();
		SetTimer(hwnd, Bubble_BfBoom, 100, (TIMERPROC)TimeProc_Bubble_BfBoom);   // 물풍선 애니메이션

		for (int i = 0; i < MAX_PLAYER; ++i) {
			Player_Speed[i] = 35;
			Player_bCount[i] = 1;
			Power[i] = 1;
		}

		SelectMap1 = TRUE;
		Sel_Map = 0;

		CSoundMgr::GetInstance()->Initialize();
		CSoundMgr::GetInstance()->LoadSoundFile();
		return 0;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		mem1dc = CreateCompatibleDC(hdc);
		Animation();
		oldBit1 = (HBITMAP)SelectObject(mem1dc, hBit);
		BitBlt(hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, mem1dc, 0, 0, SRCCOPY);

		SelectObject(mem1dc, oldBit1);
		DeleteDC(mem1dc);


		EndPaint(hWnd, &ps);
		return 0;

	case WM_MOUSEMOVE:
		M_X = LOWORD(lParam);
		M_Y = HIWORD(lParam);
		return 0;

	case WM_LBUTTONDOWN:
		M_X = LOWORD(lParam);
		M_Y = HIWORD(lParam);
		if (Collision(GameLogo, M_X, M_Y) && GameState == MENU)
		{
			CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Button_Off.ogg");
			bSceneChange = true;
			SetEvent(hRecvEvent);
			GameState = ROBBY;
		}
		if (GameState == ROBBY)
		{
			if (Collision(GameStart, M_X, M_Y))
			{
				// 레디 패킷 보냄
				Send_Client_Packet = new InputPacket(Client_Idx, 0, 0, 0);
				Send_Client_Packet->type = ready;
				SetEvent(hInputEvent);
			}
			if (Collision(GameMap1, M_X, M_Y))
			{
				CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Button_Off.ogg");
				SelectMap1 = TRUE;
				SelectMap2 = FALSE;
			}
			if (Collision(GameMap2, M_X, M_Y))
			{
				CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Button_Off.ogg");
				SelectMap1 = FALSE;
				SelectMap2 = TRUE;
			}
			if (Collision(P1_NIDDLE, M_X, M_Y))
			{
				CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Button_Off.ogg");
				if (P1_N)
					P1_N = FALSE;
				else
					P1_N = TRUE;
			}


			if (Collision(P1_Name, M_X, M_Y)) {
				CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Button_Off.ogg");
				if (P1_Live)
					P1_Live = FALSE;
				else
					P1_Live = TRUE;
			}

		}
		if (GameState == INGAME)
		{
			if (Collision(ePos, M_X, M_Y)) {
				SetPos();

				for (int i = 0; i < nPlayer; ++i) {
					bInBubble[i] = false;
					bDie[i] = false;
				}

				bSceneChange = true;
				CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Button_Off.ogg");
				GameState = ROBBY;
			}


		}
		return 0;

	case WM_CHAR:
		break;

	case WM_KEYDOWN:
		switch (wParam) {
		case VK_F1:
			Helper = true;
			break;
		case 'P':
			Helper = true;
			break;
		}
		KEY_DOWN_P1(hWnd);


		InvalidateRect(hWnd, NULL, FALSE);
		break;

	case WM_KEYUP:
		KEY_UP_P1(wParam, hWnd);

		InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_DESTROY:
		CSoundMgr::GetInstance()->DestroyInstance();
		PostQuitMessage(0);
		break;
	}

	return DefWindowProc(hWnd, iMsg, wParam, lParam);
}

void Animation()
{
	HDC mem1dc, mem2dc;
	HBITMAP oldBit1, oldBit2;
	int cnt = 0;

	if (hBit == NULL)
		hBit = CreateCompatibleBitmap(hdc, WINDOW_WIDTH, WINDOW_HEIGHT);
	mem1dc = CreateCompatibleDC(hdc);
	mem2dc = CreateCompatibleDC(mem1dc);
	oldBit1 = (HBITMAP)SelectObject(mem1dc, hBit);

	if (GameState == MENU)
	{
		if (bSceneChange)
		{
			CSoundMgr::GetInstance()->StopSoundAll();
			CSoundMgr::GetInstance()->PlayBGM(L"BGM_StageLogin.ogg");
			bSceneChange = false;
		}

		oldBit2 = (HBITMAP)SelectObject(mem2dc, LogoMenu);
		BitBlt(mem1dc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, mem2dc, 0, 0, SRCCOPY);
		if (Collision(GameLogo, M_X, M_Y))
		{
			SelectObject(mem2dc, LogoStart);
			TransparentBlt(mem1dc, 305, 430, BG_X / 2, BG_Y, mem2dc, BG_X / 2, 0, BG_X / 2, BG_Y, RGB(0, 255, 0));
		}
		else
		{
			SelectObject(mem2dc, LogoStart);
			TransparentBlt(mem1dc, 305, 430, BG_X / 2, BG_Y, mem2dc, 0, 0, BG_X / 2, BG_Y, RGB(0, 255, 0));
		}
		if (Helper) {
			SelectObject(mem2dc, Help);
			BitBlt(mem1dc, 150, 150, 594, 264, mem2dc, 0, 0, SRCCOPY);
		}
	}

	if (GameState == ROBBY)
	{
		if (bSceneChange)
		{
			CSoundMgr::GetInstance()->PlayBGM(L"BGM_Prepare.ogg");
			bSceneChange = false;
		}

		oldBit2 = (HBITMAP)SelectObject(mem2dc, LOBBY);
		BitBlt(mem1dc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, mem2dc, 0, 0, SRCCOPY);

		if (Collision(GameStart, M_X, M_Y))
		{
			SelectObject(mem2dc, Lobby_Start);
			TransparentBlt(mem1dc, 500, 487, BG_X / 2, BG_Y, mem2dc, 0, 0, BG_X / 2, BG_Y, RGB(0, 255, 0));
		}
		else
		{
			SelectObject(mem2dc, Lobby_Start);
			TransparentBlt(mem1dc, 500, 487, BG_X / 2, BG_Y, mem2dc, BG_X / 2, 0, BG_X / 2, BG_Y, RGB(0, 255, 0));
		}

		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (!Player_Live[i])
			{
				oldBit2 = (HBITMAP)SelectObject(mem2dc, Player_On);
				BitBlt(mem1dc, Player_RobbyX[i], Player_RobbyY[i], 158, 219, mem2dc, 0, 0, SRCCOPY);
			}

		}

		/*if (P1_Live == FALSE)
		{
			oldBit2 = (HBITMAP)SelectObject(mem2dc, P1_On);
			BitBlt(mem1dc, 38, 114, 158, 188, mem2dc, 0, 0, SRCCOPY);
		}
		if (P2_Live == FALSE)
		{
			oldBit2 = (HBITMAP)SelectObject(mem2dc, P2_On);
			BitBlt(mem1dc, 227, 114, 158, 188, mem2dc, 0, 0, SRCCOPY);
		}*/

		if (SelectMap1)
		{
			oldBit2 = (HBITMAP)SelectObject(mem2dc, MAP1);
			BitBlt(mem1dc, 630, 340, 135, 21, mem2dc, 0, 0, SRCCOPY);
			oldBit2 = (HBITMAP)SelectObject(mem2dc, VIL);
			BitBlt(mem1dc, 477, 342, 149, 129, mem2dc, 0, 0, SRCCOPY);
		}
		if (SelectMap2)
		{
			oldBit2 = (HBITMAP)SelectObject(mem2dc, MAP2);
			BitBlt(mem1dc, 630, 355, 135, 21, mem2dc, 0, 0, SRCCOPY);
			oldBit2 = (HBITMAP)SelectObject(mem2dc, BOSS);
			BitBlt(mem1dc, 477, 342, 149, 129, mem2dc, 0, 0, SRCCOPY);
		}

		if (P1_N && Player_Live[Client_Idx]) {
			oldBit2 = (HBITMAP)SelectObject(mem2dc, P1_NIDDLE_ON);
			BitBlt(mem1dc, Player_RobbyX[Client_Idx] + 6, Player_RobbyY[Client_Idx] + 159, 33, 26, mem2dc, 0, 0, SRCCOPY);
		}
		else if (!P1_N && Player_Live[Client_Idx]) {
			oldBit2 = (HBITMAP)SelectObject(mem2dc, P1_NIDDLE_OFF);
			BitBlt(mem1dc, Player_RobbyX[Client_Idx] + 6, Player_RobbyY[Client_Idx] + 159, 33, 26, mem2dc, 0, 0, SRCCOPY);
		}
	}

	if (GameState == INGAME)
	{
		if (bSceneChange && SelectMap1)
		{
			CSoundMgr::GetInstance()->PlayBGM(L"BGM_Map_2_0.ogg");
			bSceneChange = false;
		}
		else if (bSceneChange && SelectMap2)
		{
			CSoundMgr::GetInstance()->PlayBGM(L"BGM_Map_2_1.ogg");
			bSceneChange = false;
		}

		oldBit2 = (HBITMAP)SelectObject(mem2dc, BGBit_InGame);
		BitBlt(mem1dc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, mem2dc, 0, 0, SRCCOPY);


		// 처음 타일 O,X 그리기 (나중에 삭제)
		for (int i = 0; i < Tile_CountY; i++)
			for (int j = 0; j < Tile_CountX; j++) {
				if (Tile_Enable_Move[Sel_Map][i][j] && !isBox[Sel_Map][i][j])
					SelectObject(mem2dc, Tile_Enable);
				else
					SelectObject(mem2dc, Tile_Disable);
				BitBlt(mem1dc, Tile[i][j].left, Tile[i][j].top, Tile_CX, Tile_CY, mem2dc, 0, 0, SRCCOPY);
				SelectObject(mem2dc, oldBit2);
			}
		/*for (int i = 0; i < Tile_CountY; i++)
		for (int j = 0; j < Tile_CountX; j++) {

		}*/

		// 기본 맵 그리기

		// 맵 1
		if (SelectMap1) {
			for (int i = 0; i < Tile_CountY; i++)
				for (int j = 0; j < Tile_CountX; j++) {
					if (Tile_Enable_Move[0][i][j]) {
						SelectObject(mem2dc, TileBit);
						if (j != 6 && j != 7 && j != 8)
							xPos_Tile = cnt++ % 2;
						else if (i == 2 || i == 10)
							xPos_Tile = 6;
						else if (j == 7)
							xPos_Tile = 3;
						else if (i == 0 || i == 12)
							xPos_Tile = 4;
						else
							xPos_Tile = 2;
						// 밑바탕 타일 설정
						StretchBlt(mem1dc, Tile[i][j].left, Tile[i][j].top, Tile_CX, Tile_CY, mem2dc, xPos_Tile * Tile_CX, 0, Tile_CX, Tile_CY, SRCCOPY);
						//각종 아이템.
						if (Itemset[Sel_Map][i][j] == Ball)
						{
							oldBit2 = (HBITMAP)SelectObject(mem2dc, Items);
							TransparentBlt(mem1dc, Tile[i][j].left, Tile[i][j].top, 40, 40, mem2dc, 0, 0, 40, 40, RGB(0, 255, 0));
						}
						if (Itemset[Sel_Map][i][j] == OnePower)
						{
							oldBit2 = (HBITMAP)SelectObject(mem2dc, Items);
							TransparentBlt(mem1dc, Tile[i][j].left, Tile[i][j].top, 40, 40, mem2dc, 40, 0, 40, 40, RGB(0, 255, 0));

						}
						if (Itemset[Sel_Map][i][j] == Speed)
						{
							oldBit2 = (HBITMAP)SelectObject(mem2dc, Items);
							TransparentBlt(mem1dc, Tile[i][j].left, Tile[i][j].top, 40, 40, mem2dc, 80, 0, 40, 40, RGB(0, 255, 0));
						}
						if (Itemset[Sel_Map][i][j] == MaxPower)
						{
							oldBit2 = (HBITMAP)SelectObject(mem2dc, Items);
							TransparentBlt(mem1dc, Tile[i][j].left, Tile[i][j].top, 40, 40, mem2dc, 120, 0, 40, 40, RGB(0, 255, 0));
						}
						if (Itemset[Sel_Map][i][j] == RedDevil)
						{
							oldBit2 = (HBITMAP)SelectObject(mem2dc, Items);
							TransparentBlt(mem1dc, Tile[i][j].left, Tile[i][j].top, 40, 40, mem2dc, 160, 0, 40, 40, RGB(0, 255, 0));
						}
						// 타일 위 박스 설정
						if (isBox[0][i][j]) {
							if (MoveBox[i][j]) {
								SelectObject(mem2dc, Box_Bit0);
								TransparentBlt(mem1dc, Box[i][j].left, Box[i][j].top, Box_CX, Box_CY, mem2dc, 0, 0, Box_CX, Box_CY, TPColor);
							}
							else {
								if (isBox1[i][j])
									SelectObject(mem2dc, Box_Bit1);
								else
									SelectObject(mem2dc, Box_Bit2);
								TransparentBlt(mem1dc, Box[i][j].left, Box[i][j].top, Box_CX, Box_CY, mem2dc, 0, 0, Box_CX, Box_CY, TPColor);
							}
						}

					}
					// 고정된 나무, 집 설정
					else {
						if (isTree[i][j]) {
							SelectObject(mem2dc, TileBit);
							StretchBlt(mem1dc, Tile[i][j].left, Tile[i][j].top, Tile_CX, Tile_CY, mem2dc, 0, 0, Tile_CX, Tile_CY, SRCCOPY);
							SelectObject(mem2dc, TreeBit);
							TransparentBlt(mem1dc, Tile[i][j].left, Tile[i][j].bottom - Tree_CY, Tree_CX, Tree_CY, mem2dc, 0, 0, Tree_CX, Tree_CY, TPColor);
						}
						else {
							if (isHouse0[i][j])
								SelectObject(mem2dc, House_Bit0);
							else if (isHouse1[i][j])
								SelectObject(mem2dc, House_Bit1);
							else
								SelectObject(mem2dc, House_Bit2);
							TransparentBlt(mem1dc, Tile[i][j].left, Tile[i][j].bottom - House_CY, House_CX, House_CY, mem2dc, 0, 0, House_CX, House_CY, TPColor);
						}
					}
				}
		}
		// 맵2
		else {
			for (int i = 0; i < Tile_CountY; i++)
				for (int j = 0; j < Tile_CountX; j++) {
					oldBit2 = (HBITMAP)SelectObject(mem2dc, Tile2);

					if ((i == 7 || i == 10) && (j == 2 || j == 12))
						xPos_Tile = 2;
					else if ((6 <= i && i <= 11) && (1 <= j && j <= 13))
						xPos_Tile = 3;
					else
						xPos_Tile = (i + j) % 2;
					TransparentBlt(mem1dc, Tile[i][j].left, Tile[i][j].top, Tile_CX, Tile_CY, mem2dc, xPos_Tile * Tile_CX, 0, Tile_CX, Tile_CY, TPColor);

					if (Tile_Enable_Move[1][i][j]) {
						if (isBox[1][i][j]) {
							SelectObject(mem2dc, Block2);
							TransparentBlt(mem1dc, Tile[i][j].left, Tile[i][j].bottom - Box_CY, Box_CX, Box_CY, mem2dc, 0, 0, Box_CX, Box_CY, TPColor);
						}
					}
					else {
						if (isSteel[i][j]) {
							oldBit2 = (HBITMAP)SelectObject(mem2dc, Steel2);
							TransparentBlt(mem1dc, Tile[i][j].left, Tile[i][j].bottom - Tree_CY, Tree_CX, Tree_CY, mem2dc, 0, 0, Tree_CX, Tree_CY, TPColor);
						}
						else if (isStone[i][j]) {
							oldBit2 = (HBITMAP)SelectObject(mem2dc, Stone2);
							TransparentBlt(mem1dc, Tile[i][j].left, Tile[i][j].bottom - House_CY, House_CX, House_CY, mem2dc, 0, 0, House_CX, House_CY, TPColor);
						}
					}

					//각종 아이템.
					if (Itemset[Sel_Map][i][j] == Ball)
					{
						oldBit2 = (HBITMAP)SelectObject(mem2dc, Items);
						TransparentBlt(mem1dc, Tile[i][j].left, Tile[i][j].top, 40, 40, mem2dc, 0, 0, 40, 40, RGB(0, 255, 0));
					}
					if (Itemset[Sel_Map][i][j] == OnePower)
					{
						oldBit2 = (HBITMAP)SelectObject(mem2dc, Items);
						TransparentBlt(mem1dc, Tile[i][j].left, Tile[i][j].top, 40, 40, mem2dc, 40, 0, 40, 40, RGB(0, 255, 0));

					}
					if (Itemset[Sel_Map][i][j] == Speed)
					{
						oldBit2 = (HBITMAP)SelectObject(mem2dc, Items);
						TransparentBlt(mem1dc, Tile[i][j].left, Tile[i][j].top, 40, 40, mem2dc, 80, 0, 40, 40, RGB(0, 255, 0));
					}
					if (Itemset[Sel_Map][i][j] == MaxPower)
					{
						oldBit2 = (HBITMAP)SelectObject(mem2dc, Items);
						TransparentBlt(mem1dc, Tile[i][j].left, Tile[i][j].top, 40, 40, mem2dc, 120, 0, 40, 40, RGB(0, 255, 0));
					}
					if (Itemset[Sel_Map][i][j] == RedDevil)
					{
						oldBit2 = (HBITMAP)SelectObject(mem2dc, Items);
						TransparentBlt(mem1dc, Tile[i][j].left, Tile[i][j].top, 40, 40, mem2dc, 160, 0, 40, 40, RGB(0, 255, 0));
					}
				}
		}

		// 캐릭터 그리기
		/////P1 배찌 P2 다오
		for (int i = 0; i < nPlayer; ++i) {
			if (Player_Live[i]) {
				if (!Player_Remove[i]) {
					if (!bInBubble[i] && !bDie[i] && !bEscape[i])
					{
						BubbleResource[i] = 1;
						BubbleCount[i] = 0;
						Player_tSpeed[i] = Player_Speed[i];

						SelectObject(mem2dc, Player_Bit[i]);
						TransparentBlt(mem1dc, Player[i].left - xGap_Char, Player[i].bottom - Char_CY, Char_CX, Char_CY, mem2dc, xPos_Player[i] * Char_CX, yPos_Player[i] * Char_CY, Char_CX, Char_CY, TPColor);

					}
					////물풍선에 갇힌 상태
					else if (bInBubble[i])
					{
						SelectObject(mem2dc, Player_Bit[i]);
						TransparentBlt(mem1dc, Player[i].left - xGap_Char, Player[i].bottom - Char_CY, Char_CX, Char_CY, mem2dc, Char_CX * BubbleResource[i], 280, Char_CX, Char_CY, TPColor);
					}
					//죽음
					else if (bDie[i])
					{
						SelectObject(mem2dc, Player_Bit[i]);
						TransparentBlt(mem1dc, Player[i].left - xGap_Char, Player[i].bottom - Char_CY, Char_CX, Char_CY, mem2dc, Char_CX * Player_Dying[i], 420, Char_CX, Char_CY, TPColor);
					}
				}
			}
		}
		//if (P1_Live) {
		//	if (!P1_Remove) {
		//		if (!P1_InBubble && !P1_Die && !P1_Escape)
		//		{
		//			P1_BubbleResource = 1;
		//			P1_BubbleCount = 0;
		//			P1_tSpeed = P1_Speed;
		//			SelectObject(mem2dc, P1_Bit);
		//			TransparentBlt(mem1dc, Player1.left - xGap_Char, Player1.bottom - Char_CY, Char_CX, Char_CY, mem2dc, xPos_P1 * Char_CX, yPos_P1 * Char_CY, Char_CX, Char_CY, TPColor);
		//		}
		//		////물풍선에 갇힌 상태
		//		else if (P1_InBubble)
		//		{
		//			SelectObject(mem2dc, P1_Bit);
		//			TransparentBlt(mem1dc, Player1.left - xGap_Char, Player1.bottom - Char_CY, Char_CX, Char_CY, mem2dc, Char_CX * P1_BubbleResource, 280, Char_CX, Char_CY, TPColor);
		//		}
		//		//죽음
		//		else if (P1_Die)
		//		{
		//			SelectObject(mem2dc, P1_Bit);
		//			TransparentBlt(mem1dc, Player1.left - xGap_Char, Player1.bottom - Char_CY, Char_CX, Char_CY, mem2dc, Char_CX * P1_Dying, 420, Char_CX, Char_CY, TPColor);
		//		}
		//	}
		//}
		//if (P2_Live) {
		//	if (!P2_Remove) {
		//		if (!P2_InBubble && !P2_Die && !P2_Escape)
		//		{
		//			P2_BubbleResource = 1;
		//			P2_BubbleCount = 0;
		//			P2_tSpeed = P2_Speed;
		//			SelectObject(mem2dc, P2_Bit);
		//			TransparentBlt(mem1dc, Player2.left - xGap_Char, Player2.bottom - Char_CY, Char_CX, Char_CY, mem2dc, xPos_P2 * Char_CX, yPos_P2 * Char_CY, Char_CX, Char_CY, TPColor);
		//		}
		//		////물풍선에 갇힌 상태
		//		else if (P2_InBubble)
		//		{
		//			SelectObject(mem2dc, P2_Bit);
		//			TransparentBlt(mem1dc, Player2.left - xGap_Char, Player2.bottom - Char_CY, Char_CX, Char_CY, mem2dc, Char_CX * P2_BubbleResource, 280, Char_CX, Char_CY, TPColor);
		//		}
		//		//죽음
		//		else if (P2_Die)
		//		{
		//			SelectObject(mem2dc, P2_Bit);
		//			TransparentBlt(mem1dc, Player2.left - xGap_Char, Player2.bottom - Char_CY, Char_CX, Char_CY, mem2dc, Char_CX * P2_Dying, 420, Char_CX, Char_CY, TPColor);
		//		}
		//	}
		//}
		//// 물풍선 - 터지기 전
		SelectObject(mem2dc, Bubble);

		/*for (int i = 0; i < 7; i++) {
			if (P1_Bubble[i] && !P1_Bubble_Flow[i])
				TransparentBlt(mem1dc, Tile_Bubble1[i].left, Tile_Bubble1[i].top, 40, 40, mem2dc, Bubble_CX * xPos_Bubble, 0, 40, 40, RGB(0, 255, 0));
			if (P2_Bubble[i] && !P2_Bubble_Flow[i])
				TransparentBlt(mem1dc, Tile_Bubble2[i].left, Tile_Bubble2[i].top, 40, 40, mem2dc, Bubble_CX * xPos_Bubble, 0, 40, 40, RGB(0, 255, 0));
		}*/


		for (int j = 0; j < nPlayer; ++j) {
			for (int i = 0; i < 7; i++) {
				if (Player_Bubble[j][i] && !Player_Bubble_Flow[j][i])
					TransparentBlt(mem1dc, Tile_Bubble[j][i].left, Tile_Bubble[j][i].top, 40, 40, mem2dc, Bubble_CX * xPos_Bubble, 0, 40, 40, RGB(0, 255, 0));
			}
		}

		// 물풍선 - 터질 때
		SelectObject(mem2dc, Bubble_Bomb);
		for(int k=0;k<nPlayer;++k)
			for (int i = 0; i < 7; ++i)
			{
				if (Player_Bubble_Flow[k][i])
				{
					if (bEffect[k][i])
					{
						CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Bubble_Off.ogg");
						bEffect[k][i] = FALSE;
					}

					SelectObject(mem2dc, Bubble_Bomb);
					int a, b;
					for (a = 0; a < Tile_CountY; a++)
						if (Tile_Bubble[k][i].bottom == Tile[a][0].bottom)
							break;
					for (b = 0; b < Tile_CountX; b++)
						if (Tile_Bubble[k][i].right == Tile[0][b].right)
							break;


					for (int j = 0; j <= Power[k]; ++j) {   // 위

						if (Collision(Player[k], (Tile_Bubble[k][i].left + Tile_Bubble[k][i].right) / 2, Tile_Bubble[k][i].top - 40 * j) ||
							Collision(Player[k], (Tile_Bubble[k][i].left + Tile_Bubble[k][i].right) / 2, Tile_Bubble[k][i].top - 40 * j + 20)) {
							Player_Speed[k] = 100;
							SetTimer(hwnd, In_Bubble, Player_Speed[k], (TIMERPROC)TimeProc_InBubble);
							bInBubble[k] = TRUE;
						}

						if (j == Power[k] || !Tile_Enable_Move[Sel_Map][a - j - 1][b] || Tile_Bubble[k][i].top - 40 * j == Tile[0][0].top || isBox[Sel_Map][a - j][b]) {
							TransparentBlt(mem1dc, Tile_Bubble[k][i].left, Tile_Bubble[k][i].top - 40 * j, 40, 40, mem2dc, 40 * MoveResource[k][i], 40, 40, 40, RGB(0, 255, 0));
							if (isBox[Sel_Map][a - j][b])
								Box_Break[a - j][b] = TRUE;
							break;
						}
						TransparentBlt(mem1dc, Tile_Bubble[k][i].left, Tile_Bubble[k][i].top - (40 * j), 40, 40, mem2dc, 40 * MoveResource[k][i], 200, 40, 40, RGB(0, 255, 0));
					}
					for (int j = 0; j <= Power[k]; ++j) {   // 아래
						if (Collision(Player[k], (Tile_Bubble[k][i].left + Tile_Bubble[k][i].right) / 2, Tile_Bubble[k][i].bottom + 40 * j) ||
							Collision(Player[k], (Tile_Bubble[k][i].left + Tile_Bubble[k][i].right) / 2, Tile_Bubble[k][i].bottom + 40 * j - 20)) {
							Player_Speed[k] = 100;
							SetTimer(hwnd, In_Bubble, Player_Speed[k], (TIMERPROC)TimeProc_InBubble);
							bInBubble[k] = TRUE;
						}


						if (j == Power[k] || !Tile_Enable_Move[Sel_Map][a + j + 1][b] || Tile_Bubble[k][i].bottom + 40 * j == Tile[12][14].bottom || isBox[Sel_Map][a + j][b]) {
							TransparentBlt(mem1dc, Tile_Bubble[k][i].left, Tile_Bubble[k][i].top + 40 * j, 40, 40, mem2dc, 40 * MoveResource[k][i], 80, 40, 40, RGB(0, 255, 0));
							if (isBox[Sel_Map][a + j][b])
								Box_Break[a + j][b] = TRUE;
							break;
						}
						TransparentBlt(mem1dc, Tile_Bubble[k][i].left, Tile_Bubble[k][i].top + (40 * j), 40, 40, mem2dc, 40 * MoveResource[k][i], 240, 40, 40, RGB(0, 255, 0));
					}
					for (int j = 0; j <= Power[k]; ++j) {   // 오른쪽
						if (Collision(Player[k], Tile_Bubble[k][i].right + 40 * j, (Tile_Bubble[k][i].top + Tile_Bubble[k][i].bottom) / 2) ||
							Collision(Player[k], Tile_Bubble[k][i].right + 40 * j - 20, (Tile_Bubble[k][i].top + Tile_Bubble[k][i].bottom) / 2)) {
							Player_Speed[k] = 100;
							SetTimer(hwnd, In_Bubble, Player_Speed[k], (TIMERPROC)TimeProc_InBubble);
							bInBubble[k] = TRUE;
						}

						if (j == Power[k] || !Tile_Enable_Move[Sel_Map][a][b + j + 1] || Tile_Bubble[k][i].right + 40 * j == Tile[12][14].right || isBox[Sel_Map][a][b + j]) {
							TransparentBlt(mem1dc, Tile_Bubble[k][i].left + 40 * j, Tile_Bubble[k][i].top, 40, 40, mem2dc, 40 * MoveResource[k][i], 120, 40, 40, RGB(0, 255, 0));
							if (isBox[Sel_Map][a][b + j])
								Box_Break[a][b + j] = TRUE;
							break;
						}
						TransparentBlt(mem1dc, Tile_Bubble[k][i].left + (40 * j), Tile_Bubble[k][i].top, 40, 40, mem2dc, 40 * MoveResource[k][i], 280, 40, 40, RGB(0, 255, 0));
					}
					for (int j = 0; j <= Power[k]; ++j) {   // 왼쪽
						if (Collision(Player[k], Tile_Bubble[k][i].left - 40 * j, (Tile_Bubble[k][i].top + Tile_Bubble[k][i].bottom) / 2) ||
							Collision(Player[k], Tile_Bubble[k][i].left - 40 * j + 20, (Tile_Bubble[k][i].top + Tile_Bubble[k][i].bottom) / 2)) {
							Player_Speed[k] = 100;
							SetTimer(hwnd, In_Bubble, Player_Speed[k], (TIMERPROC)TimeProc_InBubble);
							bInBubble[k] = TRUE;
						}

						if (j == Power[k] || !Tile_Enable_Move[Sel_Map][a][b - j - 1] || Tile_Bubble[k][i].left - 40 * j == Tile[0][0].left || isBox[Sel_Map][a][b - j]) {
							TransparentBlt(mem1dc, Tile_Bubble[k][i].left - 40 * j, Tile_Bubble[k][i].top, 40, 40, mem2dc, 40 * MoveResource[k][i], 160, 40, 40, RGB(0, 255, 0));
							if (isBox[Sel_Map][a][b - j])
								Box_Break[a][b - j] = TRUE;
							break;
						}
						TransparentBlt(mem1dc, Tile_Bubble[k][i].left - (40 * j), Tile_Bubble[k][i].top, 40, 40, mem2dc, 40 * MoveResource[k][i], 320, 40, 40, RGB(0, 255, 0));
					}
					TransparentBlt(mem1dc, Tile_Bubble[k][i].left, Tile_Bubble[k][i].top, 40, 40, mem2dc, 40 * MoveResource[k][i], 0, 40, 40, RGB(0, 255, 0));   // 중앙

					if (InBubble_Collision(Player[k], Tile_Bubble[k][i].top, Tile_Bubble[k][i].left, Tile_Bubble[k][i].bottom, Tile_Bubble[k][i].right))
					{
						SetTimer(hwnd, In_Bubble, Player_Speed[k], (TIMERPROC)TimeProc_InBubble);
						Player_Speed[k] = 100;
						SelectObject(mem2dc, Player_Bit[k]);
						TransparentBlt(mem1dc, Player[k].left - xGap_Char, Player[k].bottom - Char_CY, Char_CX, Char_CY, mem2dc, 0, 280, Char_CX, Char_CY, TPColor);
					}



					////////////////////////////물줄기에 닿으면 죽음 ㅠ
				}



			}



		// 아이템
		for (int i = 0; i < Tile_CountY; i++)
			for (int j = 0; j < Tile_CountX; j++) {
				if (Itemset[Sel_Map][i][j] != 0 && Itemset[Sel_Map][i][j] != 7) {
					for (int k = 0; k < nPlayer; ++k) {
						if (IntersectRect(&Temp, &Player[k], &Tile[i][j])) {
							if (Itemset[Sel_Map][i][j] == Ball) {
								CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Item_Off.ogg");
								if (Player_bCount[k] < 7)
									Player_bCount[k]++;
							}
							if (Itemset[Sel_Map][i][j] == OnePower) {
								CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Item_Off.ogg");
								if (Power[k] < 7)
									Power[k]++;
							}
							if (Itemset[Sel_Map][i][j] == Speed) {
								CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Item_Off.ogg");
								KillTimer(hwnd, k);
								if (Player_Speed[k] >= 20)
									Player_Speed[k] -= 5;
								if(k==Client_Idx)
									SetTimer(hwnd, k, Player_Speed[k], (TIMERPROC)TimeProc_P1_Move);
							}

							if (Itemset[Sel_Map][i][j] == MaxPower) {
								CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Item_Off.ogg");
								Power[k] = 7;
							}
							if (Itemset[Sel_Map][i][j] == RedDevil) {
								CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Item_Off.ogg");
								KillTimer(hwnd, k);
								Player_Speed[k] = 15;
								if (k == Client_Idx)
									SetTimer(hwnd, k, Player_Speed[k], (TIMERPROC)TimeProc_P1_Move);
								Player_bCount[k] = 7;
								Power[k] = 7;
							}
							Itemset[Sel_Map][i][j] = 0;
						}

					}

					/*if (IntersectRect(&Temp, &Player1, &Tile[i][j])) {
						if (Itemset[Sel_Map][i][j] == Ball) {
							CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Item_Off.ogg");
							if (P1_bCount < 7)
								P1_bCount++;
						}
						if (Itemset[Sel_Map][i][j] == OnePower) {
							CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Item_Off.ogg");
							if (P1_Power < 7)
								P1_Power++;
						}
						if (Itemset[Sel_Map][i][j] == Speed) {
							CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Item_Off.ogg");
							KillTimer(hwnd, P1);
							if (P1_Speed >= 20)
								P1_Speed -= 5;
							SetTimer(hwnd, P1, P1_Speed, (TIMERPROC)TimeProc_P1_Move);
						}

						if (Itemset[Sel_Map][i][j] == MaxPower) {
							CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Item_Off.ogg");
							P1_Power = 7;
						}
						if (Itemset[Sel_Map][i][j] == RedDevil) {
							CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Item_Off.ogg");
							KillTimer(hwnd, P1);
							P1_Speed = 15;
							SetTimer(hwnd, P1, P1_Speed, (TIMERPROC)TimeProc_P1_Move);
							P1_bCount = 7;
							P1_Power = 7;
						}
						Itemset[Sel_Map][i][j] = 0;
					}*/
				}
			}
		if (P1_N) {
			SelectObject(mem2dc, P1_nOn);
			BitBlt(mem1dc, 60, 563, 44, 37, mem2dc, 0, 0, SRCCOPY);
		}


		if (Collision(ePos, M_X, M_Y))
		{
			SelectObject(mem2dc, Exit);
			BitBlt(mem1dc, 645, 560, 140, 32, mem2dc, 0, 0, SRCCOPY);
		}
		else
		{
			SelectObject(mem2dc, Exit);
			BitBlt(mem1dc, 645, 560, 140, 32, mem2dc, 140, 0, SRCCOPY);
		}

		if (TextOn)
		{
			SelectObject(mem2dc, Texture);
			TransparentBlt(mem1dc, 150, 250, 405, 62, mem2dc, 0, 0, 405, 62, RGB(255, 0, 255));
		}


		if (bDie[0] && bDie[1] && bDie[2] && bDie[3] && Ending) {
			SelectObject(mem2dc, Texture);
			TransparentBlt(mem1dc, 150, 250, 405, 62, mem2dc, 0, 62 * 5, 405, 62, RGB(255, 0, 255));
			SetTimer(hwnd, 8, 100, (TIMERPROC)TimeProc_Text);
		}




	}

	SelectObject(mem1dc, oldBit1);
	SelectObject(mem2dc, oldBit2);
	DeleteDC(mem1dc);
	DeleteDC(mem2dc);
}

// 사각형 점 충돌체크
BOOL Collision(RECT rect, int x, int y)
{
	if (rect.left < x && x < rect.right && rect.top < y && y < rect.bottom)
		return TRUE;
	return FALSE;
}

//Player랑 물풍선체크.
BOOL InBubble_Collision(RECT rt, int t, int l, int b, int r)
{
	if (t <= (rt.top + rt.bottom) / 2 && (rt.top + rt.bottom) / 2 <= b && l <= (rt.right + rt.left) / 2 && (rt.right + rt.left) / 2 <= r)
		return TRUE;
	return FALSE;
}

// 물풍선 연쇄작용
void ChainBomb(RECT Bubble, int power)
{
	RECT rc;
	RECT Check[5];	// 0: 왼쪽 사각형 / 1: 위쪽 사각형 / 2: 오른쪽 사각형 / 3: 아래쪽 사각형
	Check[4] = Bubble;

	Check[0] = { Bubble.left - power * Tile_CX,Bubble.top,Bubble.left,Bubble.bottom };
	Check[1] = { Bubble.left,Bubble.top - power * Tile_CY,Bubble.right,Bubble.top };
	Check[2] = { Bubble.right,Bubble.top,Bubble.right + power * Tile_CX,Bubble.bottom };
	Check[3] = { Bubble.left,Bubble.bottom,Bubble.right,Bubble.bottom + power * Tile_CY };


	int a, b;
	for (a = 0; a < Tile_CountY; a++)
		if (Bubble.bottom == Tile[a][0].bottom)
			break;
	for (b = 0; b < Tile_CountX; b++)
		if (Bubble.right == Tile[0][b].right)
			break;

	for (int j = 0; j <= power; ++j) {   // 위
		if (j == power || !Tile_Enable_Move[Sel_Map][a - j - 1][b] || Bubble.top - 40 * j == Tile[0][0].top || isBox[Sel_Map][a - j][b]) {
			Check[1].top = Bubble.top - 40 * j;
			break;
		}
	}
	for (int j = 0; j <= power; ++j) {   // 아래
		if (j == power || !Tile_Enable_Move[Sel_Map][a + j + 1][b] || Bubble.bottom + 40 * j == Tile[12][14].bottom || isBox[Sel_Map][a + j][b]) {
			Check[3].bottom = Bubble.bottom + 40 * j;
			break;
		}
	}
	for (int j = 0; j <= power; ++j) {   // 오른쪽
		if (j == power || !Tile_Enable_Move[Sel_Map][a][b + j + 1] || Bubble.right + 40 * j == Tile[12][14].right || isBox[Sel_Map][a][b + j]) {
			Check[2].right = Bubble.right + 40 * j;
			break;
		}
	}
	for (int j = 0; j <= power; ++j) {   // 왼쪽
		if (j == power || !Tile_Enable_Move[Sel_Map][a][b - j - 1] || Bubble.left - 40 * j == Tile[0][0].left || isBox[Sel_Map][a][b - j]) {
			Check[0].left = Bubble.left - 40 * j;
			break;
		}
	}

	for (int i = 0; i < Tile_CountY; i++)
		for (int j = 0; j < Tile_CountX; j++) {
			if (Itemset[Sel_Map][i][j] != 0 && Itemset[Sel_Map][i][j] != 7 && !isBox[Sel_Map][i][j]) {
				for (int k = 0; k < 5; k++)
					if (IntersectRect(&rc, &Check[k], &Tile[i][j]))
						Itemset[Sel_Map][i][j] = 0;
			}
		}

	for (int k = 0; k < nPlayer; ++k)
		for (int i = 0; i < 7; i++)
		{
			if (Player_Bubble[k][i] && !Player_Bubble_Boom[k][i]) {
				for (int j = 0; j < 4; j++)
					if (IntersectRect(&rc, &Tile_Bubble[k][i], &Check[j])) {
						Player_Bubble_Boom[k][i] = TRUE;
						Bubble_cnt[k][i] = 0;
						ChainBomb(Tile_Bubble[k][i], Power[k]);
					}
			}
		}
}

// 타이머 관련 함수
void CALLBACK TimeProc_Bubble_BfBoom(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	if (GameState == INGAME) {
		++xPos_Bubble %= 4;
		for (int i = 0; i < nPlayer; ++i)
			if (Player_Move[i])
				++xPos_Player[i] %= 4;

		int sum = 0;
		for (int i = 0; i < nPlayer; ++i) {
			sum += bDie[i];
		}
		if (sum == nPlayer && !Ending) {
			Ending = TRUE;
			CSoundMgr::GetInstance()->PlayEffectSound2(L"SFX_Word_Lose.ogg");
		}

		for (int j = 0; j < nPlayer; ++j) {
			for (int i = 0; i < 7; i++) {
				if (Player_Bubble[j][i] && !Player_Bubble_Boom[j][i]) {
					if (++Bubble_cnt[j][i] == 30) {
						Player_Bubble_Boom[j][i] = TRUE;
						ChainBomb(Tile_Bubble[j][i], Power[j]);
						Bubble_cnt[j][i] = 0;
						/*	SetTimer(hWnd, 2, 3, NULL);*/
					}
				}
				//if (P2_Bubble[i] && !P2_Bubble_Boom[i]) {
				//	if (++P2_Bubble_cnt[i] == 30) {
				//		P2_Bubble_Boom[i] = TRUE;
				//		ChainBomb(Tile_Bubble2[i], P2_Power);
				//		P2_Bubble_cnt[i] = 0;
				//		/*	SetTimer(hWnd, 2, 3, NULL);*/
				//	}
				//}
			}
		}

		for (int j = 0; j < nPlayer; ++j) {
			for (int i = 0; i < 7; i++) {
				if (Player_Bubble_Boom[j][i]) {
					Player_Bubble[j][i] = FALSE;
					Player_Bubble_Boom[j][i] = FALSE;
					Player_Bubble_Flow[j][i] = TRUE;

					SetTimer(hwnd, Bubble_Flow, 100, (TIMERPROC)TimeProc_Bubble_Flow);
				}
				/*if (P2_Bubble_Boom[i]) {
					P2_Bubble[i] = FALSE;
					P2_Bubble_Boom[i] = FALSE;
					P2_Bubble_Flow[i] = TRUE;

					SetTimer(hwnd, Bubble_Flow, 100, (TIMERPROC)TimeProc_Bubble_Flow);
				}*/
			}
		}
	}
	InvalidateRect(hWnd, NULL, FALSE);
}

void CALLBACK TimeProc_Bubble_Flow(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	for (int j = 0; j < nPlayer; ++j) {
		for (int i = 0; i < 7; ++i)
		{
			if (Player_Bubble_Flow[j][i] && ++MoveResource[j][i] == 4) {
				Tile_Bubble[j][i] = { 0,0,0,0 };
				MoveResource[j][i] = 0;
				Player_Bubble_Flow[j][i] = FALSE;
				for (int a = 0; a < Tile_CountY; a++)
					for (int b = 0; b < Tile_CountX; b++)
						if (Box_Break[a][b])
							isBox[Sel_Map][a][b] = FALSE;
			}
			/*if (P2_Bubble_Flow[i] && ++P2_MoveResource[i] == 4) {
				Tile_Bubble2[i] = { 0,0,0,0 };
				P2_Bubble_Flow[i] = FALSE;
				P2_MoveResource[i] = 0;
				for (int a = 0; a < Tile_CountY; a++)
					for (int b = 0; b < Tile_CountX; b++)
						if (Box_Break[a][b])
							isBox[Sel_Map][a][b] = FALSE;
			}*/
		}
	}
	InvalidateRect(hWnd, NULL, FALSE);
}

void CALLBACK TimeProc_P1_Move(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	RECT tmpRECT = Player[Client_Idx];

	switch (yPos_Player[Client_Idx]) {
	case LEFT:
		if (tmpRECT.left >= StartX + 10) {
			tmpRECT.left -= 5;
			tmpRECT.right -= 5;
			for (int i = 0; i < Tile_CountY; i++)
				for (int j = 0; j < Tile_CountX; j++)
					if ((Collision(Tile[i][j], tmpRECT.left, tmpRECT.top) || Collision(Tile[i][j], tmpRECT.left, tmpRECT.bottom)) && (isBox[Sel_Map][i][j] || !Tile_Enable_Move[Sel_Map][i][j])) {
						tmpRECT.left = Tile[i][j].right;
						tmpRECT.right = tmpRECT.left + Player_CX;
						// 블럭 충돌체크 부드럽게
						if (Tile[i][j].bottom < (tmpRECT.top + tmpRECT.bottom) / 2 && Tile_Enable_Move[Sel_Map][i + 1][j] && !isBox[Sel_Map][i + 1][j]) {
							tmpRECT.top = Tile[i + 1][j].top;
							tmpRECT.bottom = tmpRECT.top + Player_CY;
						}
						if ((tmpRECT.top + tmpRECT.bottom) / 2 < Tile[i][j].top && Tile_Enable_Move[Sel_Map][i - 1][j] && !isBox[Sel_Map][i - 1][j]) {
							tmpRECT.bottom = Tile[i - 1][j].bottom;
							tmpRECT.top = tmpRECT.bottom - Player_CY;
						}
					}
		}
		else if (tmpRECT.left <= StartX + 10) {
			tmpRECT.left = StartX;
			tmpRECT.right = tmpRECT.left + Player_CX;
		}
		// 물풍선 체크
		for (int i = 0; i < nPlayer; i++)
		{
			for (int j = 0; j < Player_bCount[i]; j++)
			{
				if(Player_Bubble[i][j] && Collision(Tile_Bubble[i][j], tmpRECT.left, (tmpRECT.top + tmpRECT.bottom) / 2) &&
					Tile_Bubble[i][j].right - 6 <= tmpRECT.left && tmpRECT.left <= Tile_Bubble[i][j].right + 6) {
					tmpRECT.left = Tile_Bubble[i][j].right;
					tmpRECT.right = tmpRECT.left + Player_CX;
				}
			}
		}
		break;
	case RIGHT:
		if (tmpRECT.right <= Tile[12][14].right - 10) {
			tmpRECT.left += 5;
			tmpRECT.right += 5;
			for (int i = 0; i < Tile_CountY; i++)
				for (int j = 0; j < Tile_CountX; j++)
					if ((Collision(Tile[i][j], tmpRECT.right, tmpRECT.top) || Collision(Tile[i][j], tmpRECT.right, tmpRECT.bottom)) && (isBox[Sel_Map][i][j] || !Tile_Enable_Move[Sel_Map][i][j])) {
						tmpRECT.right = Tile[i][j].left;
						tmpRECT.left = tmpRECT.right - Player_CX;
						// 블럭 충돌체크 부드럽게
						if (Tile[i][j].bottom < (tmpRECT.top + tmpRECT.bottom) / 2 && Tile_Enable_Move[Sel_Map][i + 1][j] && !isBox[Sel_Map][i + 1][j]) {
							tmpRECT.top = Tile[i + 1][j].top;
							tmpRECT.bottom = tmpRECT.top + Player_CY;
						}
						if ((tmpRECT.top + tmpRECT.bottom) / 2 < Tile[i][j].top && Tile_Enable_Move[Sel_Map][i - 1][j] && !isBox[Sel_Map][i - 1][j]) {
							tmpRECT.bottom = Tile[i - 1][j].bottom;
							tmpRECT.top = tmpRECT.bottom - Player_CY;
						}
					}
		}
		else if (tmpRECT.right >= Tile[12][14].right - 10) {
			tmpRECT.right = Tile[12][14].right;
			tmpRECT.left = tmpRECT.right - Player_CX;
		}
		// 물풍선 체크
		for (int i = 0; i < nPlayer; i++)
		{
			for (int j = 0; j < Player_bCount[i]; j++)
			{
				if (Player_Bubble[i][j] && Collision(Tile_Bubble[i][j], tmpRECT.right, (tmpRECT.top + tmpRECT.bottom) / 2) &&
					Tile_Bubble[i][j].left - 6 <= tmpRECT.right && tmpRECT.right <= Tile_Bubble[i][j].left + 6) {
					tmpRECT.right = Tile_Bubble[i][j].left;
					tmpRECT.left = tmpRECT.right - Player_CX;
				}
			}
		}
		break;
	case UP:
		if (tmpRECT.top >= StartY + 10) {
			tmpRECT.top -= 5;
			tmpRECT.bottom -= 5;
			for (int i = 0; i < Tile_CountY; i++)
				for (int j = 0; j < Tile_CountX; j++)
					if ((Collision(Tile[i][j], tmpRECT.left, tmpRECT.top) || Collision(Tile[i][j], tmpRECT.right, tmpRECT.top)) && (isBox[Sel_Map][i][j] || !Tile_Enable_Move[Sel_Map][i][j])) {
						tmpRECT.top = Tile[i][j].bottom;
						tmpRECT.bottom = tmpRECT.top + Player_CY;
						// 블럭 충돌체크 부드럽게
						if (Tile[i][j].right < (tmpRECT.left + tmpRECT.right) / 2 && Tile_Enable_Move[Sel_Map][i][j + 1] && !isBox[Sel_Map][i][j + 1]) {
							tmpRECT.left = Tile[i][j + 1].left;
							tmpRECT.right = tmpRECT.left + Player_CX;
						}
						if ((tmpRECT.left + tmpRECT.right) / 2 < Tile[i][j].left && Tile_Enable_Move[Sel_Map][i][j - 1] && !isBox[Sel_Map][i][j - 1]) {
							tmpRECT.right = Tile[i][j - 1].right;
							tmpRECT.left = tmpRECT.right - Player_CX;
						}
					}
		}
		else if (tmpRECT.top <= StartY + 5) {
			tmpRECT.top = StartY;
			tmpRECT.bottom = tmpRECT.top + Player_CY;
		}
		// 물풍선 체크
		for (int i = 0; i < nPlayer; i++)
		{
			for (int j = 0; j < Player_bCount[i]; j++)
			{
				if (Player_Bubble[i][j] && Collision(Tile_Bubble[i][i], (tmpRECT.left + tmpRECT.right) / 2, tmpRECT.top) &&
					Tile_Bubble[i][j].bottom - 6 <= tmpRECT.top && tmpRECT.top <= Tile_Bubble[i][j].bottom + 6) {
					tmpRECT.top = Tile_Bubble[i][j].bottom;
					tmpRECT.bottom = tmpRECT.top + Player_CY;
				}
			}
		}
		break;
	case DOWN:
		if (tmpRECT.bottom <= Tile[12][14].bottom - 10) {
			tmpRECT.top += 5;
			tmpRECT.bottom += 5;
			for (int i = 0; i < Tile_CountY; i++)
				for (int j = 0; j < Tile_CountX; j++)
					if ((Collision(Tile[i][j], tmpRECT.left, tmpRECT.bottom) || Collision(Tile[i][j], tmpRECT.right, tmpRECT.bottom)) && (isBox[Sel_Map][i][j] || !Tile_Enable_Move[Sel_Map][i][j])) {
						tmpRECT.bottom = Tile[i][j].top;
						tmpRECT.top = tmpRECT.bottom - Player_CY;
						// 블럭 충돌체크 부드럽게
						if (Tile[i][j].right < (tmpRECT.left + tmpRECT.right) / 2 && Tile_Enable_Move[Sel_Map][i][j + 1] && !isBox[Sel_Map][i][j + 1]) {
							tmpRECT.left = Tile[i][j + 1].left;
							tmpRECT.right = tmpRECT.left + Player_CX;
						}
						if ((tmpRECT.left + tmpRECT.right) / 2 < Tile[i][j].left && Tile_Enable_Move[Sel_Map][i][j - 1] && !isBox[Sel_Map][i][j - 1]) {
							tmpRECT.right = Tile[i][j - 1].right;
							tmpRECT.left = tmpRECT.right - Player_CX;
						}
					}
		}
		else if (tmpRECT.bottom >= Tile[12][14].bottom - 10) {
			tmpRECT.bottom = Tile[12][14].bottom;
			tmpRECT.top = tmpRECT.bottom - Player_CY;
		}
		// 물풍선 체크
		for (int i = 0; i < nPlayer; i++)
		{
			for (int j = 0; j < Player_bCount[i]; j++)
			{
				if (Player_Bubble[i][j] && Collision(Tile_Bubble[i][j], (tmpRECT.left + tmpRECT.right) / 2, tmpRECT.bottom) &&
					Tile_Bubble[i][j].top - 6 <= tmpRECT.bottom && tmpRECT.bottom <= Tile_Bubble[i][j].top + 6) {
					tmpRECT.bottom = Tile_Bubble[i][j].top;
					tmpRECT.top = tmpRECT.bottom - Player_CY;
				}
			}
		}
		break;
	}
	// 플레이어 이동 패킷 생성 - 상태 아직 안보냄
	
	//if (tmpRECT.left != Player[Client_Idx].left || tmpRECT.top != Player[Client_Idx].top) {
	WaitForSingleObject(hSendEvent, INFINITE);
	Send_Client_Packet = new InputPacket(Client_Idx, tmpRECT.left, tmpRECT.top, yPos_Player[Client_Idx]);
	Send_Client_Packet->type = player;
	SetEvent(hInputEvent);
	//}
	InvalidateRect(hWnd, NULL, FALSE);
}




void CALLBACK TimeProc_InBubble(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	for (int i = 0; i < nPlayer; ++i) {
		if (bInBubble[i])
		{
			if (BubbleCount[i] >= 30 && BubbleCount[i] < 33)
			{
				BubbleResource[i] = 2;
				BubbleCount[i]++;
			}
			else if (BubbleCount[i] == 33)
			{
				BubbleResource[i] = 3;
				BubbleCount[i] = 0;

				if (!bInBubble[0] && !bInBubble[1] && !bInBubble[2] && !bInBubble[3])
					KillTimer(hwnd, In_Bubble);
				bInBubble[i] = FALSE;
				bDie[i] = TRUE;
				CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Character_Die.ogg");
				SetTimer(hwnd, Die, 150, (TIMERPROC)TimeProc_Die);
			}
			else
			{
				BubbleResource[i] = (++BubbleResource[i] % 2);
				BubbleCount[i]++;
			}
		}
	}

	/*if (P1_InBubble)
	{
		if (P1_BubbleCount >= 30 && P1_BubbleCount < 33)
		{
			P1_BubbleResource = 2;
			P1_BubbleCount++;
		}
		else if (P1_BubbleCount == 33)
		{
			P1_BubbleResource = 3;
			P1_BubbleCount = 0;
			if (!P2_InBubble)
				KillTimer(hwnd, In_Bubble);
			P1_InBubble = FALSE;
			P1_Die = TRUE;
			CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Character_Die.ogg");
			SetTimer(hwnd, Die, 150, (TIMERPROC)TimeProc_Die);
		}
		else
		{
			P1_BubbleResource = (++P1_BubbleResource % 2);
			P1_BubbleCount++;
		}
	}

	if (P2_InBubble)
	{
		if (P2_BubbleCount >= 30 && P2_BubbleCount < 33)
		{
			P2_BubbleResource = 2;
			P2_BubbleCount++;
		}
		else if (P2_BubbleCount == 33)
		{
			P2_BubbleResource = 3;
			P2_BubbleCount = 0;
			if (!P1_InBubble)
				KillTimer(hwnd, In_Bubble);
			P2_InBubble = FALSE;
			P2_Die = TRUE;
			CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Character_Die.ogg");
			SetTimer(hwnd, Die, 150, (TIMERPROC)TimeProc_Die);
		}
		else
		{
			++P2_BubbleResource %= 2;
			P2_BubbleCount++;
		}
	}*/
	InvalidateRect(hWnd, NULL, FALSE);
}

void CALLBACK TimeProc_Die(HWND hWnd, UINT uMsg, UINT ideEvent, DWORD dwTime)
{
	static int cnt[4] = { 0, };

	if (bDie[0] && bDie[1] && bDie[2] && bDie[3])
		KillTimer(hWnd, Die);

	for (int i = 0; i < nPlayer; ++i) {
		if (bDie[i])
		{
			//if (Player_Dying[i] == 0)
			//	CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Character_Die.ogg");
			Player_Dying[i]++;
			if (Player_Dying[i] == 6) {
				Player_Dying[i] = 5 + cnt[i] % 2;
				cnt[i]++;
				if (cnt[i] == 3)
					Player_Remove[i] = TRUE;
			}
		}
	}

	/*if (P1_Die)
	{
		if (P1_Dying == 0)
			CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Character_Die.ogg");
		P1_Dying++;
		if (P1_Dying == 6) {
			P1_Dying = 5 + cnt1 % 2;
			cnt1++;
			if (cnt1 == 3)
				P1_Remove = TRUE;
		}
	}
	if (P2_Die)
	{
		if (P2_Dying == 0)
			CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Character_Die.ogg");
		P2_Dying++;
		if (P2_Dying == 6) {
			P2_Dying = 5 + cnt2 % 2;
			cnt2++;
			if (cnt2 == 3)
				P2_Remove = TRUE;
		}
	}*/
	InvalidateRect(hWnd, NULL, false);
}



void CALLBACK TimeProc_Text(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	static int Counting = 1;
	if (TextOn)
	{
		if (++Counting % 66)
		{
			TextOn = FALSE;
			Counting = 0;
		}
	}

	if (Ending)
	{
		if (++Counting % 20)
		{
			Ending = FALSE;
			SetPos();
			for (int i = 0; i < nPlayer; ++i) {
				bInBubble[i] = false;
				bDie[i] = false;
			}
			GameState = ROBBY;

			Counting = 0;
			bSceneChange = true;
		}
	}
}

// 키보드 관련 함수
void KEY_DOWN_P1(HWND hWnd)
{
	if (GameState == INGAME)
	{
		if (!bDie[Client_Idx] && Player_Live[Client_Idx]) {
			if (GetAsyncKeyState(VK_CONTROL) & 0x8000 && bInBubble[Client_Idx] && P1_N) {
				SetEvent(hSendEvent);
				//P1_Speed = P1_tSpeed;
				//P1_InBubble = FALSE;
				//CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Character_Revival.ogg");
			}

			if (!Player_Move[Client_Idx])
			{
				if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
					yPos_Player[Client_Idx] = DOWN;
					SetTimer(hwnd, P1, Player_Speed[Client_Idx], (TIMERPROC)TimeProc_P1_Move);
					Player_Move[Client_Idx] = TRUE;
				}
				if (GetAsyncKeyState(VK_UP) & 0x8000) {
					yPos_Player[Client_Idx] = UP;
					SetTimer(hwnd, P1, Player_Speed[Client_Idx], (TIMERPROC)TimeProc_P1_Move);
					Player_Move[Client_Idx] = TRUE;
				}
				if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
					yPos_Player[Client_Idx] = LEFT;
					SetTimer(hwnd, P1, Player_Speed[Client_Idx], (TIMERPROC)TimeProc_P1_Move);
					Player_Move[Client_Idx] = TRUE;
				}
				if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
					yPos_Player[Client_Idx] = RIGHT;
					SetTimer(hwnd, P1, Player_Speed[Client_Idx], (TIMERPROC)TimeProc_P1_Move);
					Player_Move[Client_Idx] = TRUE;
				}
			}
			if (GetAsyncKeyState(VK_SPACE) & 0x8000 && !bInBubble[Client_Idx]) {
				for (int i = 0; i < Player_bCount[Client_Idx]; ++i)
				{
					if (!Player_Bubble[Client_Idx][i] && !Player_Bubble_Flow[Client_Idx][i])
					{
						for (int a = 0; a < 7; ++a)
							bEffect[Client_Idx][a] = TRUE;

						for (int j = 0; j < P1_bCount; ++j) 
						{
							if (Collision(Tile_Bubble[Client_Idx][j], (Player[Client_Idx].right + Player[Client_Idx].left) / 2, (Player[Client_Idx].top + Player[Client_Idx].bottom) / 2) || Collision(Tile_Bubble2[j], (Player[Client_Idx].right + Player[Client_Idx].left) / 2, (Player[Client_Idx].top + Player[Client_Idx].bottom) / 2)) {
								return;
							}
						}
						for (int a = 0; a < Tile_CountY; a++)
							for (int b = 0; b < Tile_CountX; b++)
							{
								if (Collision(Tile[a][b], (Player[Client_Idx].right + Player[Client_Idx].left) / 2, (Player[Client_Idx].top + Player[Client_Idx].bottom) / 2)) {
									Tile_Bubble[Client_Idx][i] = Tile[a][b];
									// 물풍선 패킷 생성
									WaitForSingleObject(hSendEvent, INFINITE);
									if (!Send_Client_Packet) {
										Send_Client_Packet = new InputPacket(Client_Idx, Tile[a][b].left, Tile[a][b].top);
										Send_Client_Packet->type = PacketType::bubble;
									}
									SetEvent(hInputEvent);
									return;
								}
							}
					}
				}
			}
		}
	}
}
void KEY_UP_P1(WPARAM wParam, HWND hWnd)
{
	if ((wParam == VK_DOWN && yPos_Player[Client_Idx] == DOWN) ||
		(wParam == VK_UP && yPos_Player[Client_Idx] == UP) ||
		(wParam == VK_LEFT && yPos_Player[Client_Idx] == LEFT) ||
		(wParam == VK_RIGHT && yPos_Player[Client_Idx] == RIGHT)) {
		KillTimer(hwnd, P1);
		xPos_Player[Client_Idx] = 0;
		Player_Move[Client_Idx] = FALSE;
		WaitForSingleObject(hSendEvent, INFINITE);
		Send_Client_Packet = new InputPacket(Client_Idx, Player[Client_Idx].left, Player[Client_Idx].top, STOP);
		Send_Client_Packet->type = player;
		SetEvent(hInputEvent);
	}
	switch (wParam) {
	case VK_F1:
		Helper = false;
		break;/*
	case VK_DOWN:
		if (yPos_Player[Client_Idx] != DOWN)break;
	case VK_UP:
		if (yPos_Player[Client_Idx] != UP) break;
	case VK_LEFT:
		if (yPos_Player[Client_Idx] != LEFT)break;
	case VK_RIGHT:
		if (yPos_Player[Client_Idx] != RIGHT)break;
		KillTimer(hwnd, P1);
		xPos_Player[Client_Idx] = 0;
		Player_Move[Client_Idx] = FALSE;
		break;*/
	}
}

/*
1P = 이동(방향키), 물폭탄( 스페이스 )
2P = 이동(wasd) , 물폭탄( shift )
*/

void SetBitmap()
{
	BGBit_InGame = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_INGAME_BG));
	Tile_Enable = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_TILE_ENABLE));
	Tile_Disable = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_TILE_DISABLE));
	TileBit = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_TILE));
	Box_Bit0 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_BOX0_M1));
	Box_Bit1 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_BOX1_M1));
	Box_Bit2 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_BOX2_M1));
	House_Bit0 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_HOUSE0));
	House_Bit1 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_HOUSE1));
	House_Bit2 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_HOUSE2));
	TreeBit = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_TREE));
	P1_Bit = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_C1));
	P2_Bit = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_C2));
	
	Player_Bit[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_C1));
	Player_Bit[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_C2));
	Player_Bit[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_C3));
	Player_Bit[3] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_C4));

	Bubble = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_Bubble));
	Bubble_Bomb = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_BubbleBomb));
	LogoMenu = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_BGLOGO));
	LogoStart = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_LOGO));
	LOBBY = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_LOBBy));
	VIL = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_VIL));
	BOSS = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_BOSSMAP));
	MAP1 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_MAP1));
	MAP2 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_MAP2));

	P1_NIDDLE_ON = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_NIDDLE));


	P1_NIDDLE_OFF = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_OFFNID));


	Lobby_Start = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_START));
	P1_On = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_READY));
	P2_On = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_READY));
	Player_On = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_READY));
	Items = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_Item));

	Tile2 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_Tile2));
	Block2 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_BLOCK));
	Steel2 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_STEEL));
	Stone2 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_STONE));

	Mon1Bit = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_MON1));
	Mon2Bit = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_MON2));

	P1_nOn = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_P1_NIDDLE));

	Help = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_Help));

	Exit = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_EXIT));

	Texture = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_TEXT));
}

void SetPos()
{
	// 맵 좌표 설정
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

	srand((unsigned)time(NULL));
	// 맵1 아이템
	for (int i = 0; i < Tile_CountY; i++)
		for (int j = 0; j < Tile_CountX; j++) {
			ItemValue = rand() % 30;
			if (ItemValue != 0 && ItemValue != 7 && isBox[0][i][j]) {
				Itemset[0][i][j] = ItemValue;
			}
		}

	// 맵2 아이템
	for (int i = 0; i < Tile_CountY; i++)
		for (int j = 0; j < Tile_CountX; j++) {
			if (i == 8 || i == 9) {
				if (j == 5 || j == 6)
					Itemset[1][i][j] = Speed;
				else if (j == 7 || j == 8)
					Itemset[1][i][j] = Ball;
				else if (j == 9)
					Itemset[1][i][j] = MaxPower;
			}
		}


	// 플레이어 좌표 설정
	//Player1 = Tile[0][0];
	//Player1.right = Player1.left + Player_CX;
	//Player1.top = Player1.bottom - Player_CY;

	Player[0] = Tile[0][0];
	Player[0].right = Player[0].left + Player_CX;
	Player[0].bottom = Player[0].top + Player_CY;

	Player[1] = Tile[12][1];
	Player[1].right = Player[1].left + Player_CX;
	Player[1].bottom = Player[1].top + Player_CY;

	Player[2] = Tile[1][13];
	Player[2].right = Player[2].left + Player_CX;
	Player[2].bottom = Player[2].top + Player_CY;

	Player[3] = Tile[13][13];
	Player[3].right = Player[3].left + Player_CX;
	Player[3].bottom = Player[3].top + Player_CY;


	//GAMESTATE==In LOBBY 일때 좌표 설정.
	GameMap1.left = 630;
	GameMap1.top = 340;
	GameMap1.right = GameMap1.left + 135;
	GameMap1.bottom = GameMap1.top + 21;
	GameMap2.left = 630;
	GameMap2.top = 355;
	GameMap2.right = GameMap2.left + 135;
	GameMap2.bottom = GameMap2.top + 21;
	GameStart.left = 500;
	GameStart.top = 487;
	GameStart.right = GameStart.left + BG_X / 2;
	GameStart.bottom = GameStart.top + BG_Y;
	GameLogo.left = 305;
	GameLogo.top = 430;
	GameLogo.right = 305 + BG_X / 2;
	GameLogo.bottom = 430 + BG_Y;
	ePos.left = 645;
	ePos.top = 560;
	ePos.right = ePos.left + 140;
	ePos.bottom = ePos.top + 32;

	// 플레이어 설정 세팅
	P1_Name.left = 58;
	P1_Name.top = 85;
	P1_Name.right = 171;
	P1_Name.bottom = 108;

	P2_Name.left = 246;
	P2_Name.top = 92;
	P2_Name.right = 371;
	P2_Name.bottom = 108;
	P1_NIDDLE.left = 44, P1_NIDDLE.top = 243, P1_NIDDLE.right = P1_NIDDLE.left + 33, P1_NIDDLE.bottom = P1_NIDDLE.top + 26;

	// 플레이어 로비 위치
	/*Player_Robby[0].left = 38;
	Player_Robby[0].top = 114;
	Player_Robby[0].right = 38+158;
	Player_Robby[0].bottom = 114+188;

	Player_Robby[1].left = 227;
	Player_Robby[1].top = 114;
	Player_Robby[1].right = 227 + 158;
	Player_Robby[1].bottom = 114 + 188;*/

	for (int i = 0; i < MAX_PLAYER; ++i) {
		Player_RobbyX[i] = 38 + 189 * i;
		Player_RobbyY[i] = 84;
	}
}

