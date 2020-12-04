#include "Socket_Programming.h"
#include "Packet.h"
#include "SoundMgr.h"

// 이벤트
extern HANDLE hRecvEvent, hSendEvent, hConnectEvent, hPlayerEvent, hBubbleEvent, hInputEvent;
// 소켓
extern SOCKET sock;
// 패킷
extern InputPacket* Recv_Player_Packet;
extern InputPacket* Send_Client_Packet;

//핸들값
extern HWND hwnd;
// 플레이어
extern RECT Player1, Player2;
extern RECT Player[MAX_PLAYER];
// 게임 스테이트 enum GAME_BG { MENU = 1, ROBBY = 2, INGAME = 3};
extern int GameState;
extern const int Player_CX = 34;
extern const int Player_CY = 34;
extern int Client_Idx;
extern int nPlayer;
extern int xPos_Player[4];
extern int yPos_Player[4];
extern BOOL Player_Bubble[4][7]; 
extern BOOL Player_Bubble_Flow[4][7];
extern RECT Tile_Bubble[4][7];
extern int Power[4];
extern int Itemset[2][13][15];
extern BOOL bInBubble[4];
extern BOOL bDie[4];
extern int Player_bCount[4]; 
extern int Player_Speed[4];
extern BOOL Ending;
extern int BubbleResource[4];
extern int BubbleCount[4];
extern BOOL Player_Bubble_Boom[4][7];
extern int MoveResource[4][7];
extern int Player_Dying[4];
extern BOOL Player_Remove[4];
extern int Bubble_cnt[4][7];
extern BOOL Box_Break[13][15];



extern enum Player_Position { LEFT = 3, RIGHT = 2, UP = 0, DOWN = 1 };
extern enum GAME_BG { MENU = 1, ROBBY, INGAME };
extern enum Item { Ball = 1, OnePower = 6, Speed = 11, MaxPower = 16, RedDevil = 21 };
extern enum Timer { P1 = 0, P2 = 1, P3 = 2, P4 = 3, Bubble_BfBoom, Bubble_Flow, In_Bubble, Die, Monster_Move, Recv_Bubble };

extern int Sel_Map;

extern BOOL TextOn; 
extern bool bSceneChange;
extern BOOL SelectMap1, SelectMap2;					//맵선택 

extern BOOL Player_Live[MAX_PLAYER];
extern BOOL Player_Move[MAX_PLAYER];

extern void CALLBACK TimeProc_Text(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime);
extern void CALLBACK TimeProc_P1_Move(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime);
extern void CALLBACK TimeProc_InBubble(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime);
extern void CALLBACK TimeProc_Die(HWND hWnd, UINT uMsg, UINT ideEvent, DWORD dwTime);
extern void SetPos();

int recvn(SOCKET s, char* buf, int len, int flags)
{
    int received;
    char* ptr = buf;
    int left = len;

    while (left > 0) {
        received = recv(s, ptr, left, flags);
        if (received == SOCKET_ERROR)
            return SOCKET_ERROR;
        else if (received == 0)
            break;
        left -= received;
        ptr += received;
    }

    return (len - left);
}

