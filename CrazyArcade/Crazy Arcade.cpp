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
void CALLBACK TimeProc_P2_Move(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime);
void CALLBACK TimeProc_InBubble(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime);
void CALLBACK TimeProc_Die(HWND hWnd, UINT uMsg, UINT ideEvent, DWORD dwTime);
void CALLBACK TimeProc_Monster_Move(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime);
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
HANDLE hRecvEvent, hSendEvent, hConnectEvent, hBubbleEvent, hPlayerEvent;
// 소켓
SOCKET sock;
// 불 값
BOOL Bubble_Arrive = false, Player_Arrive = false;
// 패킷
PlayerPacket *Recv_Player_Packet;
BubblePacket *Recv_Bubble_Packet;
enum ClientPacket Send_Client_Packet;


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
enum Item { Ball = 1, OnePower = 6, Speed = 11, MaxPower = 16, RedDevil = 21, Pint = 26 };
enum Timer { Bubble_BfBoom, Bubble_Flow, P1, P2, In_Bubble, Die, Monster_Move };

int GameState = MENU;
int ItemValue;
int Itemset[2][13][15];

HDC hdc;
HBITMAP hBit;         // 여기에 그려서 memdc -> hdc 에 넣음
HBITMAP P1_Bit, P2_Bit, BGBit_InGame;
HBITMAP TileBit, Tile_Enable, Tile_Disable;
HBITMAP Box_Bit0, Box_Bit1, Box_Bit2;
HBITMAP House_Bit0, House_Bit1, House_Bit2, TreeBit;
HBITMAP Mon1Bit, Mon2Bit;
HBITMAP Bubble, Bubble_Bomb;
HBITMAP LogoMenu, LogoStart;
HBITMAP LOBBY, VIL, BOSS, MAP1, MAP2;
HBITMAP P1_NIDDLE_ON, P1_PIN_ON, P2_NIDDLE_ON, P2_PIN_ON;
HBITMAP P1_NIDDLE_OFF, P1_PIN_OFF, P2_NIDDLE_OFF, P2_PIN_OFF;
HBITMAP Lobby_Start;
HBITMAP P1_On, P2_On;
HBITMAP P1_nOn, P2_nOn;
HBITMAP Items;
HBITMAP Tile2, Steel2, Stone2, Block2;
RECT Crect;
RECT Player1, Player2;
RECT Monster1, Monster2;
RECT Tile[13][15], Box[13][15];
RECT Tile_Bubble1[7], Tile_Bubble2[7];	//P1의 물풍선 좌표저장, P2의 물풍선 좌표저장
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

// 몬스터
int xPos_Mon1, yPos_Mon1;								// (0: 아래, 1: 왼쪽, 2: 오른쪽, 3: 위쪽, 4: 죽음, 5: 사라짐)
int xPos_Mon2, yPos_Mon2;
bool Mon1_Live, Mon2_Live;



int xPos_Tile;
int xPos_Bubble;
int P1_Bubble_cnt[7], P2_Bubble_cnt[7];

BOOL P1_Bubble[7];   //최대 폭탄을 놓을 수 있는 갯수는 7개
BOOL P2_Bubble[7];
BOOL P1_Bubble_Boom[7];
BOOL P2_Bubble_Boom[7];


BOOL P1_Bubble_Flow[7];   //폭탄이 터질때를 알려주는 BOOL값
int P1_MoveResource[7];   //물줄기 리소스 40만큼 이동하기
int P1_Power;         //물줄기 파워(현재 WM_CREATE에 정의해놓고 사용중
BOOL P1_InBubble;		//물방울 안에 갇혔을 때
BOOL P1_Escape;			//탈출했을 때
BOOL P1_Die;			//죽었을때
int P1_BubbleResource;	//죽었을때 리소스 반복
int P1_BubbleCount;		//죽었을때 리소스 반복

BOOL P2_Bubble_Flow[7];   //폭탄이 터질때를 알려주는 BOOL값
int P2_MoveResource[7];   //물줄기 리소스 40만큼 이동하기
int P2_Power;         //물줄기 파워(현재 WM_CREATE에 정의해놓고 사용중
BOOL P2_InBubble;
BOOL P2_Escape;
BOOL P2_Die;
int P2_BubbleResource;	//죽었을때 리소스 반복
int P2_BubbleCount;		//죽었을때 리소스 반복

BOOL P1_Move, P2_Move;

BOOL Box_Break[13][15];
int Box_cnt[13][15];

int M_X, M_Y; //마우스 x,y좌표
int P1_bCount, P2_bCount;

RECT GameLogo;									//게임로고와 마우스 좌표 충돌체크를 위해서 선언.
RECT GameStart, GameMap1, GameMap2;				//게임시작버튼, 게임맵 고르는 버튼.
BOOL SelectMap1, SelectMap2;					//맵선택 
RECT P1_NIDDLE, P1_PIN, P2_NIDDLE, P2_PIN;		//바늘과 핀의 좌표값
BOOL P1_N, P1_P, P2_N, P2_P;					//바늘과 핀의 작동여부
RECT P1_Name, P2_Name;
BOOL P1_Live = TRUE, P2_Live = TRUE;
int P1_Speed, P2_Speed;
int P1_tSpeed, P2_tSpeed;
int P1_Dying, P2_Dying;
BOOL P1_Remove, P2_Remove;

int Sel_Map;


