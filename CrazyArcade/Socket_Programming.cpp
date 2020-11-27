#include "Socket_Programming.h"
#include "Packet.h"
 
// 이벤트
extern HANDLE hRecvEvent, hSendEvent, hConnectEvent, hPlayerEvent, hBubbleEvent;
// 소켓
extern SOCKET sock;
// 패킷
extern PlayerPacket *Recv_Player_Packet;
extern BubblePacket *Recv_Bubble_Packet;
extern Packet* Send_Client_Packet;
Packet *Recv_Packet_Type;
// 불값
extern BOOL Bubble_Arrive, Player_Arrive;
//핸들값
extern HWND hwnd;
// 플레이어
extern RECT Player1, Player2;
// 게임 스테이트 enum GAME_BG { MENU = 1, ROBBY = 2, INGAME = 3};
extern int GameState;
extern const int Player_CX = 34;
extern const int Player_CY = 34;
extern int Client_Idx;


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
    buf[retval] = '\0';
    Recv_Player_Packet = (PlayerPacket*)buf;
    SetEvent(hConnectEvent);    //connect가 끝나면 송신에서도 변수 사용가능하다는 이벤트 발생시킨다.
    printf("야 위치 받았다. %d %d\n\n", Recv_Player_Packet->x, Recv_Player_Packet->y);
    printf("Packet ID : %d\nPacket x : %d\nPacket y : %d\nPacket type : %d\n", Recv_Player_Packet->idx_player, Recv_Player_Packet->x, Recv_Player_Packet->y, Recv_Player_Packet->type);
    Client_Idx = Recv_Player_Packet->idx_player;
    // 자기 자신의 위치 저장
    Player1.left = Recv_Player_Packet->x;
    Player1.top = Recv_Player_Packet->y;
    Player1.right = Player1.left + Player_CX;
    Player1.bottom = Player1.top + Player_CY;


    // 데이터 통신에 쓰일 while, 이 위에 처음 서버와 연결했을 때의 패킷을 받아오는 작업 필요
    while (1) {
        if (GameState == 2) // 게임 스테이트가 메뉴일 때
        {
            // 다른 플레이어가 접속한다면 받아온다.
            //retval = recvn(sock, buf, sizeof(PlayerPacket), 0);
            //buf[retval] = '\0';
            Recv_Player_Packet = (PlayerPacket*)buf;
        }
        else if (GameState == 3)
        {
            retval = recvn(sock, buf, sizeof(Packet), 0);
            buf[retval] = '\0';
            Recv_Packet_Type = (PlayerPacket*)buf;
            printf("%d 번 패킷 수신\n\n", Recv_Packet_Type->type);
            if (Recv_Packet_Type->type == 0 || Recv_Packet_Type->type == 1)
            {
                printf("여기까지는 넘어왔다\n");
                retval = recvn(sock, buf, sizeof(PlayerPacket), 0);
                buf[retval] = '\0';
                Recv_Player_Packet = (PlayerPacket*)buf;
                printf("받았다? %d, %d\n", Recv_Player_Packet->x, Recv_Player_Packet->y);
                SetEvent(hPlayerEvent);
                Player_Arrive = true;
            }
            else if (Recv_Packet_Type->type == 16)
            {
                retval = recvn(sock, buf, sizeof(BubblePacket), 0);
                buf[retval] = '\0';
                Recv_Bubble_Packet = (BubblePacket*)buf; 
                SetEvent(hBubbleEvent);
                printf("Power : %d\nX : %d Y : %d\n\n", Recv_Bubble_Packet->power, Recv_Bubble_Packet->x, Recv_Bubble_Packet->y);
                Bubble_Arrive = true;
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
    // 처음엔 로비 화면에서 클릭에 따라 전송, game state 가 ingame이면 break하는 형태
    WaitForSingleObject(hSendEvent, INFINITE);
    printf("ClientPacket Send Value : %d\n", Send_Client_Packet);
    send(sock, (char*)&Send_Client_Packet, sizeof(ClientPacket), 0);
    delete Send_Client_Packet;
    SetEvent(hSendEvent);
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
            printf("ClientPacket Send Value : %d\n", Send_Client_Packet);
            send(sock, (char*)&Send_Client_Packet, sizeof(ClientPacket), 0);
            delete Send_Client_Packet;
            SetEvent(hSendEvent);
        }
    }
    return 0;
}