// Receive를 수행할 스레드. 
DWORD WINAPI RecvClient(LPVOID arg)
{
    // 메뉴 -> 로비로 가기 전까지 기다린다.
    WaitForSingleObject(hRecvEvent, INFINITE);
    ResetEvent(hRecvEvent);
    int retval;

    char buf[BUFSIZE];

    // 윈속 초기화
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;

    // socket()
    sock = socket(AF_INET, SOCK_STREAM, 0);

    // 소켓 옵션에서 recv 함수가 0.5초 이상 대기하고 있으면 return 하도록 변경
    DWORD recvTimeout = 500; //0.5초
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&recvTimeout, sizeof(recvTimeout));

    // connect()
    SOCKADDR_IN serveraddr;
    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
    serveraddr.sin_port = htons(SERVERPORT);
    retval = connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR)
        MessageBox(hwnd, L"연결 안 됨", L"연결됨", MB_OK);
    else
        MessageBox(hwnd, L"연결됨", L"연결됨?", MB_OK);


    // 커넥트 이후 자신의 플레이어 패킷 수신
    printf("여기\n");
    retval = recvn(sock, buf, sizeof(InputPacket), 0);
    buf[retval] = '\0';
    Recv_Player_Packet = (InputPacket*)buf;
    SetEvent(hConnectEvent);    //connect가 끝나면 송신에서도 변수 사용가능하다는 이벤트 발생시킨다.
    printf("야 위치 받았다. %d %d\n\n", Recv_Player_Packet->x, Recv_Player_Packet->y);
    printf("Packet ID : %d\nPacket x : %d\nPacket y : %d\nPacket type : %d\n", Recv_Player_Packet->idx_player, Recv_Player_Packet->x, Recv_Player_Packet->y, Recv_Player_Packet->type);
    Client_Idx = Recv_Player_Packet->idx_player;
    nPlayer = Client_Idx + 1;
    for (int i = 0; i < nPlayer; ++i)
        Player_Live[i] = true;

    // 데이터 통신에 쓰일 while, 이 위에 처음 서버와 연결했을 때의 패킷을 받아오는 작업 필요
    while (1) {
        if (GameState == ROBBY) // 게임 스테이트가 메뉴일 때
        {
            retval = recvn(sock, buf, sizeof(InputPacket), 0);
            if (retval == SOCKET_ERROR)
                continue;
            buf[retval] = '\0';
            Recv_Player_Packet = (InputPacket*)buf;
            printf("Packet ID : %d\nPacket x : %d\nPacket y : %d\nPacket type : %d\n", Recv_Player_Packet->idx_player, Recv_Player_Packet->x, Recv_Player_Packet->y, Recv_Player_Packet->type);
            if (Recv_Player_Packet->type == PacketType::start)
            {
                printf("시작하랍신다~\n");
                // 게임 스타트 (GameState 3으로 바꾸고 사운드랑 불값 체인지 필요
                CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Button_Off.ogg");
                CSoundMgr::GetInstance()->PlayEffectSound2(L"SFX_Word_Start.ogg");
                TextOn = TRUE;
                SetTimer(hwnd, 8, 750, (TIMERPROC)TimeProc_Text);
                GameState = INGAME;
                bSceneChange = true;
                if (SelectMap1)
                    Sel_Map = 0;
                else
                    Sel_Map = 1;
            }
            else if (Recv_Player_Packet->type == PacketType::player)
            {
                if (Recv_Player_Packet->idx_player + 1 > nPlayer) {
                    Player_Live[nPlayer++] = TRUE;
                }
            }
            else if (Recv_Player_Packet->type == PacketType::item)
            {
                Itemset[0][Recv_Player_Packet->x][Recv_Player_Packet->y] = Recv_Player_Packet->idx_player;
            }
        }
        else if (GameState == INGAME)
        {
            retval = recvn(sock, buf, sizeof(InputPacket), 0);
            if (retval == SOCKET_ERROR)
                continue;
            buf[retval] = '\0';
            Recv_Player_Packet = (InputPacket*)buf;
            printf("%d 타입 패킷 수신\n", Recv_Player_Packet->type);
            if (Recv_Player_Packet->type == PacketType:: player)
            {
                printf("플레이어 패킷 수신 -> type : %d, idx : %d, x : %d, y : %d, status : %d\n\n", Recv_Player_Packet->type, Recv_Player_Packet->idx_player, Recv_Player_Packet->x, Recv_Player_Packet->y, Recv_Player_Packet->status);
                if (Recv_Player_Packet->status ==Status::IN_BUBBLE) {
                    Player_Speed[Recv_Player_Packet->idx_player] = 100;
                    SetTimer(hwnd, Timer::In_Bubble, Player_Speed[Recv_Player_Packet->idx_player], (TIMERPROC)TimeProc_InBubble);
                    bInBubble[Recv_Player_Packet->idx_player] = TRUE;
                }
                else if (Recv_Player_Packet->status == Status::Dead)
                {
                    bInBubble[Recv_Player_Packet->idx_player] = FALSE;
                    bDie[Recv_Player_Packet->idx_player] = TRUE;
                    BubbleResource[Recv_Player_Packet->idx_player] = 3;
                    BubbleCount[Recv_Player_Packet->idx_player] = 0;
                    CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Character_Die.ogg");
                    SetTimer(hwnd, Die, 150, (TIMERPROC)TimeProc_Die);
                }
                else {
                    Player[Recv_Player_Packet->idx_player].left = Recv_Player_Packet->x;
                    Player[Recv_Player_Packet->idx_player].right = Player[Client_Idx].left + Player_CX;
                    Player[Recv_Player_Packet->idx_player].top = Recv_Player_Packet->y;
                    Player[Recv_Player_Packet->idx_player].bottom = Recv_Player_Packet->y + Player_CY;
                    if (Recv_Player_Packet->status == Status::STOP)
                    {
                        xPos_Player[Recv_Player_Packet->idx_player] = 0;
                        Player_Move[Recv_Player_Packet->idx_player] = FALSE;
                    }
                    else if (yPos_Player[Recv_Player_Packet->idx_player] != Recv_Player_Packet->status)
                    {
                        yPos_Player[Recv_Player_Packet->idx_player] = Recv_Player_Packet->status;
                        xPos_Player[Recv_Player_Packet->idx_player] = 0;
                        Player_Move[Recv_Player_Packet->idx_player] = TRUE;
                    }                    
                }
            }
            else if (Recv_Player_Packet->type == PacketType::bubble)
            {
                printf("버블 패킷 수신 -> type : %d x : %d y : %d\n\n", Recv_Player_Packet->type, Recv_Player_Packet->x, Recv_Player_Packet->y);
                for (int i = 0; i < Player_bCount[Recv_Player_Packet->idx_player]; i++)
                {
                    if (Player_Bubble[Recv_Player_Packet->idx_player][i] == TRUE || Player_Bubble_Flow[Recv_Player_Packet->idx_player][i] == TRUE)
                    {
                        continue;
                    }
                    else
                    {
                        if (Tile_Bubble[Recv_Player_Packet->idx_player][i - 1].left != Recv_Player_Packet->x || Tile_Bubble[Recv_Player_Packet->idx_player][i - 1].top != Recv_Player_Packet->y)
                        {
                            Tile_Bubble[Recv_Player_Packet->idx_player][i].left = Recv_Player_Packet->x;
                            Tile_Bubble[Recv_Player_Packet->idx_player][i].right = Tile_Bubble[Recv_Player_Packet->idx_player][i].left + 40;
                            Tile_Bubble[Recv_Player_Packet->idx_player][i].top = Recv_Player_Packet->y;
                            Tile_Bubble[Recv_Player_Packet->idx_player][i].bottom = Tile_Bubble[Recv_Player_Packet->idx_player][i].top + 40;
                            Player_Bubble[Recv_Player_Packet->idx_player][i] = TRUE;
                            CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Bubble_On.ogg");
                        }
                        SetEvent(hBubbleEvent);
                        break;
                    }
                }
                // 데이터 받은거 처리 부분 구현 필요
                // 지금은 버블 생성하고 패킷 보내는 형식으로 진행되는데 이걸 보낸 뒤에 버블 패킷 받고 생성하는걸로 수정 필요
            }
            else if (Recv_Player_Packet->type == PacketType::item)
            {
                if (Itemset[Sel_Map][Recv_Player_Packet->x][Recv_Player_Packet->y] == Ball) {
                    CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Item_Off.ogg");
                    if (Player_bCount[Recv_Player_Packet->idx_player] < 7)
                        Player_bCount[Recv_Player_Packet->idx_player]++;
                }
                if (Itemset[Sel_Map][Recv_Player_Packet->x][Recv_Player_Packet->y] == OnePower) {
                    CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Item_Off.ogg");
                    if (Power[Recv_Player_Packet->idx_player] < 7)
                        Power[Recv_Player_Packet->idx_player]++;
                }
                if (Itemset[Sel_Map][Recv_Player_Packet->x][Recv_Player_Packet->y] == Speed) {
                    CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Item_Off.ogg");
                    if (Player_Speed[Recv_Player_Packet->idx_player] >= 20)
                        Player_Speed[Recv_Player_Packet->idx_player] -= 5;
                    if (Recv_Player_Packet->idx_player == Client_Idx) {
                        KillTimer(hwnd, P1);
                        SetTimer(hwnd, P1, Player_Speed[Client_Idx], (TIMERPROC)TimeProc_P1_Move);
                    }
                }
                if (Itemset[Sel_Map][Recv_Player_Packet->x][Recv_Player_Packet->y] == MaxPower) {
                    CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Item_Off.ogg");
                    Power[Recv_Player_Packet->idx_player] = 7;
                }
                if (Itemset[Sel_Map][Recv_Player_Packet->x][Recv_Player_Packet->y] == RedDevil) {
                    CSoundMgr::GetInstance()->PlayEffectSound(L"SFX_Item_Off.ogg");
                    Player_Speed[Recv_Player_Packet->idx_player] = 15;
                    if (Recv_Player_Packet->idx_player == Client_Idx) {
                        KillTimer(hwnd, P1);
                        SetTimer(hwnd, P1, Player_Speed[Client_Idx], (TIMERPROC)TimeProc_P1_Move);
                    }
                    Player_bCount[Recv_Player_Packet->idx_player] = 7;
                    Power[Recv_Player_Packet->idx_player] = 7;
                }
                Itemset[Sel_Map][Recv_Player_Packet->x][Recv_Player_Packet->y] = 0;
            }
            else if (Recv_Player_Packet->type == PacketType::end)
            {
                Ending = true;
                GameState = ROBBY;
                TextOn = FALSE;
                bSceneChange = true;
                for (int i = 0; i < MAX_PLAYER; ++i) {
                    Player_Speed[i] = 35;
                    Player_bCount[i] = 1;
                    Power[i] = 1;
                    bDie[i] = FALSE;
                    bInBubble[i] = FALSE;
                    BubbleCount[i] = 0;
                    BubbleResource[i] = 0;
                    xPos_Player[i] = 0;
                    yPos_Player[i] = 1;
                    Player_Dying[i] = 0;
                    Player_Remove[i] = FALSE;
                    Player_Move[i] = FALSE;
                    for (int j = 0; j < 7; j++)
                    {
                        MoveResource[i][j] = 0;
                        Player_Bubble[i][j] = FALSE;
                        Player_Bubble_Flow[i][j] = FALSE;
                        Player_Bubble_Boom[i][j] = FALSE;
                        Bubble_cnt[i][j] = 0;
                        Tile_Bubble[i][j] = { 0,0,0,0 };
                    }
                }
                for (int i = 0; i < 13; i++)
                {
                    for (int j = 0; j < 15; j++)
                    {
                        Box_Break[i][j] = FALSE;
                        Itemset[0][i][j] = 0;
                    }
                }
                KillTimer(hwnd, In_Bubble);
                KillTimer(hwnd, Die);
                KillTimer(hwnd, Bubble_Flow);
                SetPos();
            }
        }
    }
    // closesocket()
    closesocket(sock);

    // 윈속 종료
    WSACleanup();
    return 0;
}