static bool bSceneChange = true;
static bool bEffect[7] = { true,true,true,true,true,true,true };
static bool bEffect2[7] = { true,true,true,true,true,true,true };
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
		SetTimer(hwnd, Monster_Move, 50, (TIMERPROC)TimeProc_Monster_Move);
		P1_Speed = 35;
		P2_Speed = 35;
		P1_bCount = 1;
		P2_bCount = 1;
		P1_Power = 1;
		P2_Power = 1;

		Mon1_Live = TRUE;
		Mon2_Live = TRUE;

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
				Send_Client_Packet = input_left;
				SetEvent(hSendEvent);
				CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Button_Off.ogg");
				CSoundMgr::GetInstance()->PlayEffectSound2(L"SFX_Word_Start.ogg");
				TextOn = TRUE;
				SetTimer(hWnd, 8, 750, (TIMERPROC)TimeProc_Text);
				GameState = INGAME;
				bSceneChange = true;
				if (SelectMap1)
					Sel_Map = 0;
				else
					Sel_Map = 1;
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
			if (Collision(P2_NIDDLE, M_X, M_Y))
			{
				CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Button_Off.ogg");
				if (P2_N)
					P2_N = FALSE;
				else
					P2_N = TRUE;
			}

			if (Collision(P1_Name, M_X, M_Y)) {
				CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Button_Off.ogg");
				if (P1_Live)
					P1_Live = FALSE;
				else
					P1_Live = TRUE;
			}
			if (Collision(P2_Name, M_X, M_Y)) {
				CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Button_Off.ogg");
				if (P2_Live)
					P2_Live = FALSE;
				else
					P2_Live = TRUE;
			}
		}
		if (GameState == INGAME)
		{
			if (Collision(ePos, M_X, M_Y)) {
				SetPos();
				P1_InBubble = false;
				P1_Die = false;
				P2_InBubble = false;
				P2_Die = false;
				bSceneChange = true;
				CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Button_Off.ogg");
				GameState = ROBBY;
			}


		}
		return 0;

	case WM_CHAR:
		/* 물줄기 세기 조절*/
		if (wParam == '+') {
			if (P1_bCount < 7)
				++P1_bCount;
			if (P2_bCount < 7)
				++P2_bCount;
		}
		if (wParam == '-') {
			if (P1_bCount > 1)
				--P1_bCount;
			if (P2_bCount < 7)
				++P2_bCount;
		}
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
		KEY_DOWN_P2(hWnd);

		InvalidateRect(hWnd, NULL, FALSE);
		break;

	case WM_KEYUP:
		KEY_UP_P1(wParam, hWnd);
		KEY_UP_P2(wParam, hWnd);
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

		if (P1_Live == FALSE)
		{
			oldBit2 = (HBITMAP)SelectObject(mem2dc, P1_On);
			BitBlt(mem1dc, 38, 114, 158, 188, mem2dc, 0, 0, SRCCOPY);
		}
		if (P2_Live == FALSE)
		{
			oldBit2 = (HBITMAP)SelectObject(mem2dc, P1_On);
			BitBlt(mem1dc, 227, 114, 158, 188, mem2dc, 0, 0, SRCCOPY);
		}

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

		if (P1_N && P1_Live) {
			oldBit2 = (HBITMAP)SelectObject(mem2dc, P1_NIDDLE_ON);
			BitBlt(mem1dc, 44, 243, 33, 26, mem2dc, 0, 0, SRCCOPY);
		}
		else if (!P1_N && P1_Live) {
			oldBit2 = (HBITMAP)SelectObject(mem2dc, P1_NIDDLE_OFF);
			BitBlt(mem1dc, 44, 243, 33, 26, mem2dc, 0, 0, SRCCOPY);
		}



		if (P2_N && P2_Live) {
			oldBit2 = (HBITMAP)SelectObject(mem2dc, P2_NIDDLE_ON);
			BitBlt(mem1dc, 232, 243, 33, 26, mem2dc, 0, 0, SRCCOPY);
		}
		else if (!P2_N && P2_Live) {
			oldBit2 = (HBITMAP)SelectObject(mem2dc, P2_NIDDLE_OFF);
			BitBlt(mem1dc, 232, 243, 33, 26, mem2dc, 0, 0, SRCCOPY);
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
		if (P1_Live) {
			if (!P1_Remove) {
				if (!P1_InBubble && !P1_Die && !P1_Escape)
				{
					P1_BubbleResource = 1;
					P1_BubbleCount = 0;
					P1_tSpeed = P1_Speed;
					SelectObject(mem2dc, P1_Bit);
					TransparentBlt(mem1dc, Player1.left - xGap_Char, Player1.bottom - Char_CY, Char_CX, Char_CY, mem2dc, xPos_P1 * Char_CX, yPos_P1 * Char_CY, Char_CX, Char_CY, TPColor);

				}
				////물풍선에 갇힌 상태
				else if (P1_InBubble)
				{
					SelectObject(mem2dc, P1_Bit);
					TransparentBlt(mem1dc, Player1.left - xGap_Char, Player1.bottom - Char_CY, Char_CX, Char_CY, mem2dc, Char_CX * P1_BubbleResource, 280, Char_CX, Char_CY, TPColor);
				}
				//죽음
				else if (P1_Die)
				{
					SelectObject(mem2dc, P1_Bit);
					TransparentBlt(mem1dc, Player1.left - xGap_Char, Player1.bottom - Char_CY, Char_CX, Char_CY, mem2dc, Char_CX * P1_Dying, 420, Char_CX, Char_CY, TPColor);

				}
			}
		}
		if (P2_Live) {
			if (!P2_Remove) {
				if (!P2_InBubble && !P2_Die && !P2_Escape)
				{
					P2_BubbleResource = 1;
					P2_BubbleCount = 0;
					P2_tSpeed = P2_Speed;
					SelectObject(mem2dc, P2_Bit);
					TransparentBlt(mem1dc, Player2.left - xGap_Char, Player2.bottom - Char_CY, Char_CX, Char_CY, mem2dc, xPos_P2 * Char_CX, yPos_P2 * Char_CY, Char_CX, Char_CY, TPColor);

				}
				////물풍선에 갇힌 상태
				else if (P2_InBubble)
				{
					SelectObject(mem2dc, P2_Bit);
					TransparentBlt(mem1dc, Player2.left - xGap_Char, Player2.bottom - Char_CY, Char_CX, Char_CY, mem2dc, Char_CX * P2_BubbleResource, 280, Char_CX, Char_CY, TPColor);
				}
				//죽음
				else if (P2_Die)
				{
					SelectObject(mem2dc, P2_Bit);
					TransparentBlt(mem1dc, Player2.left - xGap_Char, Player2.bottom - Char_CY, Char_CX, Char_CY, mem2dc, Char_CX * P2_Dying, 420, Char_CX, Char_CY, TPColor);

				}
			}
		}
		//// 물풍선 - 터지기 전
		SelectObject(mem2dc, Bubble);

		for (int i = 0; i < 7; i++) {
			if (P1_Bubble[i] && !P1_Bubble_Flow[i])
				TransparentBlt(mem1dc, Tile_Bubble1[i].left, Tile_Bubble1[i].top, 40, 40, mem2dc, Bubble_CX * xPos_Bubble, 0, 40, 40, RGB(0, 255, 0));
			if (P2_Bubble[i] && !P2_Bubble_Flow[i])
				TransparentBlt(mem1dc, Tile_Bubble2[i].left, Tile_Bubble2[i].top, 40, 40, mem2dc, Bubble_CX * xPos_Bubble, 0, 40, 40, RGB(0, 255, 0));
		}

		// 물풍선 - 터질 때
		SelectObject(mem2dc, Bubble_Bomb);
		for (int i = 0; i < 7; ++i)
		{
			if (P1_Bubble_Flow[i])
			{
				if (bEffect[i])
				{
					CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Bubble_Off.ogg");
					bEffect[i] = FALSE;
				}

				SelectObject(mem2dc, Bubble_Bomb);
				int a, b;
				for (a = 0; a < Tile_CountY; a++)
					if (Tile_Bubble1[i].bottom == Tile[a][0].bottom)
						break;
				for (b = 0; b < Tile_CountX; b++)
					if (Tile_Bubble1[i].right == Tile[0][b].right)
						break;


				for (int j = 0; j <= P1_Power; ++j) {   // 위

					if (Collision(Player1, (Tile_Bubble1[i].left + Tile_Bubble1[i].right) / 2, Tile_Bubble1[i].top - 40 * j) ||
						Collision(Player1, (Tile_Bubble1[i].left + Tile_Bubble1[i].right) / 2, Tile_Bubble1[i].top - 40 * j + 20)) {
						P1_Speed = 100;
						SetTimer(hwnd, In_Bubble, P1_Speed, (TIMERPROC)TimeProc_InBubble);
						P1_InBubble = TRUE;
					}
					if (Collision(Monster1, (Tile_Bubble1[i].left + Tile_Bubble1[i].right) / 2, Tile_Bubble1[i].top - 40 * j) ||
						Collision(Monster1, (Tile_Bubble1[i].left + Tile_Bubble1[i].right) / 2, Tile_Bubble1[i].top - 40 * j + 20)) {
						if (mEffect[0]) {
							CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Monster_Basic_Normal_Die.ogg");
							mEffect[0] = false;
							Mon1_Live = FALSE;
							yPos_Mon1 = M_DIE;
						}
					}

					if (Collision(Monster2, (Tile_Bubble1[i].left + Tile_Bubble1[i].right) / 2, Tile_Bubble1[i].top - 40 * j) ||
						Collision(Monster2, (Tile_Bubble1[i].left + Tile_Bubble1[i].right) / 2, Tile_Bubble1[i].top - 40 * j + 20)) {
						if (mEffect[1]) {
							CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Monster_Basic_Normal_Die.ogg");
							mEffect[1] = false;
							Mon2_Live = FALSE;
							yPos_Mon2 = M_DIE;
						}
					}

					if (j == P1_Power || !Tile_Enable_Move[Sel_Map][a - j - 1][b] || Tile_Bubble1[i].top - 40 * j == Tile[0][0].top || isBox[Sel_Map][a - j][b]) {
						TransparentBlt(mem1dc, Tile_Bubble1[i].left, Tile_Bubble1[i].top - 40 * j, 40, 40, mem2dc, 40 * P1_MoveResource[i], 40, 40, 40, RGB(0, 255, 0));
						if (isBox[Sel_Map][a - j][b])
							Box_Break[a - j][b] = TRUE;
						break;
					}
					TransparentBlt(mem1dc, Tile_Bubble1[i].left, Tile_Bubble1[i].top - (40 * j), 40, 40, mem2dc, 40 * P1_MoveResource[i], 200, 40, 40, RGB(0, 255, 0));
				}
				for (int j = 0; j <= P1_Power; ++j) {   // 아래
					if (Collision(Player1, (Tile_Bubble1[i].left + Tile_Bubble1[i].right) / 2, Tile_Bubble1[i].bottom + 40 * j) ||
						Collision(Player1, (Tile_Bubble1[i].left + Tile_Bubble1[i].right) / 2, Tile_Bubble1[i].bottom + 40 * j - 20)) {
						P1_Speed = 100;
						SetTimer(hwnd, In_Bubble, P1_Speed, (TIMERPROC)TimeProc_InBubble);
						P1_InBubble = TRUE;
					}
					if (Collision(Monster1, (Tile_Bubble1[i].left + Tile_Bubble1[i].right) / 2, Tile_Bubble1[i].bottom + 40 * j) ||
						Collision(Monster1, (Tile_Bubble1[i].left + Tile_Bubble1[i].right) / 2, Tile_Bubble1[i].bottom + 40 * j - 20)) {
						if (mEffect[0]) {
							CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Monster_Basic_Normal_Die.ogg");
							mEffect[0] = false;
							Mon1_Live = FALSE;
							yPos_Mon1 = M_DIE;
						}

					}

					if (Collision(Monster2, (Tile_Bubble1[i].left + Tile_Bubble1[i].right) / 2, Tile_Bubble1[i].bottom + 40 * j) ||
						Collision(Monster2, (Tile_Bubble1[i].left + Tile_Bubble1[i].right) / 2, Tile_Bubble1[i].bottom + 40 * j - 20)) {
						if (mEffect[1]) {
							CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Monster_Basic_Normal_Die.ogg");
							mEffect[1] = false;
							Mon2_Live = FALSE;
							yPos_Mon2 = M_DIE;
						}
					}

					if (j == P1_Power || !Tile_Enable_Move[Sel_Map][a + j + 1][b] || Tile_Bubble1[i].bottom + 40 * j == Tile[12][14].bottom || isBox[Sel_Map][a + j][b]) {
						TransparentBlt(mem1dc, Tile_Bubble1[i].left, Tile_Bubble1[i].top + 40 * j, 40, 40, mem2dc, 40 * P1_MoveResource[i], 80, 40, 40, RGB(0, 255, 0));
						if (isBox[Sel_Map][a + j][b])
							Box_Break[a + j][b] = TRUE;
						break;
					}
					TransparentBlt(mem1dc, Tile_Bubble1[i].left, Tile_Bubble1[i].top + (40 * j), 40, 40, mem2dc, 40 * P1_MoveResource[i], 240, 40, 40, RGB(0, 255, 0));
				}
				for (int j = 0; j <= P1_Power; ++j) {   // 오른쪽
					if (Collision(Player1, Tile_Bubble1[i].right + 40 * j, (Tile_Bubble1[i].top + Tile_Bubble1[i].bottom) / 2) ||
						Collision(Player1, Tile_Bubble1[i].right + 40 * j - 20, (Tile_Bubble1[i].top + Tile_Bubble1[i].bottom) / 2)) {
						P1_Speed = 100;
						SetTimer(hwnd, In_Bubble, P1_Speed, (TIMERPROC)TimeProc_InBubble);
						P1_InBubble = TRUE;
					}
					if (Collision(Monster1, Tile_Bubble1[i].right + 40 * j, (Tile_Bubble1[i].top + Tile_Bubble1[i].bottom) / 2) ||
						Collision(Monster1, Tile_Bubble1[i].right + 40 * j - 20, (Tile_Bubble1[i].top + Tile_Bubble1[i].bottom) / 2)) {
						if (mEffect[0]) {
							CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Monster_Basic_Normal_Die.ogg");
							mEffect[0] = false;
							Mon1_Live = FALSE;
							yPos_Mon1 = M_DIE;
						}
					}

					if (Collision(Monster2, Tile_Bubble1[i].right + 40 * j, (Tile_Bubble1[i].top + Tile_Bubble1[i].bottom) / 2) ||
						Collision(Monster2, Tile_Bubble1[i].right + 40 * j - 20, (Tile_Bubble1[i].top + Tile_Bubble1[i].bottom) / 2)) {
						if (mEffect[1]) {
							CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Monster_Basic_Normal_Die.ogg");
							mEffect[1] = false;
							Mon2_Live = FALSE;
							yPos_Mon2 = M_DIE;
						}
					}

					if (j == P1_Power || !Tile_Enable_Move[Sel_Map][a][b + j + 1] || Tile_Bubble1[i].right + 40 * j == Tile[12][14].right || isBox[Sel_Map][a][b + j]) {
						TransparentBlt(mem1dc, Tile_Bubble1[i].left + 40 * j, Tile_Bubble1[i].top, 40, 40, mem2dc, 40 * P1_MoveResource[i], 120, 40, 40, RGB(0, 255, 0));
						if (isBox[Sel_Map][a][b + j])
							Box_Break[a][b + j] = TRUE;
						break;
					}
					TransparentBlt(mem1dc, Tile_Bubble1[i].left + (40 * j), Tile_Bubble1[i].top, 40, 40, mem2dc, 40 * P1_MoveResource[i], 280, 40, 40, RGB(0, 255, 0));
				}
				for (int j = 0; j <= P1_Power; ++j) {   // 왼쪽
					if (Collision(Player1, Tile_Bubble1[i].left - 40 * j, (Tile_Bubble1[i].top + Tile_Bubble1[i].bottom) / 2) ||
						Collision(Player1, Tile_Bubble1[i].left - 40 * j + 20, (Tile_Bubble1[i].top + Tile_Bubble1[i].bottom) / 2)) {
						P1_Speed = 100;
						SetTimer(hwnd, In_Bubble, P1_Speed, (TIMERPROC)TimeProc_InBubble);
						P1_InBubble = TRUE;
					}
					if (Collision(Monster1, Tile_Bubble1[i].left - 40 * j, (Tile_Bubble1[i].top + Tile_Bubble1[i].bottom) / 2) ||
						Collision(Monster1, Tile_Bubble1[i].left - 40 * j + 20, (Tile_Bubble1[i].top + Tile_Bubble1[i].bottom) / 2)) {
						if (mEffect[0]) {
							CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Monster_Basic_Normal_Die.ogg");
							mEffect[0] = false;
							Mon1_Live = FALSE;
							yPos_Mon1 = M_DIE;
						}
					}

					if (Collision(Monster2, Tile_Bubble1[i].left - 40 * j, (Tile_Bubble1[i].top + Tile_Bubble1[i].bottom) / 2) ||
						Collision(Monster2, Tile_Bubble1[i].left - 40 * j + 20, (Tile_Bubble1[i].top + Tile_Bubble1[i].bottom) / 2)) {
						if (mEffect[1]) {
							CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Monster_Basic_Normal_Die.ogg");
							mEffect[1] = false;
							Mon2_Live = FALSE;
							yPos_Mon2 = M_DIE;
						}
					}

					if (j == P1_Power || !Tile_Enable_Move[Sel_Map][a][b - j - 1] || Tile_Bubble1[i].left - 40 * j == Tile[0][0].left || isBox[Sel_Map][a][b - j]) {
						TransparentBlt(mem1dc, Tile_Bubble1[i].left - 40 * j, Tile_Bubble1[i].top, 40, 40, mem2dc, 40 * P1_MoveResource[i], 160, 40, 40, RGB(0, 255, 0));
						if (isBox[Sel_Map][a][b - j])
							Box_Break[a][b - j] = TRUE;
						break;
					}
					TransparentBlt(mem1dc, Tile_Bubble1[i].left - (40 * j), Tile_Bubble1[i].top, 40, 40, mem2dc, 40 * P1_MoveResource[i], 320, 40, 40, RGB(0, 255, 0));
				}
				TransparentBlt(mem1dc, Tile_Bubble1[i].left, Tile_Bubble1[i].top, 40, 40, mem2dc, 40 * P1_MoveResource[i], 0, 40, 40, RGB(0, 255, 0));   // 중앙

				if (InBubble_Collision(Player1, Tile_Bubble1[i].top, Tile_Bubble1[i].left, Tile_Bubble1[i].bottom, Tile_Bubble1[i].right))
				{
					SetTimer(hwnd, In_Bubble, P1_Speed, (TIMERPROC)TimeProc_InBubble);
					P1_Speed = 100;
					SelectObject(mem2dc, P1_Bit);
					TransparentBlt(mem1dc, Player1.left - xGap_Char, Player1.bottom - Char_CY, Char_CX, Char_CY, mem2dc, 0, 280, Char_CX, Char_CY, TPColor);
				}



				////////////////////////////물줄기에 닿으면 죽음 ㅠ
			}

			if (P2_Bubble_Flow[i])
			{
				if (bEffect2[i])
				{
					CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Bubble_Off.ogg");
					bEffect2[i] = FALSE;
				}
				SelectObject(mem2dc, Bubble_Bomb);
				int a, b;
				for (a = 0; a < Tile_CountY; a++)
					if (Tile_Bubble2[i].bottom == Tile[a][0].bottom)
						break;
				for (b = 0; b < Tile_CountX; b++)
					if (Tile_Bubble2[i].right == Tile[0][b].right)
						break;


				for (int j = 0; j <= P2_Power; ++j) {   // 위

					if (Collision(Player2, (Tile_Bubble2[i].left + Tile_Bubble2[i].right) / 2, Tile_Bubble2[i].top - 40 * j) ||
						Collision(Player2, (Tile_Bubble2[i].left + Tile_Bubble2[i].right) / 2, Tile_Bubble2[i].top - 40 * j + 20)) {
						P2_Speed = 100;
						SetTimer(hwnd, In_Bubble, P2_Speed, (TIMERPROC)TimeProc_InBubble);
						P2_InBubble = TRUE;
					}
					if (Collision(Monster1, (Tile_Bubble2[i].left + Tile_Bubble2[i].right) / 2, Tile_Bubble2[i].top - 40 * j) ||
						Collision(Monster1, (Tile_Bubble2[i].left + Tile_Bubble2[i].right) / 2, Tile_Bubble2[i].top - 40 * j + 20)) {
						if (mEffect[0]) {
							CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Monster_Basic_Normal_Die.ogg");
							mEffect[0] = false;
							Mon1_Live = FALSE;
							yPos_Mon1 = M_DIE;
						}
					}

					if (Collision(Monster2, (Tile_Bubble2[i].left + Tile_Bubble2[i].right) / 2, Tile_Bubble2[i].top - 40 * j) ||
						Collision(Monster2, (Tile_Bubble2[i].left + Tile_Bubble2[i].right) / 2, Tile_Bubble2[i].top - 40 * j + 20)) {
						if (mEffect[1]) {
							CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Monster_Basic_Normal_Die.ogg");
							mEffect[1] = false;
							Mon2_Live = FALSE;
							yPos_Mon2 = M_DIE;
						}
					}

					if (j == P2_Power || !Tile_Enable_Move[Sel_Map][a - j - 1][b] || Tile_Bubble2[i].top - 40 * j == Tile[0][0].top || isBox[Sel_Map][a - j][b]) {
						TransparentBlt(mem1dc, Tile_Bubble2[i].left, Tile_Bubble2[i].top - 40 * j, 40, 40, mem2dc, 40 * P2_MoveResource[i], 40, 40, 40, RGB(0, 255, 0));
						if (isBox[Sel_Map][a - j][b])
							Box_Break[a - j][b] = TRUE;
						break;
					}
					TransparentBlt(mem1dc, Tile_Bubble2[i].left, Tile_Bubble2[i].top - (40 * j), 40, 40, mem2dc, 40 * P2_MoveResource[i], 200, 40, 40, RGB(0, 255, 0));
				}
				for (int j = 0; j <= P2_Power; ++j) {   // 아래
					if (Collision(Player2, (Tile_Bubble2[i].left + Tile_Bubble2[i].right) / 2, Tile_Bubble2[i].bottom + 40 * j) ||
						Collision(Player2, (Tile_Bubble2[i].left + Tile_Bubble2[i].right) / 2, Tile_Bubble2[i].bottom + 40 * j - 20)) {
						P2_Speed = 100;
						SetTimer(hwnd, In_Bubble, P2_Speed, (TIMERPROC)TimeProc_InBubble);
						P2_InBubble = TRUE;
					}
					if (Collision(Monster1, (Tile_Bubble2[i].left + Tile_Bubble2[i].right) / 2, Tile_Bubble2[i].bottom + 40 * j) ||
						Collision(Monster1, (Tile_Bubble2[i].left + Tile_Bubble2[i].right) / 2, Tile_Bubble2[i].bottom + 40 * j - 20)) {
						if (mEffect[0]) {
							CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Monster_Basic_Normal_Die.ogg");
							mEffect[0] = false;
							Mon1_Live = FALSE;
							yPos_Mon1 = M_DIE;
						}
					}

					if (Collision(Monster2, (Tile_Bubble2[i].left + Tile_Bubble2[i].right) / 2, Tile_Bubble2[i].bottom + 40 * j) ||
						Collision(Monster2, (Tile_Bubble2[i].left + Tile_Bubble2[i].right) / 2, Tile_Bubble2[i].bottom + 40 * j - 20)) {
						if (mEffect[1]) {
							CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Monster_Basic_Normal_Die.ogg");
							mEffect[1] = false;
							Mon2_Live = FALSE;
							yPos_Mon2 = M_DIE;
						}
					}

					if (j == P2_Power || !Tile_Enable_Move[Sel_Map][a + j + 1][b] || Tile_Bubble2[i].bottom + 40 * j == Tile[12][14].bottom || isBox[Sel_Map][a + j][b]) {
						TransparentBlt(mem1dc, Tile_Bubble2[i].left, Tile_Bubble2[i].top + 40 * j, 40, 40, mem2dc, 40 * P2_MoveResource[i], 80, 40, 40, RGB(0, 255, 0));
						if (isBox[Sel_Map][a + j][b])
							Box_Break[a + j][b] = TRUE;
						break;
					}
					TransparentBlt(mem1dc, Tile_Bubble2[i].left, Tile_Bubble2[i].top + (40 * j), 40, 40, mem2dc, 40 * P2_MoveResource[i], 240, 40, 40, RGB(0, 255, 0));
				}
				for (int j = 0; j <= P2_Power; ++j) {   // 오른쪽
					if (Collision(Player2, Tile_Bubble2[i].right + 40 * j, (Tile_Bubble2[i].top + Tile_Bubble2[i].bottom) / 2) ||
						Collision(Player2, Tile_Bubble2[i].right + 40 * j - 20, (Tile_Bubble2[i].top + Tile_Bubble2[i].bottom) / 2)) {
						P2_Speed = 100;
						SetTimer(hwnd, In_Bubble, P2_Speed, (TIMERPROC)TimeProc_InBubble);
						P2_InBubble = TRUE;
					}
					if (Collision(Monster1, Tile_Bubble2[i].right + 40 * j, (Tile_Bubble2[i].top + Tile_Bubble2[i].bottom) / 2) ||
						Collision(Monster1, Tile_Bubble2[i].right + 40 * j - 20, (Tile_Bubble2[i].top + Tile_Bubble2[i].bottom) / 2)) {
						if (mEffect[0]) {
							CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Monster_Basic_Normal_Die.ogg");
							mEffect[0] = false;
							Mon1_Live = FALSE;
							yPos_Mon1 = M_DIE;
						}
					}

					if (Collision(Monster2, Tile_Bubble2[i].right + 40 * j, (Tile_Bubble2[i].top + Tile_Bubble2[i].bottom) / 2) ||
						Collision(Monster2, Tile_Bubble2[i].right + 40 * j - 20, (Tile_Bubble2[i].top + Tile_Bubble2[i].bottom) / 2)) {
						if (mEffect[1]) {
							CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Monster_Basic_Normal_Die.ogg");
							mEffect[1] = false;
							Mon2_Live = FALSE;
							yPos_Mon2 = M_DIE;
						}
					}

					if (j == P2_Power || !Tile_Enable_Move[Sel_Map][a][b + j + 1] || Tile_Bubble2[i].right + 40 * j == Tile[12][14].right || isBox[Sel_Map][a][b + j]) {
						TransparentBlt(mem1dc, Tile_Bubble2[i].left + 40 * j, Tile_Bubble2[i].top, 40, 40, mem2dc, 40 * P2_MoveResource[i], 120, 40, 40, RGB(0, 255, 0));
						if (isBox[Sel_Map][a][b + j])
							Box_Break[a][b + j] = TRUE;
						break;
					}
					TransparentBlt(mem1dc, Tile_Bubble2[i].left + (40 * j), Tile_Bubble2[i].top, 40, 40, mem2dc, 40 * P2_MoveResource[i], 280, 40, 40, RGB(0, 255, 0));
				}
				for (int j = 0; j <= P2_Power; ++j) {   // 왼쪽
					if (Collision(Player2, Tile_Bubble2[i].left - 40 * j, (Tile_Bubble2[i].top + Tile_Bubble2[i].bottom) / 2) ||
						Collision(Player2, Tile_Bubble2[i].left - 40 * j + 20, (Tile_Bubble2[i].top + Tile_Bubble2[i].bottom) / 2)) {
						P2_Speed = 100;
						SetTimer(hwnd, In_Bubble, P2_Speed, (TIMERPROC)TimeProc_InBubble);
						P2_InBubble = TRUE;
					}
					if (Collision(Monster1, Tile_Bubble2[i].left - 40 * j, (Tile_Bubble2[i].top + Tile_Bubble2[i].bottom) / 2) ||
						Collision(Monster1, Tile_Bubble2[i].left - 40 * j + 20, (Tile_Bubble2[i].top + Tile_Bubble2[i].bottom) / 2)) {
						if (mEffect[0]) {
							CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Monster_Basic_Normal_Die.ogg");
							mEffect[0] = false;
							Mon1_Live = FALSE;
							yPos_Mon1 = M_DIE;
						}
					}

					if (Collision(Monster2, Tile_Bubble2[i].left - 40 * j, (Tile_Bubble2[i].top + Tile_Bubble2[i].bottom) / 2) ||
						Collision(Monster2, Tile_Bubble2[i].left - 40 * j + 20, (Tile_Bubble2[i].top + Tile_Bubble2[i].bottom) / 2)) {
						if (mEffect[1]) {
							CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Monster_Basic_Normal_Die.ogg");
							mEffect[1] = false;
							Mon2_Live = FALSE;
							yPos_Mon2 = M_DIE;
						}
					}

					if (j == P2_Power || !Tile_Enable_Move[Sel_Map][a][b - j - 1] || Tile_Bubble2[i].left - 40 * j == Tile[0][0].left || isBox[Sel_Map][a][b - j]) {
						TransparentBlt(mem1dc, Tile_Bubble2[i].left - 40 * j, Tile_Bubble2[i].top, 40, 40, mem2dc, 40 * P2_MoveResource[i], 160, 40, 40, RGB(0, 255, 0));
						if (isBox[Sel_Map][a][b - j])
							Box_Break[a][b - j] = TRUE;
						break;
					}
					TransparentBlt(mem1dc, Tile_Bubble2[i].left - (40 * j), Tile_Bubble2[i].top, 40, 40, mem2dc, 40 * P2_MoveResource[i], 320, 40, 40, RGB(0, 255, 0));
				}
				TransparentBlt(mem1dc, Tile_Bubble2[i].left, Tile_Bubble2[i].top, 40, 40, mem2dc, 40 * P2_MoveResource[i], 0, 40, 40, RGB(0, 255, 0));   // 중앙

				if (InBubble_Collision(Player2, Tile_Bubble2[i].top, Tile_Bubble2[i].left, Tile_Bubble2[i].bottom, Tile_Bubble2[i].right))
				{
					SetTimer(hwnd, In_Bubble, P2_Speed, (TIMERPROC)TimeProc_InBubble);
					P2_Speed = 100;
					SelectObject(mem2dc, P2_Bit);
					TransparentBlt(mem1dc, Player2.left - xGap_Char, Player2.bottom - Char_CY, Char_CX, Char_CY, mem2dc, 0, 280, Char_CX, Char_CY, TPColor);
				}
			}

		}

		// 몬스터
		if (Mon1_Live) {
			SelectObject(mem2dc, Mon1Bit);
			TransparentBlt(mem1dc, Monster1.left, Monster1.top, Monster1_CX, Monster1_CY, mem2dc, xPos_Mon1 * Monster1_CX, yPos_Mon1 * Monster1_CY, Monster1_CX, Monster1_CY, TPColor);
		}
		else {
			SelectObject(mem2dc, Mon1Bit);
			TransparentBlt(mem1dc, Monster1.left, Monster1.top, Monster1_CX, Monster1_CY, mem2dc, xPos_Mon1 * Monster1_CX, yPos_Mon1 * Monster1_CY, Monster1_CX, Monster1_CY, TPColor);
		}

		if (Mon2_Live) {
			SelectObject(mem2dc, Mon2Bit);
			TransparentBlt(mem1dc, Monster2.left, Monster2.top, Monster2_CX, Monster2_CY, mem2dc, xPos_Mon2 * Monster2_CX, yPos_Mon2 * Monster2_CY, Monster2_CX, Monster2_CY, TPColor);
		}
		else {
			SelectObject(mem2dc, Mon2Bit);
			TransparentBlt(mem1dc, Monster2.left, Monster2.top, Monster2_CX, Monster2_CY, mem2dc, xPos_Mon2 * Monster2_CX, yPos_Mon2 * Monster2_CY, Monster2_CX, Monster2_CY, TPColor);
		}


		// 아이템
		for (int i = 0; i < Tile_CountY; i++)
			for (int j = 0; j < Tile_CountX; j++) {
				if (Itemset[Sel_Map][i][j] != 0 && Itemset[Sel_Map][i][j] != 7 && Itemset[Sel_Map][i][j] != Pint) {
					if (IntersectRect(&Temp, &Player1, &Tile[i][j])) {
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
					}

					if (IntersectRect(&Temp, &Player2, &Tile[i][j])) {
						CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Item_Off.ogg");
						if (Itemset[Sel_Map][i][j] == Ball) {
							if (P2_bCount < 7)
								P2_bCount++;
						}
						if (Itemset[Sel_Map][i][j] == OnePower) {
							CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Item_Off.ogg");
							if (P2_Power < 7)
								P2_Power++;
						}if (Itemset[Sel_Map][i][j] == Speed) {
							CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Item_Off.ogg");
							KillTimer(hwnd, P2);
							if (P2_Speed >= 20)
								P2_Speed -= 5;
							SetTimer(hwnd, P2, P2_Speed, (TIMERPROC)TimeProc_P2_Move);
						}

						if (Itemset[Sel_Map][i][j] == MaxPower) {
							CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Item_Off.ogg");
							P2_Power = 7;
						}

						if (Itemset[Sel_Map][i][j] == RedDevil) {
							CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Item_Off.ogg");
							KillTimer(hwnd, P2);
							P2_Speed = 15;
							SetTimer(hwnd, P2, P2_Speed, (TIMERPROC)TimeProc_P2_Move);
							P2_bCount = 7;
							P2_Power = 7;
						}
						Itemset[Sel_Map][i][j] = 0;
					}
				}
			}
		if (P1_N) {
			SelectObject(mem2dc, P1_nOn);
			BitBlt(mem1dc, 60, 563, 44, 37, mem2dc, 0, 0, SRCCOPY);
		}
		if (P2_N) {
			SelectObject(mem2dc, P2_nOn);
			BitBlt(mem1dc, 360, 563, 44, 37, mem2dc, 0, 0, SRCCOPY);
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


		if (P1_Die && P2_Die && Ending) {
			SelectObject(mem2dc, Texture);
			TransparentBlt(mem1dc, 150, 250, 405, 62, mem2dc, 0, 62 * 5, 405, 62, RGB(255, 0, 255));
			SetTimer(hwnd, 8, 100, (TIMERPROC)TimeProc_Text);
		}

		if (!Mon1_Live && !Mon2_Live && Ending) {
			SelectObject(mem2dc, Texture);
			TransparentBlt(mem1dc, 150, 250, 405, 62, mem2dc, 0, 62 * 2, 405, 62, RGB(255, 0, 255));
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
void ChainBomb(RECT Bubble, int Power)
{
	RECT rc;
	RECT Check[5];	// 0: 왼쪽 사각형 / 1: 위쪽 사각형 / 2: 오른쪽 사각형 / 3: 아래쪽 사각형
	Check[4] = Bubble;

	Check[0] = { Bubble.left - Power * Tile_CX,Bubble.top,Bubble.left,Bubble.bottom };
	Check[1] = { Bubble.left,Bubble.top - Power * Tile_CY,Bubble.right,Bubble.top };
	Check[2] = { Bubble.right,Bubble.top,Bubble.right + Power * Tile_CX,Bubble.bottom };
	Check[3] = { Bubble.left,Bubble.bottom,Bubble.right,Bubble.bottom + Power * Tile_CY };


	int a, b;
	for (a = 0; a < Tile_CountY; a++)
		if (Bubble.bottom == Tile[a][0].bottom)
			break;
	for (b = 0; b < Tile_CountX; b++)
		if (Bubble.right == Tile[0][b].right)
			break;

	for (int j = 0; j <= Power; ++j) {   // 위
		if (j == Power || !Tile_Enable_Move[Sel_Map][a - j - 1][b] || Bubble.top - 40 * j == Tile[0][0].top || isBox[Sel_Map][a - j][b]) {
			Check[1].top = Bubble.top - 40 * j;
			break;
		}
	}
	for (int j = 0; j <= Power; ++j) {   // 아래
		if (j == Power || !Tile_Enable_Move[Sel_Map][a + j + 1][b] || Bubble.bottom + 40 * j == Tile[12][14].bottom || isBox[Sel_Map][a + j][b]) {
			Check[3].bottom = Bubble.bottom + 40 * j;
			break;
		}
	}
	for (int j = 0; j <= Power; ++j) {   // 오른쪽
		if (j == Power || !Tile_Enable_Move[Sel_Map][a][b + j + 1] || Bubble.right + 40 * j == Tile[12][14].right || isBox[Sel_Map][a][b + j]) {
			Check[2].right = Bubble.right + 40 * j;
			break;
		}
	}
	for (int j = 0; j <= Power; ++j) {   // 왼쪽
		if (j == Power || !Tile_Enable_Move[Sel_Map][a][b - j - 1] || Bubble.left - 40 * j == Tile[0][0].left || isBox[Sel_Map][a][b - j]) {
			Check[0].left = Bubble.left - 40 * j;
			break;
		}
	}

	for (int i = 0; i < Tile_CountY; i++)
		for (int j = 0; j < Tile_CountX; j++) {
			if (Itemset[Sel_Map][i][j] != 0 && Itemset[Sel_Map][i][j] != 7 && Itemset[Sel_Map][i][j] != Pint && !isBox[Sel_Map][i][j]) {
				for (int k = 0; k < 5; k++)
					if (IntersectRect(&rc, &Check[k], &Tile[i][j]))
						Itemset[Sel_Map][i][j] = 0;
			}
		}


	for (int i = 0; i < 7; i++)
	{
		if (P1_Bubble[i] && !P1_Bubble_Boom[i]) {
			for (int j = 0; j < 4; j++)
				if (IntersectRect(&rc, &Tile_Bubble1[i], &Check[j])) {
					P1_Bubble_Boom[i] = TRUE;
					P1_Bubble_cnt[i] = 0;
					ChainBomb(Tile_Bubble1[i], P1_Power);
				}
		}
		if (P2_Bubble[i] && !P2_Bubble_Boom[i]) {
			for (int j = 0; j < 4; j++)
				if (IntersectRect(&rc, &Tile_Bubble2[i], &Check[j])) {
					P2_Bubble_Boom[i] = TRUE;
					P2_Bubble_cnt[i] = 0;
					ChainBomb(Tile_Bubble2[i], P2_Power);
				}
		}
	}
}

// 타이머 관련 함수
void CALLBACK TimeProc_Bubble_BfBoom(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	if (P1_Move)
		++xPos_P1 %= 4;
	if (P2_Move)
		++xPos_P2 %= 4;

	++xPos_Bubble %= 4;

	++xPos_Mon1 %= 2;
	++xPos_Mon2 %= 2;

	if (P1_Die && P2_Die && !Ending) {
		Ending = TRUE;
		CSoundMgr::GetInstance()->PlayEffectSound2(L"SFX_Word_Lose.ogg");
	}
	if (!Mon1_Live && !Mon2_Live && !Ending) {
		Ending = TRUE;
		CSoundMgr::GetInstance()->PlayEffectSound2(L"SFX_Word_Win.ogg");
	}
	for (int i = 0; i < 7; i++) {
		if (P1_Bubble[i] && !P1_Bubble_Boom[i]) {
			if (++P1_Bubble_cnt[i] == 30) {
				P1_Bubble_Boom[i] = TRUE;
				ChainBomb(Tile_Bubble1[i], P1_Power);
				P1_Bubble_cnt[i] = 0;
				/*	SetTimer(hWnd, 2, 3, NULL);*/
			}
		}
		if (P2_Bubble[i] && !P2_Bubble_Boom[i]) {
			if (++P2_Bubble_cnt[i] == 30) {
				P2_Bubble_Boom[i] = TRUE;
				ChainBomb(Tile_Bubble2[i], P2_Power);
				P2_Bubble_cnt[i] = 0;
				/*	SetTimer(hWnd, 2, 3, NULL);*/
			}
		}
	}

	for (int i = 0; i < 7; i++) {
		if (P1_Bubble_Boom[i]) {
			P1_Bubble[i] = FALSE;
			P1_Bubble_Boom[i] = FALSE;
			P1_Bubble_Flow[i] = TRUE;

			SetTimer(hwnd, Bubble_Flow, 100, (TIMERPROC)TimeProc_Bubble_Flow);
		}
		if (P2_Bubble_Boom[i]) {
			P2_Bubble[i] = FALSE;
			P2_Bubble_Boom[i] = FALSE;
			P2_Bubble_Flow[i] = TRUE;

			SetTimer(hwnd, Bubble_Flow, 100, (TIMERPROC)TimeProc_Bubble_Flow);
		}
	}
	InvalidateRect(hWnd, NULL, FALSE);
}

void CALLBACK TimeProc_Bubble_Flow(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	for (int i = 0; i < 7; ++i)
	{
		if (P1_Bubble_Flow[i] && ++P1_MoveResource[i] == 4) {
			Tile_Bubble1[i] = { 0,0,0,0 };
			P1_MoveResource[i] = 0;
			P1_Bubble_Flow[i] = FALSE;
			for (int a = 0; a < Tile_CountY; a++)
				for (int b = 0; b < Tile_CountX; b++)
					if (Box_Break[a][b])
						isBox[Sel_Map][a][b] = FALSE;
		}
		if (P2_Bubble_Flow[i] && ++P2_MoveResource[i] == 4) {
			Tile_Bubble2[i] = { 0,0,0,0 };
			P2_Bubble_Flow[i] = FALSE;
			P2_MoveResource[i] = 0;
			for (int a = 0; a < Tile_CountY; a++)
				for (int b = 0; b < Tile_CountX; b++)
					if (Box_Break[a][b])
						isBox[Sel_Map][a][b] = FALSE;
		}
	}
	InvalidateRect(hWnd, NULL, FALSE);
}

void CALLBACK TimeProc_P1_Move(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	WaitForSingleObject(hPlayerEvent, INFINITE);
	Player1.left = Recv_Player_Packet->x;
	Player1.bottom = Player1.left + Player_CX;
	Player1.top = Recv_Player_Packet->y;
	Player1.bottom = Recv_Player_Packet->y + Player_CY;
	ResetEvent(hPlayerEvent);
	//switch (yPos_P1) {
	//case LEFT:
	//	if (Player1.left >= StartX + 10) {
	//		Player1.left -= 5;
	//		Player1.right -= 5;
	//		for (int i = 0; i < Tile_CountY; i++)
	//			for (int j = 0; j < Tile_CountX; j++)
	//				if ((Collision(Tile[i][j], Player1.left, Player1.top) || Collision(Tile[i][j], Player1.left, Player1.bottom)) && (isBox[Sel_Map][i][j] || !Tile_Enable_Move[Sel_Map][i][j])) {
	//					Player1.left = Tile[i][j].right;
	//					Player1.right = Player1.left + Player_CX;
	//					// 블럭 충돌체크 부드럽게
	//					if (Tile[i][j].bottom < (Player1.top + Player1.bottom) / 2 && Tile_Enable_Move[Sel_Map][i + 1][j] && !isBox[Sel_Map][i + 1][j]) {
	//						Player1.top = Tile[i + 1][j].top;
	//						Player1.bottom = Player1.top + Player_CY;
	//					}
	//					if ((Player1.top + Player1.bottom) / 2 < Tile[i][j].top && Tile_Enable_Move[Sel_Map][i - 1][j] && !isBox[Sel_Map][i - 1][j]) {
	//						Player1.bottom = Tile[i - 1][j].bottom;
	//						Player1.top = Player1.bottom - Player_CY;
	//					}
	//				}
	//	}
	//	else if (Player1.left <= StartX + 10) {
	//		Player1.left = StartX;
	//		Player1.right = Player1.left + Player_CX;
	//	}
	//	// 물풍선 체크
	//	for (int i = 0; i < P1_bCount; i++) {
	//		if (P1_Bubble[i] && Collision(Tile_Bubble1[i], Player1.left, (Player1.top + Player1.bottom) / 2) &&
	//			Tile_Bubble1[i].right - 6 <= Player1.left && Player1.left <= Tile_Bubble1[i].right + 6) {
	//			Player1.left = Tile_Bubble1[i].right;
	//			Player1.right = Player1.left + Player_CX;
	//		}
	//	}
	//	break;
	//case RIGHT:
	//	if (Player1.right <= Tile[12][14].right - 10) {
	//		Player1.left += 5;
	//		Player1.right += 5;
	//		for (int i = 0; i < Tile_CountY; i++)
	//			for (int j = 0; j < Tile_CountX; j++)
	//				if ((Collision(Tile[i][j], Player1.right, Player1.top) || Collision(Tile[i][j], Player1.right, Player1.bottom)) && (isBox[Sel_Map][i][j] || !Tile_Enable_Move[Sel_Map][i][j])) {
	//					Player1.right = Tile[i][j].left;
	//					Player1.left = Player1.right - Player_CX;
	//					// 블럭 충돌체크 부드럽게
	//					if (Tile[i][j].bottom < (Player1.top + Player1.bottom) / 2 && Tile_Enable_Move[Sel_Map][i + 1][j] && !isBox[Sel_Map][i + 1][j]) {
	//						Player1.top = Tile[i + 1][j].top;
	//						Player1.bottom = Player1.top + Player_CY;
	//					}
	//					if ((Player1.top + Player1.bottom) / 2 < Tile[i][j].top && Tile_Enable_Move[Sel_Map][i - 1][j] && !isBox[Sel_Map][i - 1][j]) {
	//						Player1.bottom = Tile[i - 1][j].bottom;
	//						Player1.top = Player1.bottom - Player_CY;
	//					}
	//				}
	//	}
	//	else if (Player1.right >= Tile[12][14].right - 10) {
	//		Player1.right = Tile[12][14].right;
	//		Player1.left = Player1.right - Player_CX;
	//	}
	//	// 물풍선 체크
	//	for (int i = 0; i < P1_bCount; i++) {
	//		if (P1_Bubble[i] && Collision(Tile_Bubble1[i], Player1.right, (Player1.top + Player1.bottom) / 2) &&
	//			Tile_Bubble1[i].left - 6 <= Player1.right && Player1.right <= Tile_Bubble1[i].left + 6) {
	//			Player1.right = Tile_Bubble1[i].left;
	//			Player1.left = Player1.right - Player_CX;
	//		}
	//	}
	//	break;
	//case UP:
	//	if (Player1.top >= StartY + 10) {
	//		Player1.top -= 5;
	//		Player1.bottom -= 5;
	//		for (int i = 0; i < Tile_CountY; i++)
	//			for (int j = 0; j < Tile_CountX; j++)
	//				if ((Collision(Tile[i][j], Player1.left, Player1.top) || Collision(Tile[i][j], Player1.right, Player1.top)) && (isBox[Sel_Map][i][j] || !Tile_Enable_Move[Sel_Map][i][j])) {
	//					Player1.top = Tile[i][j].bottom;
	//					Player1.bottom = Player1.top + Player_CY;
	//					// 블럭 충돌체크 부드럽게
	//					if (Tile[i][j].right < (Player1.left + Player1.right) / 2 && Tile_Enable_Move[Sel_Map][i][j + 1] && !isBox[Sel_Map][i][j + 1]) {
	//						Player1.left = Tile[i][j + 1].left;
	//						Player1.right = Player1.left + Player_CX;
	//					}
	//					if ((Player1.left + Player1.right) / 2 < Tile[i][j].left && Tile_Enable_Move[Sel_Map][i][j - 1] && !isBox[Sel_Map][i][j - 1]) {
	//						Player1.right = Tile[i][j - 1].right;
	//						Player1.left = Player1.right - Player_CX;
	//					}
	//				}
	//	}
	//	else if (Player1.top <= StartY + 5) {
	//		Player1.top = StartY;
	//		Player1.bottom = Player1.top + Player_CY;
	//	}
	//	// 물풍선 체크
	//	for (int i = 0; i < P1_bCount; i++) {
	//		if (P1_Bubble[i] && Collision(Tile_Bubble1[i], (Player1.left + Player1.right) / 2, Player1.top) &&
	//			Tile_Bubble1[i].bottom - 6 <= Player1.top && Player1.top <= Tile_Bubble1[i].bottom + 6) {
	//			Player1.top = Tile_Bubble1[i].bottom;
	//			Player1.bottom = Player1.top + Player_CY;
	//		}
	//	}
	//	break;
	//case DOWN:
	//	if (Player1.bottom <= Tile[12][14].bottom - 10) {
	//		Player1.top += 5;
	//		Player1.bottom += 5;
	//		for (int i = 0; i < Tile_CountY; i++)
	//			for (int j = 0; j < Tile_CountX; j++)
	//				if ((Collision(Tile[i][j], Player1.left, Player1.bottom) || Collision(Tile[i][j], Player1.right, Player1.bottom)) && (isBox[Sel_Map][i][j] || !Tile_Enable_Move[Sel_Map][i][j])) {
	//					Player1.bottom = Tile[i][j].top;
	//					Player1.top = Player1.bottom - Player_CY;
	//					// 블럭 충돌체크 부드럽게
	//					if (Tile[i][j].right < (Player1.left + Player1.right) / 2 && Tile_Enable_Move[Sel_Map][i][j + 1] && !isBox[Sel_Map][i][j + 1]) {
	//						Player1.left = Tile[i][j + 1].left;
	//						Player1.right = Player1.left + Player_CX;
	//					}
	//					if ((Player1.left + Player1.right) / 2 < Tile[i][j].left && Tile_Enable_Move[Sel_Map][i][j - 1] && !isBox[Sel_Map][i][j - 1]) {
	//						Player1.right = Tile[i][j - 1].right;
	//						Player1.left = Player1.right - Player_CX;
	//					}
	//				}
	//	}
	//	else if (Player1.bottom >= Tile[12][14].bottom - 10) {
	//		Player1.bottom = Tile[12][14].bottom;
	//		Player1.top = Player1.bottom - Player_CY;
	//	}
	//	// 물풍선 체크
	//	for (int i = 0; i < P1_bCount; i++) {
	//		if (P1_Bubble[i] && Collision(Tile_Bubble1[i], (Player1.left + Player1.right) / 2, Player1.bottom) &&
	//			Tile_Bubble1[i].top - 6 <= Player1.bottom && Player1.bottom <= Tile_Bubble1[i].top + 6) {
	//			Player1.bottom = Tile_Bubble1[i].top;
	//			Player1.top = Player1.bottom - Player_CY;
	//		}
	//	}
	//	break;
	//}
	InvalidateRect(hWnd, NULL, FALSE);
}

void CALLBACK TimeProc_P2_Move(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	switch (yPos_P2) {
	case LEFT:
		if (Player2.left >= StartX + 10) {
			Player2.left -= 5;
			Player2.right -= 5;
			for (int i = 0; i < Tile_CountY; i++)
				for (int j = 0; j < Tile_CountX; j++)
					if ((Collision(Tile[i][j], Player2.left, Player2.top) || Collision(Tile[i][j], Player2.left, Player2.bottom)) && (isBox[Sel_Map][i][j] || !Tile_Enable_Move[Sel_Map][i][j])) {
						Player2.left = Tile[i][j].right;
						Player2.right = Player2.left + Player_CX;
						// 블럭 충돌체크 부드럽게
						if (Tile[i][j].bottom < (Player2.top + Player2.bottom) / 2 && Tile_Enable_Move[Sel_Map][i + 1][j] && !isBox[Sel_Map][i + 1][j]) {
							Player2.top = Tile[i + 1][j].top;
							Player2.bottom = Player2.top + Player_CY;
						}
						if ((Player2.top + Player2.bottom) / 2 < Tile[i][j].top && Tile_Enable_Move[Sel_Map][i - 1][j] && !isBox[Sel_Map][i - 1][j]) {
							Player2.bottom = Tile[i - 1][j].bottom;
							Player2.top = Player2.bottom - Player_CY;
						}
					}
		}
		else if (Player2.left <= StartX + 10) {
			Player2.left = StartX;
			Player2.right = Player2.left + Player_CX;
		}
		// 물풍선 체크
		for (int i = 0; i < P2_bCount; i++) {
			if (P2_Bubble[i] && Collision(Tile_Bubble2[i], Player2.left, (Player2.top + Player2.bottom) / 2) &&
				Tile_Bubble2[i].right - 6 <= Player2.left && Player2.left <= Tile_Bubble2[i].right + 6) {
				Player2.left = Tile_Bubble2[i].right;
				Player2.right = Player2.left + Player_CX;
			}
		}
		break;
	case RIGHT:
		if (Player2.right <= Tile[12][14].right - 10) {
			Player2.left += 5;
			Player2.right += 5;
			for (int i = 0; i < Tile_CountY; i++)
				for (int j = 0; j < Tile_CountX; j++)
					if ((Collision(Tile[i][j], Player2.right, Player2.top) || Collision(Tile[i][j], Player2.right, Player2.bottom)) && (isBox[Sel_Map][i][j] || !Tile_Enable_Move[Sel_Map][i][j])) {
						Player2.right = Tile[i][j].left;
						Player2.left = Player2.right - Player_CX;
						// 블럭 충돌체크 부드럽게
						if (Tile[i][j].bottom < (Player2.top + Player2.bottom) / 2 && Tile_Enable_Move[Sel_Map][i + 1][j] && !isBox[Sel_Map][i + 1][j]) {
							Player2.top = Tile[i + 1][j].top;
							Player2.bottom = Player2.top + Player_CY;
						}
						if ((Player2.top + Player2.bottom) / 2 < Tile[i][j].top && Tile_Enable_Move[Sel_Map][i - 1][j] && !isBox[Sel_Map][i - 1][j]) {
							Player2.bottom = Tile[i - 1][j].bottom;
							Player2.top = Player2.bottom - Player_CY;
						}
					}
		}
		else if (Player2.right >= Tile[12][14].right - 10) {
			Player2.right = Tile[12][14].right;
			Player2.left = Player2.right - Player_CX;
		}
		// 물풍선 체크
		for (int i = 0; i < P2_bCount; i++) {
			if (P2_Bubble[i] && Collision(Tile_Bubble2[i], Player2.right, (Player2.top + Player2.bottom) / 2) &&
				Tile_Bubble2[i].left - 6 <= Player2.right && Player2.right <= Tile_Bubble2[i].left + 6) {
				Player2.right = Tile_Bubble2[i].left;
				Player2.left = Player2.right - Player_CX;
			}
		}
		break;
	case UP:
		if (Player2.top >= StartY + 10) {
			Player2.top -= 5;
			Player2.bottom -= 5;
			for (int i = 0; i < Tile_CountY; i++)
				for (int j = 0; j < Tile_CountX; j++)
					if ((Collision(Tile[i][j], Player2.left, Player2.top) || Collision(Tile[i][j], Player2.right, Player2.top)) && (isBox[Sel_Map][i][j] || !Tile_Enable_Move[Sel_Map][i][j])) {
						Player2.top = Tile[i][j].bottom;
						Player2.bottom = Player2.top + Player_CY;
						// 블럭 충돌체크 부드럽게
						if (Tile[i][j].right < (Player2.left + Player2.right) / 2 && Tile_Enable_Move[Sel_Map][i][j + 1] && !isBox[Sel_Map][i][j + 1]) {
							Player2.left = Tile[i][j + 1].left;
							Player2.right = Player2.left + Player_CX;
						}
						if ((Player2.left + Player2.right) / 2 < Tile[i][j].left && Tile_Enable_Move[Sel_Map][i][j - 1] && !isBox[Sel_Map][i][j - 1]) {
							Player2.right = Tile[i][j - 1].right;
							Player2.left = Player2.right - Player_CX;
						}
					}
		}
		else if (Player2.top <= StartY + 5) {
			Player2.top = StartY;
			Player2.bottom = Player2.top + Player_CY;
		}
		// 물풍선 체크
		for (int i = 0; i < P2_bCount; i++) {
			if (P2_Bubble[i] && Collision(Tile_Bubble2[i], (Player2.left + Player2.right) / 2, Player2.top) &&
				Tile_Bubble2[i].bottom - 6 <= Player2.top && Player2.top <= Tile_Bubble2[i].bottom + 6) {
				Player2.top = Tile_Bubble2[i].bottom;
				Player2.bottom = Player2.top + Player_CY;
			}
		}
		break;
	case DOWN:
		if (Player2.bottom <= Tile[12][14].bottom - 10) {
			Player2.top += 5;
			Player2.bottom += 5;
			for (int i = 0; i < Tile_CountY; i++)
				for (int j = 0; j < Tile_CountX; j++)
					if ((Collision(Tile[i][j], Player2.left, Player2.bottom) || Collision(Tile[i][j], Player2.right, Player2.bottom)) && (isBox[Sel_Map][i][j] || !Tile_Enable_Move[Sel_Map][i][j])) {
						Player2.bottom = Tile[i][j].top;
						Player2.top = Player2.bottom - Player_CY;
						// 블럭 충돌체크 부드럽게
						if (Tile[i][j].right < (Player2.left + Player2.right) / 2 && Tile_Enable_Move[Sel_Map][i][j + 1] && !isBox[Sel_Map][i][j + 1]) {
							Player2.left = Tile[i][j + 1].left;
							Player2.right = Player2.left + Player_CX;
						}
						if ((Player2.left + Player2.right) / 2 < Tile[i][j].left && Tile_Enable_Move[Sel_Map][i][j - 1] && !isBox[Sel_Map][i][j - 1]) {
							Player2.right = Tile[i][j - 1].right;
							Player2.left = Player2.right - Player_CX;
						}
					}
		}
		else if (Player2.bottom >= Tile[12][14].bottom - 10) {
			Player2.bottom = Tile[12][14].bottom;
			Player2.top = Player2.bottom - Player_CY;
		}
		// 물풍선 체크
		for (int i = 0; i < P2_bCount; i++) {
			if (P2_Bubble[i] && Collision(Tile_Bubble2[i], (Player2.left + Player2.right) / 2, Player2.bottom) &&
				Tile_Bubble2[i].top - 6 <= Player2.bottom && Player2.bottom <= Tile_Bubble2[i].top + 6) {
				Player2.bottom = Tile_Bubble2[i].top;
				Player2.top = Player2.bottom - Player_CY;
			}
		}
		break;
	}
	InvalidateRect(hWnd, NULL, FALSE);
}

void CALLBACK TimeProc_InBubble(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	if (P1_InBubble)
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
	}
	InvalidateRect(hWnd, NULL, FALSE);
}

void CALLBACK TimeProc_Die(HWND hWnd, UINT uMsg, UINT ideEvent, DWORD dwTime)
{
	static int cnt1 = 0, cnt2 = 0;

	if (P1_Remove && P2_Remove)
		KillTimer(hWnd, Die);

	if (P1_Die)
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
	}
	InvalidateRect(hWnd, NULL, false);
}

void CALLBACK TimeProc_Monster_Move(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	srand((unsigned)time(NULL));

	RECT rc;
	int random;
	random = rand() % 5;

	if (!Mon1_Live) {
		if (++xPos_Mon1 % 2) {
			yPos_Mon1++;
			xPos_Mon1 = 1;
		}
	}
	if (!Mon2_Live) {
		if (++xPos_Mon2 % 2) {
			yPos_Mon2++;
			xPos_Mon2 = 1;
		}
	}

	if (!Mon1_Live && !Mon2_Live) {
		KillTimer(hwnd, Monster_Move);
	}
	switch (yPos_Mon1) {
	case M_LEFT:
		if (Monster1.left >= StartX + 5) {
			Monster1.left -= 4;
			Monster1.right -= 4;
			for (int i = 0; i < Tile_CountY; i++)
				for (int j = 0; j < Tile_CountX; j++) {
					if ((Collision(Tile[i][j], Monster1.left, Monster1.top + 1) || Collision(Tile[i][j], Monster1.left, Monster1.bottom - 1)) && (isBox[Sel_Map][i][j] || !Tile_Enable_Move[Sel_Map][i][j])) {
						Monster1.left = Tile[i][j].right;
						Monster1.right = Monster1.left + Monster1_CX;
						yPos_Mon1 = M_RIGHT;
					}
					if (Tile[i][j].left <= Monster1.left && Monster1.left <= Tile[i][j].left + 2 &&
						((Tile[i][j].top <= Monster1.top && Monster1.top <= Tile[i][j].top + 2) || (Tile[i][j].bottom - 2 <= Monster1.bottom && Monster1.bottom <= Tile[i][j].bottom))) {
						// 상
						if (Tile_Enable_Move[Sel_Map][i - 1][j] && !isBox[Sel_Map][i - 1][j] && random == 0)
							yPos_Mon1 = M_UP;
						// 하
						else if (Tile_Enable_Move[Sel_Map][i + 1][j] && !isBox[Sel_Map][i + 1][j] && random == 1)
							yPos_Mon1 = M_DOWN;
					}
				}
		}
		else {
			Monster1.left = StartX;
			Monster1.right = Monster1.left + Monster1_CX;
			if (yPos_Mon1 == M_LEFT)
				yPos_Mon1 = M_RIGHT;
		}
		// 물풍선 체크
		for (int i = 0; i < P1_bCount; i++) {
			if (P1_Bubble[i] && Collision(Tile_Bubble1[i], Monster1.left, (Monster1.top + Monster1.bottom) / 2) &&
				Tile_Bubble1[i].right - 6 <= Monster1.left && Monster1.left <= Tile_Bubble1[i].right + 6) {
				Monster1.left = Tile_Bubble1[i].right;
				Monster1.right = Monster1.left + Monster1_CX;
			}
		}
		break;
	case M_RIGHT:
		if (Monster1.right <= Tile[12][14].right - 5) {
			Monster1.left += 4;
			Monster1.right += 4;
			for (int i = 0; i < Tile_CountY; i++)
				for (int j = 0; j < Tile_CountX; j++) {
					if ((Collision(Tile[i][j], Monster1.right, Monster1.top + 1) || Collision(Tile[i][j], Monster1.right, Monster1.bottom - 1)) && (isBox[Sel_Map][i][j] || !Tile_Enable_Move[Sel_Map][i][j])) {
						Monster1.right = Tile[i][j].left;
						Monster1.left = Monster1.right - Monster1_CX;
						yPos_Mon1 = M_LEFT;
					}
					if (Tile[i][j].left <= Monster1.left && Monster1.left <= Tile[i][j].left + 2 &&
						((Tile[i][j].top <= Monster1.top && Monster1.top <= Tile[i][j].top + 2) || (Tile[i][j].bottom - 2 <= Monster1.bottom && Monster1.bottom <= Tile[i][j].bottom))) {
						// 상
						if (Tile_Enable_Move[Sel_Map][i - 1][j] && !isBox[Sel_Map][i - 1][j] && random == 0)
							yPos_Mon1 = M_UP;
						// 하
						else if (Tile_Enable_Move[Sel_Map][i + 1][j] && !isBox[Sel_Map][i + 1][j] && random == 1)
							yPos_Mon1 = M_DOWN;
					}
				}
		}
		else {
			Monster1.right = Tile[12][14].right;
			Monster1.left = Monster1.right - Monster1_CX;
			if (yPos_Mon1 == M_RIGHT)
				yPos_Mon1 = M_LEFT;
		}
		// 물풍선 체크
		for (int i = 0; i < P1_bCount; i++) {
			if (P1_Bubble[i] && Collision(Tile_Bubble1[i], Monster1.right, (Monster1.top + Monster1.bottom) / 2) &&
				Tile_Bubble1[i].left - 6 <= Monster1.right && Monster1.right <= Tile_Bubble1[i].left + 6) {
				Monster1.right = Tile_Bubble1[i].left;
				Monster1.left = Monster1.right - Monster1_CX;
			}
		}
		break;
	case M_UP:
		if (Monster1.top >= StartY + 5) {
			Monster1.top -= 4;
			Monster1.bottom -= 4;
			for (int i = 0; i < Tile_CountY; i++)
				for (int j = 0; j < Tile_CountX; j++) {
					if ((Collision(Tile[i][j], Monster1.left + 1, Monster1.top) || Collision(Tile[i][j], Monster1.right - 1, Monster1.top)) && (isBox[Sel_Map][i][j] || !Tile_Enable_Move[Sel_Map][i][j])) {
						Monster1.top = Tile[i][j].bottom;
						Monster1.bottom = Monster1.top + Monster1_CY;
						yPos_Mon1 = M_DOWN;
					}
					if (Tile[i][j].top <= Monster1.top && Monster1.top <= Tile[i][j].top + 2 &&
						((Tile[i][j].left <= Monster1.left && Monster1.left <= Tile[i][j].left + 2) || (Tile[i][j].right - 2 <= Monster1.right && Monster1.right <= Tile[i][j].right))) {
						// 좌
						if (Tile_Enable_Move[Sel_Map][i][j - 1] && !isBox[Sel_Map][i][j - 1] && random == 0)
							yPos_Mon1 = M_LEFT;
						// 우
						else if (Tile_Enable_Move[Sel_Map][i][j + 1] && !isBox[Sel_Map][i][j + 1] && random == 1)
							yPos_Mon1 = M_RIGHT;
					}
				}
		}
		else {
			Monster1.top = StartY;
			Monster1.bottom = Monster1.top + Monster1_CY;
			if (yPos_Mon1 == M_UP)
				yPos_Mon1 = M_DOWN;
		}
		// 물풍선 체크
		for (int i = 0; i < P1_bCount; i++) {
			if (P1_Bubble[i] && Collision(Tile_Bubble1[i], (Monster1.left + Monster1.right) / 2, Monster1.top) &&
				Tile_Bubble1[i].bottom - 6 <= Monster1.top && Monster1.top <= Tile_Bubble1[i].bottom + 6) {
				Monster1.top = Tile_Bubble1[i].bottom;
				Monster1.bottom = Monster1.top + Monster1_CY;
			}
		}
		break;
	case M_DOWN:
		if (Monster1.bottom <= Tile[12][14].bottom - 5) {
			Monster1.top += 4;
			Monster1.bottom += 4;
			for (int i = 0; i < Tile_CountY; i++)
				for (int j = 0; j < Tile_CountX; j++) {
					if ((Collision(Tile[i][j], Monster1.left + 1, Monster1.bottom) || Collision(Tile[i][j], Monster1.right - 1, Monster1.bottom)) && (isBox[Sel_Map][i][j] || !Tile_Enable_Move[Sel_Map][i][j])) {
						Monster1.bottom = Tile[i][j].top;
						Monster1.top = Monster1.bottom - Monster1_CY;
						yPos_Mon1 = M_UP;
					}
					if (Tile[i][j].top <= Monster1.top && Monster1.top <= Tile[i][j].top + 2 &&
						((Tile[i][j].left <= Monster1.left && Monster1.left <= Tile[i][j].left + 2) || (Tile[i][j].right - 2 <= Monster1.right && Monster1.right <= Tile[i][j].right))) {
						// 좌
						if (Tile_Enable_Move[Sel_Map][i][j - 1] && !isBox[Sel_Map][i][j - 1] && random == 0)
							yPos_Mon1 = M_LEFT;
						// 우
						else if (Tile_Enable_Move[Sel_Map][i][j + 1] && !isBox[Sel_Map][i][j + 1] && random == 1)
							yPos_Mon1 = M_RIGHT;
					}

				}
		}
		else {
			Monster1.bottom = Tile[12][14].bottom;
			Monster1.top = Monster1.bottom - Monster1_CY;
			if (yPos_Mon1 == M_DOWN)
				yPos_Mon1 = M_UP;
		}
		// 물풍선 체크
		for (int i = 0; i < P1_bCount; i++) {
			if (P1_Bubble[i] && Collision(Tile_Bubble1[i], (Monster1.left + Monster1.right) / 2, Monster1.bottom) &&
				Tile_Bubble1[i].top - 6 <= Monster1.bottom && Monster1.bottom <= Tile_Bubble1[i].top + 6) {
				Monster1.bottom = Tile_Bubble1[i].top;
				Monster1.top = Monster1.bottom - Monster1_CY;
			}
		}
		break;
	}
	switch (yPos_Mon2) {
	case M_LEFT:
		if (Monster2.left >= StartX + 5) {
			Monster2.left -= 5;
			Monster2.right -= 5;
			for (int i = 0; i < Tile_CountY; i++)
				for (int j = 0; j < Tile_CountX; j++) {
					if ((Collision(Tile[i][j], Monster2.left, Monster2.top + 1) || Collision(Tile[i][j], Monster2.left, Monster2.bottom - 1)) && (isBox[Sel_Map][i][j] || !Tile_Enable_Move[Sel_Map][i][j])) {
						Monster2.left = Tile[i][j].right;
						Monster2.right = Monster2.left + Monster2_CX;
						yPos_Mon2 = M_RIGHT;
					}
					if (Tile[i][j].left <= Monster2.left && Monster2.left <= Tile[i][j].left + 2 &&
						((Tile[i][j].top <= Monster2.top && Monster2.top <= Tile[i][j].top + 2) || (Tile[i][j].bottom - 2 <= Monster2.bottom && Monster2.bottom <= Tile[i][j].bottom))) {
						// 상
						if (Tile_Enable_Move[Sel_Map][i - 1][j] && !isBox[Sel_Map][i - 1][j] && random == 0)
							yPos_Mon2 = M_UP;
						// 하
						else if (Tile_Enable_Move[Sel_Map][i + 1][j] && !isBox[Sel_Map][i + 1][j] && random == 1)
							yPos_Mon2 = M_DOWN;
						// 우
						/*else if (Tile_Enable_Move[Sel_Map][i][j + 1] && !isBox[Sel_Map][i][j + 1] && random == M_RIGHT)
						yPos_Mon2 = M_RIGHT;*/
					}
				}
		}
		else {
			Monster2.left = StartX;
			Monster2.right = Monster2.left + Monster2_CX;
			if (yPos_Mon2 == M_LEFT)
				yPos_Mon2 = M_RIGHT;
		}
		// 물풍선 체크
		for (int i = 0; i < P1_bCount; i++) {
			if (P1_Bubble[i] && Collision(Tile_Bubble1[i], Monster2.left, (Monster2.top + Monster2.bottom) / 2) &&
				Tile_Bubble1[i].right - 6 <= Monster2.left && Monster2.left <= Tile_Bubble1[i].right + 6) {
				Monster2.left = Tile_Bubble1[i].right;
				Monster2.right = Monster2.left + Monster2_CX;
			}
		}
		break;
	case M_RIGHT:
		if (Monster2.right <= Tile[12][14].right - 5) {
			Monster2.left += 5;
			Monster2.right += 5;
			for (int i = 0; i < Tile_CountY; i++)
				for (int j = 0; j < Tile_CountX; j++) {
					if ((Collision(Tile[i][j], Monster2.right, Monster2.top + 1) || Collision(Tile[i][j], Monster2.right, Monster2.bottom - 1)) && (isBox[Sel_Map][i][j] || !Tile_Enable_Move[Sel_Map][i][j])) {
						Monster2.right = Tile[i][j].left;
						Monster2.left = Monster2.right - Monster2_CX;
						yPos_Mon2 = M_LEFT;
					}
					if (Tile[i][j].left <= Monster2.left && Monster2.left <= Tile[i][j].left + 2 &&
						((Tile[i][j].top <= Monster2.top && Monster2.top <= Tile[i][j].top + 2) || (Tile[i][j].bottom - 2 <= Monster2.bottom && Monster2.bottom <= Tile[i][j].bottom))) {
						// 상
						if (Tile_Enable_Move[Sel_Map][i - 1][j] && !isBox[Sel_Map][i - 1][j] && random == 0)
							yPos_Mon2 = M_UP;
						// 하
						else if (Tile_Enable_Move[Sel_Map][i + 1][j] && !isBox[Sel_Map][i + 1][j] && random == 1)
							yPos_Mon2 = M_DOWN;
						// 좌
						/*if (Tile_Enable_Move[Sel_Map][i][j - 1] && !isBox[Sel_Map][i][j - 1] && random == M_LEFT)
						yPos_Mon2 = M_LEFT;*/
					}
				}
		}
		else {
			Monster2.right = Tile[12][14].right;
			Monster2.left = Monster2.right - Monster2_CX;
			if (yPos_Mon2 == M_RIGHT)
				yPos_Mon2 = M_LEFT;
		}
		// 물풍선 체크
		for (int i = 0; i < P1_bCount; i++) {
			if (P1_Bubble[i] && Collision(Tile_Bubble1[i], Monster2.right, (Monster2.top + Monster2.bottom) / 2) &&
				Tile_Bubble1[i].left - 6 <= Monster2.right && Monster2.right <= Tile_Bubble1[i].left + 6) {
				Monster2.right = Tile_Bubble1[i].left;
				Monster2.left = Monster2.right - Monster2_CX;
			}
		}
		break;
	case M_UP:
		if (Monster2.top >= StartY + 5) {
			Monster2.top -= 5;
			Monster2.bottom -= 5;
			for (int i = 0; i < Tile_CountY; i++)
				for (int j = 0; j < Tile_CountX; j++) {
					if ((Collision(Tile[i][j], Monster2.left + 1, Monster2.top) || Collision(Tile[i][j], Monster2.right - 1, Monster2.top)) && (isBox[Sel_Map][i][j] || !Tile_Enable_Move[Sel_Map][i][j])) {
						Monster2.top = Tile[i][j].bottom;
						Monster2.bottom = Monster2.top + Monster2_CY;
						yPos_Mon2 = M_DOWN;
					}
					if (Tile[i][j].top <= Monster2.top && Monster2.top <= Tile[i][j].top + 2 &&
						((Tile[i][j].left <= Monster2.left && Monster2.left <= Tile[i][j].left + 2) || (Tile[i][j].right - 2 <= Monster2.right && Monster2.right <= Tile[i][j].right))) {
						// 좌
						if (Tile_Enable_Move[Sel_Map][i][j - 1] && !isBox[Sel_Map][i][j - 1] && random == 0)
							yPos_Mon2 = M_LEFT;
						// 우
						else if (Tile_Enable_Move[Sel_Map][i][j + 1] && !isBox[Sel_Map][i][j + 1] && random == 1)
							yPos_Mon2 = M_RIGHT;
						// 하
						/*else if (Tile_Enable_Move[Sel_Map][i + 1][j] && !isBox[Sel_Map][i + 1][j] && random == M_DOWN)
						yPos_Mon2 = M_DOWN;*/
					}
				}
		}
		else {
			Monster2.top = StartY;
			Monster2.bottom = Monster2.top + Monster2_CY;
			if (yPos_Mon2 == M_UP)
				yPos_Mon2 = M_DOWN;
		}
		// 물풍선 체크
		for (int i = 0; i < P1_bCount; i++) {
			if (P1_Bubble[i] && Collision(Tile_Bubble1[i], (Monster2.left + Monster2.right) / 2, Monster2.top) &&
				Tile_Bubble1[i].bottom - 6 <= Monster2.top && Monster2.top <= Tile_Bubble1[i].bottom + 6) {
				Monster2.top = Tile_Bubble1[i].bottom;
				Monster2.bottom = Monster2.top + Monster2_CY;
			}
		}
		break;
	case M_DOWN:
		if (Monster2.bottom <= Tile[12][14].bottom - 5) {
			Monster2.top += 5;
			Monster2.bottom += 5;
			for (int i = 0; i < Tile_CountY; i++)
				for (int j = 0; j < Tile_CountX; j++) {
					if ((Collision(Tile[i][j], Monster2.left + 1, Monster2.bottom) || Collision(Tile[i][j], Monster2.right - 1, Monster2.bottom)) && (isBox[Sel_Map][i][j] || !Tile_Enable_Move[Sel_Map][i][j])) {
						Monster2.bottom = Tile[i][j].top;
						Monster2.top = Monster2.bottom - Monster2_CY;
						yPos_Mon2 = M_UP;
					}
					if (Tile[i][j].top <= Monster2.top && Monster2.top <= Tile[i][j].top + 2 &&
						((Tile[i][j].left <= Monster2.left && Monster2.left <= Tile[i][j].left + 2) || (Tile[i][j].right - 2 <= Monster2.right && Monster2.right <= Tile[i][j].right))) {
						// 좌
						if (Tile_Enable_Move[Sel_Map][i][j - 1] && !isBox[Sel_Map][i][j - 1] && random == 0)
							yPos_Mon2 = M_LEFT;
						// 우
						else if (Tile_Enable_Move[Sel_Map][i][j + 1] && !isBox[Sel_Map][i][j + 1] && random == 1)
							yPos_Mon2 = M_RIGHT;
						// 상
						/*if (Tile_Enable_Move[Sel_Map][i - 1][j] && !isBox[Sel_Map][i - 1][j] && random == M_UP)
						yPos_Mon2 = M_UP;*/
					}

				}
		}
		else {
			Monster2.bottom = Tile[12][14].bottom;
			Monster2.top = Monster2.bottom - Monster2_CY;
			if (yPos_Mon2 == M_DOWN)
				yPos_Mon2 = M_UP;
		}
		// 물풍선 체크
		for (int i = 0; i < P1_bCount; i++) {
			if (P1_Bubble[i] && Collision(Tile_Bubble1[i], (Monster2.left + Monster2.right) / 2, Monster2.bottom) &&
				Tile_Bubble1[i].top - 6 <= Monster2.bottom && Monster2.bottom <= Tile_Bubble1[i].top + 6) {
				Monster2.bottom = Tile_Bubble1[i].top;
				Monster2.top = Monster2.bottom - Monster2_CY;
			}
		}
		break;
	}

	if ((IntersectRect(&rc, &Monster1, &Player1) && Mon1_Live) || (IntersectRect(&rc, &Monster2, &Player1) && Mon2_Live)) {
		P1_Die = TRUE;
		SetTimer(hwnd, Die, 150, (TIMERPROC)TimeProc_Die);
	}
	if ((IntersectRect(&rc, &Monster1, &Player2) && Mon1_Live) || (IntersectRect(&rc, &Monster2, &Player2) && Mon2_Live)) {
		P2_Die = TRUE;
		SetTimer(hwnd, Die, 150, (TIMERPROC)TimeProc_Die);
	}

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
			KillTimer(hWnd, 8);
			Counting = 0;
		}
	}

	if (Ending)
	{
		if (++Counting % 20)
		{
			Ending = FALSE;
			SetPos();
			P1_InBubble = false;
			P1_Die = false;
			P2_InBubble = false;
			P2_Die = false;
			bSceneChange = true;
			GameState = ROBBY;
			KillTimer(hWnd, 8);
			Counting = 0;
			bSceneChange = false;
		}
	}
}

