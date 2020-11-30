#define _WINSOCK_DEPRECATED_NO_WARNINGS // �ֽ� VC++ ������ �� ��� ����
#include "stdafx.h"
#include "ServerFunc.h"
#include "CClientPacket.h"


int client_ID[3] = { 1, 2, 3 };
BOOL ThreadOn[3] = { FALSE }; // ������ ���� Ȯ��

BOOL InitPosition[3] = { FALSE }; // ���� ��ġ �ʱ�ȭ Ȯ��

int Thread_Count = -1;

HANDLE hSendEvent[3];
HANDLE hRecvEvent;


InputPacket Send_P;
InputPacket Recv_P;

InputPacket Player_P[3];

CMap m_Map;

CRITICAL_SECTION cs1, cs2;

SocketFunc m_SF;
PacketFunc m_PF;

int GameState = Robby; // ���� �帧
int SendPacket_Idx = 0;
int count = 0;
int Accept_count = 0; // Ŭ���̾�Ʈ�� ������ ������ Ƚ��


DWORD WINAPI SendThreadFunc(LPVOID arg)
{
    SOCKET client_sock = (SOCKET)arg;
    int retval;
    SOCKADDR_IN clientaddr;
    int addrlen;
    char buf[BUFSIZE + 1];

    int len;

    int Thread_idx = Thread_Count; // 0, 1, 2

    int startsign = 0;

    // Ŭ���̾�Ʈ ���� ���
    addrlen = sizeof(clientaddr);
    getpeername(client_sock, (SOCKADDR*)&clientaddr, &addrlen);

    m_PF.InitPacket(&Send_P);
    m_PF.InitPacket(&Recv_P);
    for (int i = 0;i<3;i++)
        m_PF.InitPacket(&Player_P[i]);

    while (1) {
        if (GameState == Robby)
        {
            if (!InitPosition[Thread_idx])
            {
                Accept_count++;
                for (int i = 0; i < 3; i++)
                {
                    if (Thread_idx != i)
                        SetEvent(hSendEvent[i]);
                }
                SetEvent(hSendEvent[Thread_idx]);
                EnterCriticalSection(&cs2);
                m_PF.InitPlayer(m_Map, &Player_P[Thread_idx], Thread_idx);
                retval = send(client_sock, (char*)&Player_P[Thread_idx], sizeof(InputPacket), 0);
                if (retval == SOCKET_ERROR) {
                    m_SF.err_display("send()");
                    break;
                }
                printf("[TCP ����] %d�� Ŭ���̾�Ʈ ��ġ ���� : %d %d %d\n",
                    client_ID[Thread_idx], Player_P[Thread_idx].x
                    , Player_P[Thread_idx].y, Player_P[Thread_idx].type);
                InitPosition[Thread_idx] = TRUE;
                for (int i = 0; i < 3; i++)
                {
                    if (Thread_Count != i)
                        ResetEvent(hSendEvent[i]);
                }
                LeaveCriticalSection(&cs2);
            }
            if (Accept_count - Thread_Count < 1) {
                if (ThreadOn[Thread_Count])
                {
                    WaitForSingleObject(hSendEvent[Thread_Count], INFINITE);
                    EnterCriticalSection(&cs2);
                    m_PF.InitPlayer(m_Map, &Send_P, Thread_Count);
                    retval = send(client_sock, (char*)&Send_P, sizeof(InputPacket), 0);
                    if (retval == SOCKET_ERROR) {
                        m_SF.err_display("send()");
                        break;
                    }
                    printf("Send %d������ %d�� Ŭ���̾�Ʈ�� ��ġ ���� : %d %d\n",
                        client_ID[Thread_idx], Send_P.idx_player+1, Send_P.x, Send_P.y);
                    InitPosition[Thread_idx] = TRUE;
                    LeaveCriticalSection(&cs2);
                }
            }
            if (Recv_P.type == ready)
            {
                if (InitPosition[0] || InitPosition[1])
                {
                    EnterCriticalSection(&cs1);;
                    m_PF.InitPacket(&Send_P);
                    Send_P.type = start;
                    LeaveCriticalSection(&cs1);
                    retval = send(client_sock, (char*)&Send_P, sizeof(InputPacket), 0);
                    if (retval == SOCKET_ERROR) {
                        m_SF.err_display("send()");
                        break;
                    }
                    printf("Send %d�� ���� ���� ��ȣ ����\n", client_ID[Thread_idx]);
                    SetEvent(hRecvEvent);
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

        if (GameState == Robby)
        {
            //WaitForSingleObject(hRecvEvent, INFINITE);
            retval = m_SF.recvn(client_sock, (char*)&Recv_P, sizeof(InputPacket), 0);
            if (retval == SOCKET_ERROR) {
                m_SF.err_display("recv()");
                break;
            }
            printf("Recv %d��: [%d] S<-C: %d = ���� ��ȣ\n", client_ID[Thread_idx], ntohs(clientaddr.sin_port), Recv_P.type);
        }
        if (GameState == InGame)
        {
            //WaitForSingleObject(hRecvEvent, INFINITE);
            EnterCriticalSection(&cs2);
            retval = m_SF.recvn(client_sock, (char*)&Recv_P, sizeof(InputPacket), 0);
            if (retval == SOCKET_ERROR) {
                m_SF.err_display("recv()");
                break;
            }
            printf("Recv %d��:[%d] S<-C: type = %d %d %d\n", client_ID[Thread_idx], ntohs(clientaddr.sin_port),
                Recv_P.type, Recv_P.x, Recv_P.y);
            m_PF.InitPacket(&Send_P);
            Send_P = Recv_P;
            m_PF.InitPacket(&Recv_P);
            LeaveCriticalSection(&cs2);
            for (int i = 0; i < 3; i++)
                SetEvent(hSendEvent[i]);
            /*if (!ThreadOn[0] || !ThreadOn[1])
                break;*/
        }

    }
    // closesocket()
    closesocket(client_sock);
    printf("[TCP ����] %d��° Ŭ���̾�Ʈ ����: IP �ּ�=%s, ID=%d\n",
        client_ID[Thread_idx], inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
    ThreadOn[Thread_idx] = FALSE;
    Thread_Count = 0;

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
    retval = listen(listen_sock, SOMAXCONN);
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

    hSendEvent[0] = CreateEvent(NULL, TRUE, TRUE, NULL);
    hSendEvent[1] = CreateEvent(NULL, TRUE, TRUE, NULL);
    hSendEvent[2] = CreateEvent(NULL, TRUE, TRUE, NULL);
    hRecvEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    InitializeCriticalSection(&cs1);
    InitializeCriticalSection(&cs2);

    while (1) {
        if (Thread_Count < MAX_CLIENT)
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
                printf("\n[TCP ����] %d�� Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
                    Thread_Count+1, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
                // ������ Ŭ���̾�Ʈ ���� ���
                m_Map.Init_Map();

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
                ThreadOn[Thread_Count] = TRUE;

            }

        }
    }
    DeleteCriticalSection(&cs1);
    DeleteCriticalSection(&cs2);

    // closesocket()
    closesocket(listen_sock);

    // ���� ����
    WSACleanup();
    return 0;
}