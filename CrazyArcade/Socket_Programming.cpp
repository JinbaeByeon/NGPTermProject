#include "Socket_Programming.h"
#include "Packet.h"

// 이벤트
extern HANDLE hRecvEvent, hSendEvent, hConnectEvent;
// 소켓
extern SOCKET sock;
// 패킷
extern PlayerPacket *MyPlayer_Packet;
extern BubblePacket *Bubble_Packet;
extern Packet client;

extern HWND hwnd;

// 게임 스테이트 enum GAME_BG { MENU = 1, ROBBY = 2, INGAME = 3};
extern int GameState;


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

    int retval;

    char buf[BUFSIZE];

    // 윈속 초기화
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;

    // socket()
    sock = socket(AF_INET, SOCK_STREAM, 0);

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
    retval = recvn(sock, buf, sizeof(PlayerPacket), 0);
    SetEvent(hConnectEvent);    //connect가 끝나면 송신에서도 변수 사용가능하다는 이벤트 발생시킨다.

    // 실험용
    char experiment[BUFSIZE];

    // 데이터 통신에 쓰일 while, 이 위에 처음 서버와 연결했을 때의 패킷을 받아오는 작업 필요
    while (1) {
        if (GameState == 2) // 게임 스테이트가 메뉴일 때
        {
            // 다른 플레이어가 접속한다면 받아온다.
            retval = recvn(sock, buf, sizeof(PlayerPacket), 0);
            buf[retval] = '\0';
            MyPlayer_Packet = (PlayerPacket*)buf;
            if (MyPlayer_Packet->idx_player == 3)
            {
                printf("Packet ID : %d\nPacket x : %d\nPacket y : %d\nPacket type : %d\n", MyPlayer_Packet->idx_player, MyPlayer_Packet->x, MyPlayer_Packet->y, MyPlayer_Packet->type);
            }
        }
        else if (GameState == 3)
        {
            retval = recvn(sock, buf, sizeof(PlayerPacket), 0);
            buf[retval] = '\0';
            MyPlayer_Packet = (PlayerPacket*)buf;
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
    // 처음엔 로비 화면에서 클릭에 따라 전송, game state 가 ingame이면 break하는 형태
    WaitForSingleObject(hSendEvent, INFINITE);
    printf("ClientPacket Send Value : %d", client);
    send(sock, (char*)&client, sizeof(Packet), 0);
    ResetEvent(hSendEvent);
    if (GameState == 2)
    {
        while (1)
        {
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
            WaitForSingleObject(hSendEvent, INFINITE);
            send(sock, (char*)&client, sizeof(Packet), 0);
            ResetEvent(hSendEvent);
        }
    }
    return 0;
}