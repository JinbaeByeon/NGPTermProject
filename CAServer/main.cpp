#define _WINSOCK_DEPRECATED_NO_WARNINGS // �ֽ� VC++ ������ �� ��� ����
#include "stdafx.h"
#include "ServerFunc.h"
#include "CClientPacket.h"


int client_ID[4] = { 1, 2, 3, 4 };
BOOL ThreadOn[4] = { FALSE, }; // ������ ���� Ȯ��
BOOL InRobby[4] = { FALSE, }; // �κ� ���� Ȯ��
BOOL Ready[4] = { TRUE, TRUE, TRUE, TRUE }; // ���� ���� �غ� Ȯ��
BOOL ItemReady[4] = { TRUE,TRUE,TRUE, TRUE };  // ������ �ʱ�ȭ Ȯ��
BOOL Game_Over[4] = { TRUE, TRUE, TRUE, TRUE }; // �� ������ ���� ���� ó�� Ȯ��
 
int Thread_Count = -1; // send+recv������ �� ����

HANDLE hSendEvent[4];  // �����庰 send�̺�Ʈ
HANDLE hRecvEvent;


InputPacket Send_P;
InputPacket Recv_P;
InputPacket Player_P[4]; // �÷��̾� �ʱ� ����
InputPacket Item_P;

CMap m_Map;

CRITICAL_SECTION cs;

SocketFunc m_SF;
PacketFunc m_PF;

int GameState = Robby; // ���� �帧
int Death_count = 0; // ���� ���� ��
int Accept_count = 0; // Ŭ���̾�Ʈ�� ������ ������ Ƚ��
int ItemValue;

void InitGame()
{
    for (int i = 0; i <MAX_CLIENT; i++)
    {
        if (ThreadOn[i])
        {
            Ready[i] = FALSE;
            ItemReady[i] = FALSE;
            Game_Over[i] = FALSE;
        }
    }
    Death_count = 0;
    GameState = Robby;
}