// 키보드 관련 함수
void KEY_DOWN_P1(HWND hWnd)
{
	if (GameState == INGAME)
	{
		if (!P1_Die && P1_Live) {
			if (GetAsyncKeyState(VK_CONTROL) & 0x8000 && P1_InBubble && P1_N) {
				Send_Client_Packet = input_ctrl;
				SetEvent(hSendEvent);
				//P1_Speed = P1_tSpeed;
				//P1_InBubble = FALSE;
				//CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Character_Revival.ogg");
			}

			if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
				Send_Client_Packet = input_bottom;
				SetEvent(hSendEvent);
				yPos_P1 = DOWN;
				SetTimer(hwnd, P1, P1_Speed, (TIMERPROC)TimeProc_P1_Move);
				P1_Move = TRUE;
			}
			if (GetAsyncKeyState(VK_UP) & 0x8000) {
				Send_Client_Packet = input_top;
				SetEvent(hSendEvent);
				yPos_P1 = UP;
				SetTimer(hwnd, P1, P1_Speed, (TIMERPROC)TimeProc_P1_Move);
				P1_Move = TRUE;
			}
			if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
				Send_Client_Packet = input_left;
				SetEvent(hSendEvent);
				yPos_P1 = LEFT;
				SetTimer(hwnd, P1, P1_Speed, (TIMERPROC)TimeProc_P1_Move);
				P1_Move = TRUE;
			}
			if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
				Send_Client_Packet = input_right;
				SetEvent(hSendEvent);
				yPos_P1 = RIGHT;
				SetTimer(hwnd, P1, P1_Speed, (TIMERPROC)TimeProc_P1_Move);
				P1_Move = TRUE;
			}
			if (GetAsyncKeyState(VK_SPACE) & 0x8000 && !P1_InBubble) {
				Send_Client_Packet = input_space;
				SetEvent(hSendEvent);
				for (int i = 0; i < P1_bCount; ++i)
				{
					if (!P1_Bubble[i] && !P1_Bubble_Flow[i])
					{
						for (int a = 0; a < 7; ++a)
							bEffect[a] = TRUE;

						for (int j = 0; j < P1_bCount; ++j) {
							if (Collision(Tile_Bubble1[j], (Player1.right + Player1.left) / 2, (Player1.top + Player1.bottom) / 2) || Collision(Tile_Bubble2[j], (Player1.right + Player1.left) / 2, (Player1.top + Player1.bottom) / 2)) {
								return;
							}
						}
						P1_Bubble[i] = TRUE;
						CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Bubble_On.ogg");
						for (int a = 0; a < Tile_CountY; a++)
							for (int b = 0; b < Tile_CountX; b++)
							{
								if (Collision(Tile[a][b], (Player1.right + Player1.left) / 2, (Player1.top + Player1.bottom) / 2)) {
									Tile_Bubble1[i] = Tile[a][b];
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
	switch (wParam) {
	case VK_F1:
		Helper = false;
		break;
	case VK_DOWN:
		if (yPos_P1 == DOWN) {
			KillTimer(hwnd, P1);
			xPos_P1 = 0;
			P1_Move = FALSE;
		}
		break;
	case VK_UP:
		if (yPos_P1 == UP) {
			KillTimer(hwnd, P1);
			xPos_P1 = 0;
			P1_Move = FALSE;
		}
		break;
	case VK_LEFT:
		if (yPos_P1 == LEFT) {
			KillTimer(hwnd, P1);
			xPos_P1 = 0;
			P1_Move = FALSE;
		}
		break;
	case VK_RIGHT:
		if (yPos_P1 == RIGHT) {
			KillTimer(hwnd, P1);
			xPos_P1 = 0;
			P1_Move = FALSE;
		}
		break;
	}
}
void KEY_DOWN_P2(HWND hWnd)
{
	if (GameState == INGAME) {


		if (!P2_Die && P2_Live) {
			if (GetAsyncKeyState('1') & 0x8000 && P2_InBubble && P2_N) {
				P2_Speed = P2_tSpeed;
				P2_InBubble = FALSE;
				CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Character_Revival.ogg");
			}

			if (GetAsyncKeyState('S') & 0x8000) {
				yPos_P2 = DOWN;
				if (!P2_Move) {
					SetTimer(hwnd, P2, P2_Speed, (TIMERPROC)TimeProc_P2_Move);
					P2_Move = TRUE;
				}
			}
			if (GetAsyncKeyState('W') & 0x8000) {
				yPos_P2 = UP;
				if (!P2_Move) {
					SetTimer(hwnd, P2, P2_Speed, (TIMERPROC)TimeProc_P2_Move);
					P2_Move = TRUE;
				}
			}
			if (GetAsyncKeyState('A') & 0x8000) {
				yPos_P2 = LEFT;
				if (!P2_Move) {
					SetTimer(hwnd, P2, P2_Speed, (TIMERPROC)TimeProc_P2_Move);
					P2_Move = TRUE;
				}
			}
			if (GetAsyncKeyState('D') & 0x8000) {
				yPos_P2 = RIGHT;
				if (!P2_Move) {
					SetTimer(hwnd, P2, P2_Speed, (TIMERPROC)TimeProc_P2_Move);
					P2_Move = TRUE;
				}
			}
			if (GetAsyncKeyState(VK_SHIFT) & 0x8000 && !P2_InBubble) {
				for (int i = 0; i < P2_bCount; ++i)
				{
					if (!P2_Bubble[i] && !P2_Bubble_Flow[i])
					{
						for (int a = 0; a < 7; ++a)
							bEffect2[a] = TRUE;

						for (int j = 0; j < P2_bCount; ++j) {
							if (Collision(Tile_Bubble2[j], (Player2.right + Player2.left) / 2, (Player2.top + Player2.bottom) / 2) || Collision(Tile_Bubble1[j], (Player2.right + Player2.left) / 2, (Player2.top + Player2.bottom) / 2)) {
								return;
							}
						}
						CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Bubble_On.ogg");
						for (int a = 0; a < Tile_CountY; a++)
							for (int b = 0; b < Tile_CountX; b++)
							{
								if (Collision(Tile[a][b], (Player2.right + Player2.left) / 2, (Player2.top + Player2.bottom) / 2)) {
									P2_Bubble[i] = TRUE;
									Tile_Bubble2[i] = Tile[a][b];
									return;
								}
							}
					}
				}
			}
		}
	}
}
void KEY_UP_P2(WPARAM wParam, HWND hWnd)
{
	switch (wParam) {
	case 'S':
		if (yPos_P2 == DOWN) {
			KillTimer(hwnd, P2);
			xPos_P2 = 0;
			P2_Move = FALSE;
		}
		break;
	case 'W':
		if (yPos_P2 == UP) {
			KillTimer(hwnd, P2);
			xPos_P2 = 0;
			P2_Move = FALSE;
		}
		break;
	case 'A':
		if (yPos_P2 == LEFT) {
			KillTimer(hwnd, P2);
			xPos_P2 = 4;
			P2_Move = FALSE;
		}
		break;
	case 'D':
		if (yPos_P2 == RIGHT) {
			KillTimer(hwnd, P2);
			xPos_P2 = 0;
			P2_Move = FALSE;
		}
		break;
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
	P2_NIDDLE_ON = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_NIDDLE));
	P1_PIN_ON = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_PIN));
	P2_PIN_ON = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_PIN));

	P1_NIDDLE_OFF = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_OFFNID));
	P2_NIDDLE_OFF = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_OFFNID));
	P1_PIN_OFF = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_OFFPIN));
	P2_PIN_OFF = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_OFFPIN));

	Lobby_Start = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_START));
	P1_On = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_READY));
	P2_On = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_READY));

	Items = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_Item));

	Tile2 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_Tile2));
	Block2 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_BLOCK));
	Steel2 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_STEEL));
	Stone2 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_STONE));

	Mon1Bit = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_MON1));
	Mon2Bit = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_MON2));

	P1_nOn = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_P1_NIDDLE));
	P2_nOn = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_P2_NIDDLE));
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
			if (ItemValue != 0 && ItemValue != 7 && isBox[0][i][j])
				Itemset[0][i][j] = ItemValue;
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

	Player2 = Tile[12][13];
	Player2.right = Player2.left + Player_CX;
	Player2.top = Player2.bottom - Player_CY;

	// 몬스터 좌표설정
	Monster1 = Tile[0][13];
	Monster1.right = Monster1.left + Monster1_CX;
	Monster1.top = Monster1.bottom - Monster1_CY;

	Monster2 = Tile[12][1];
	Monster2.right = Monster2.left + Monster1_CX;
	Monster2.top = Monster2.bottom - Monster1_CY;

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
	P2_NIDDLE.left = 232, P2_NIDDLE.top = 243, P2_NIDDLE.right = P2_NIDDLE.left + 33, P2_NIDDLE.bottom = P2_NIDDLE.top + 26;
	P1_PIN.left = 82, P1_PIN.top = 243, P1_PIN.right = P1_PIN.left + 33, P1_PIN.bottom = P1_PIN.top + 26;
	P2_PIN.left = 268, P2_PIN.top = 243, P2_PIN.right = P2_PIN.left + 33, P2_PIN.bottom = P2_PIN.top + 26;
}