// 센드를 수행할 스레드
DWORD WINAPI SendClient(LPVOID arg)
{
    WaitForSingleObject(hConnectEvent, INFINITE);   // RecvClient에서 Connect 될 때까지 wait

    if (GameState == 2)
    {
        while (1)
        {
            // 처음엔 로비 화면에서 클릭에 따라 전송, game state 가 ingame이면 break하는 형태
            WaitForSingleObject(hInputEvent, INFINITE);
            printf("플레이어 패킷 송신 -> type : %d idx : %d, x : %d, y : %d, status : %d\n", Send_Client_Packet->type, Send_Client_Packet->idx_player, Send_Client_Packet->x, Send_Client_Packet->y, Send_Client_Packet->status);
            send(sock, (char*)Send_Client_Packet, sizeof(InputPacket), 0);
            delete Send_Client_Packet;
            Send_Client_Packet = NULL;
            SetEvent(hSendEvent);
            if (GameState == 3)
            {
                break;
            }
        }
    }
    // ingame 진입하고 난 이후에는 입력에 따라 데이터 송신
    if (GameState == 3)
    {
        while (1)
        {
            WaitForSingleObject(hInputEvent, INFINITE);
            printf("플레이어 패킷 송신 -> type : %d idx : %d, x : %d, y : %d, status : %d\n", Send_Client_Packet->type, Send_Client_Packet->idx_player, Send_Client_Packet->x, Send_Client_Packet->y, Send_Client_Packet->status);
            send(sock, (char*)Send_Client_Packet, sizeof(InputPacket), 0);
            delete Send_Client_Packet;
            Send_Client_Packet = NULL;
            SetEvent(hSendEvent);
        }
    }
    return 0;
}