DWORD WINAPI SendThreadFunc(LPVOID arg)
{
    SOCKET client_sock = (SOCKET)arg;
    int retval;
    SOCKADDR_IN clientaddr;
    int addrlen;
    char buf[BUFSIZE + 1];

    int len;

    int Thread_idx = Thread_Count; // 0, 1, 2
    Ready[Thread_idx] = FALSE;
    ItemReady[Thread_idx] = FALSE;
    Game_Over[Thread_idx] = FALSE;

    int startsign = 0;

    // Ŭ���̾�Ʈ ���� ���
    addrlen = sizeof(clientaddr);
    getpeername(client_sock, (SOCKADDR*)&clientaddr, &addrlen);

    m_PF.InitPacket(&Send_P);
    m_PF.InitPacket(&Recv_P);
    for (int i = 0; i < MAX_CLIENT; i++)
        m_PF.InitPacket(&Player_P[i]);

    while (1) {
        if (GameState == Robby)
        {
            // ���� �� �ʱ� ������ ����
            if (!InRobby[Thread_idx])
            {
                Accept_count++;
                EnterCriticalSection(&cs);
                m_PF.InitPlayer(m_Map, &Player_P[Thread_idx], Thread_idx);
                retval = send(client_sock, (char*)&Player_P[Thread_idx], sizeof(InputPacket), 0);
                if (retval == SOCKET_ERROR) {
                    m_SF.err_display("send()");
                    break;
                }
                printf("[TCP ����] %d�� Ŭ���̾�Ʈ �ʱ� ������ ���� : %d %d %d\n",
                    client_ID[Thread_idx], Player_P[Thread_idx].x
                    , Player_P[Thread_idx].y, Player_P[Thread_idx].type);
                InRobby[Thread_idx] = TRUE;

                LeaveCriticalSection(&cs);


            }

            // �ٸ� Ŭ���̾�Ʈ���� �ڽ��� ������ ����
            if (Accept_count - Thread_Count < 1) {
                if (ThreadOn[Thread_Count] && !InRobby[Thread_Count])
                {
                    EnterCriticalSection(&cs);
                    m_PF.InitPlayer(m_Map, &Send_P, Thread_Count);
                    retval = send(client_sock, (char*)&Send_P, sizeof(InputPacket), 0);
                    if (retval == SOCKET_ERROR) {
                        m_SF.err_display("send()");
                        break;
                    }
                    printf("Send %d������ %d�� Ŭ���̾�Ʈ�� ��ġ ���� : %d %d\n",
                        client_ID[Thread_idx], Send_P.idx_player + 1, Send_P.x, Send_P.y);
                    LeaveCriticalSection(&cs);
                }
            }

        }
        if (GameState == Gameready)
        {
            // ���� ���� �Ǵ�
            if (Ready[0] && Ready[1] && Ready[2] && Ready[3])
            {
                if (!ItemReady[Thread_idx])
                {
                    EnterCriticalSection(&cs);
                    srand((unsigned)time(NULL));
                    for (int i = 0; i < m_Map.Tile_CountY; i++)
                    {
                        for (int j = 0; j < m_Map.Tile_CountX; j++) {
                            ItemValue = rand() % 30;
                            if (ItemValue != 0 && ItemValue != 7 && m_Map.isBox[0][i][j]) {
                                Item_P.x = i;
                                Item_P.y = j;
                                Item_P.idx_player = ItemValue;
                                Item_P.type = PacketType::item;
                                retval = send(client_sock, (char*)&Item_P, sizeof(Item_P), 0);
                                if (retval == SOCKET_ERROR) {
                                    m_SF.err_display("send()");
                                    break;
                                }
                            }
                        }
                    }
                    LeaveCriticalSection(&cs);
                    printf("Send %d�� ������ ��ġ ����\n", client_ID[Thread_idx]);
                    ItemReady[Thread_idx] = TRUE;
                }
                if (ItemReady[0] && ItemReady[1] && ItemReady[2] && ItemReady[3])
                {
                    EnterCriticalSection(&cs);
                    m_PF.InitPacket(&Send_P);
                    Send_P.type = start;
                    LeaveCriticalSection(&cs);
                    retval = send(client_sock, (char*)&Send_P, sizeof(InputPacket), 0);
                    if (retval == SOCKET_ERROR) {
                        m_SF.err_display("send()");
                        break;
                    }
                    printf("Send %d�� ���� ���� ��ȣ ����\n", client_ID[Thread_idx]);
                    GameState = InGame;
                }
            }
        }
        if (GameState == InGame)
        {
            WaitForSingleObject(hSendEvent[Thread_idx], INFINITE);
            retval = send(client_sock, (char*)&Send_P, sizeof(InputPacket), 0);
            if (retval == SOCKET_ERROR) {
                m_SF.err_display("send()");
                break;
            }
            printf("Send %d��: [%d] S->C: type = %d %d %d\n", client_ID[Thread_idx], ntohs(clientaddr.sin_port),
                Send_P.type, Send_P.x, Send_P.y);
            ResetEvent(hSendEvent[Thread_idx]);
            if (Send_P.type == end)
            {
                Game_Over[Thread_idx] = TRUE;
                if (Game_Over[0] && Game_Over[1] && Game_Over[2] && Game_Over[3])
                {
                    GameState = GameOver;
                    printf("%d��: [%d] ���� ����\n"
                        , client_ID[Thread_idx], ntohs(clientaddr.sin_port));
                }
            }
        }
        if (GameState == GameOver)
        {
            InitGame();
        }

    }

    return 0;
}

DWORD WINAPI RecvThreadFunc(LPVOID arg)
{
    SOCKET client_sock = (SOCKET)arg;
    int retval;
    SOCKADDR_IN clientaddr;
    int addrlen;

    int Thread_idx = Thread_Count;


    int recv_ClientID = 0; // �����͸� ������ ������ Ŭ���� ID ����

    int startsign = 0;

    // Ŭ���̾�Ʈ ���� ���
    addrlen = sizeof(clientaddr);
    getpeername(client_sock, (SOCKADDR*)&clientaddr, &addrlen);


    while (1) {
        // Ŭ���̾�Ʈ�� ������ ���

        if (GameState == Robby || GameState == InGame)
        {
            retval = m_SF.recvn(client_sock, (char*)&Recv_P, sizeof(InputPacket), 0);
            if (retval == SOCKET_ERROR) {
                m_SF.err_display("recv()");
                break;
            }
            if (Recv_P.type == ready) {
                printf("Recv %d��: [%d] S<-C: %d = ���� ��ȣ\n", client_ID[Thread_idx], ntohs(clientaddr.sin_port), Recv_P.type);
                m_PF.InitPacket(&Recv_P);
                Ready[Thread_idx] = TRUE;
                GameState = Gameready;
                for (int i = 0; i < Accept_count; i++)
                    SetEvent(hSendEvent[i]);
            }
            else
            {
                printf("Recv %d��:[%d] S<-C: type = %d %d %d\n", client_ID[Thread_idx], ntohs(clientaddr.sin_port),
                    Recv_P.type, Recv_P.x, Recv_P.y);
                EnterCriticalSection(&cs);
                m_PF.InitPacket(&Send_P);
                Send_P = Recv_P;
                LeaveCriticalSection(&cs);
                for (int i = 0; i < Accept_count; i++)
                    SetEvent(hSendEvent[i]);
                if (Recv_P.status == Status::DEAD)
                {
                    Death_count++;
                    if (Thread_Count - Death_count <= 0)
                        Send_P.type = end;
                }
                m_PF.InitPacket(&Recv_P);
            }
        }

    }
    // closesocket()
    closesocket(client_sock);

    // �ʱ�ȭ
    m_PF.InitPacket(&Player_P[Thread_idx]);
    ThreadOn[Thread_idx] = FALSE;
    InRobby[Thread_idx] = FALSE;
    Thread_Count = Thread_idx - 1;
    Ready[Thread_idx] = TRUE;
    ItemReady[Thread_idx] = TRUE;
    Accept_count - 1;

    printf("[TCP ����] %d��° Ŭ���̾�Ʈ ����: IP �ּ�=%s, ID=%d\n",
        client_ID[Thread_idx], inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

    return 0;
}



int main(int argc, char* argv[])
{
    int retval;

    // ���� �ʱ�ȭ
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;

    // socket()
    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET) m_SF.err_quit("socket()");

    // bind()
    SOCKADDR_IN serveraddr;
    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SERVERPORT);
    retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) m_SF.err_quit("bind()");

    // listen()
    retval = listen(listen_sock, MAX_CLIENT);
    if (retval == SOCKET_ERROR) m_SF.err_quit("listen()");

    // ������ ��ſ� ����� ����
    SOCKET client_sock;
    SOCKADDR_IN clientaddr;
    int addrlen, i, j;
    HANDLE hRecvThread[3];
    HANDLE hSendThread[3];
    int startsign = 0;

    int recv_ClientID = 0;

    int count = 0;

    for (int i = 0; i < MAX_CLIENT;i++)
        hSendEvent[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
    hRecvEvent = CreateEvent(NULL, TRUE, TRUE, NULL);

    InitializeCriticalSection(&cs);

    while (1) {
        if (Thread_Count+1 < MAX_CLIENT)
        {
            // accept()
            addrlen = sizeof(clientaddr);
            client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
            if (client_sock == INVALID_SOCKET) {
                m_SF.err_display("accept()");
                break;
            }
            else
            {
                Thread_Count++;
                // ������ Ŭ���̾�Ʈ ���� ���
                printf("\n[TCP ����] %d�� Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
                    Thread_Count + 1, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

                m_Map.Init_Map();
                m_PF.InitPacket(&Recv_P);
                m_PF.InitPacket(&Send_P);
                ThreadOn[Thread_Count] = TRUE;

                // ������ ����
                hSendThread[Thread_Count] = CreateThread(NULL, 0, SendThreadFunc,
                    (LPVOID)client_sock, 0, NULL);
                hRecvThread[Thread_Count] = CreateThread(NULL, 0, RecvThreadFunc,
                    (LPVOID)client_sock, 0, NULL);
                if (hRecvThread[Thread_Count] == NULL && hSendThread[Thread_Count] == NULL) {
                    closesocket(client_sock);
                }
                else {
                    CloseHandle(hRecvThread[Thread_Count]);
                    CloseHandle(hSendThread[Thread_Count]);
                }

            }

        }
    }
    DeleteCriticalSection(&cs);

    // closesocket()
    closesocket(listen_sock);

    // ���� ����
    WSACleanup();
    return 0;
}