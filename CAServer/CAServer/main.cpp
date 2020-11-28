#define _WINSOCK_DEPRECATED_NO_WARNINGS // �ֽ� VC++ ������ �� ��� ����
#include "stdafx.h"
#include "ServerFunc.h"
#include "CClientPacket.h"


int client_ID[3] = { 1, 2, 3 };
BOOL ThreadOn[3] = { FALSE }; // ������ ���� Ȯ��

int Thread_Count;

HANDLE hSendEvent;
HANDLE hRecvEvent;


InputPacket Send_P;
InputPacket Recv_P;

CMap m_Map;

CRITICAL_SECTION cs1, cs2;

SocketFunc m_SF;
PacketFunc m_PF;

int step = Accept; // ���� �帧
int SendPacket_Idx = 0;
int count = 0;


DWORD WINAPI SendThreadFunc(LPVOID arg)
{
    SOCKET client_sock = (SOCKET)arg;
    int retval;
    SOCKADDR_IN clientaddr;
    int addrlen;
    char buf[BUFSIZE + 1];

    int len;

    int Thread_idx = Thread_Count - 1;

    int startsign = 0;

    // Ŭ���̾�Ʈ ���� ���
    addrlen = sizeof(clientaddr);
    getpeername(client_sock, (SOCKADDR*)&clientaddr, &addrlen);

    m_PF.InitPacket(&Send_P);
    m_PF.InitPacket(&Recv_P);

    while (1) {
        // Ŭ���̾�Ʈ�� ������ ���
        if (step == Accept)
        {
            if (ThreadOn[Thread_idx])
            {
                WaitForSingleObject(hSendEvent, INFINITE);
                m_PF.InitPlayer(m_Map, &Send_P, Thread_idx);
                retval = send(client_sock, (char*)&Send_P, sizeof(InputPacket), 0);
                if (retval == SOCKET_ERROR) {
                    m_SF.err_display("send()");
                    break;
                }

                printf("[TCP ����] %d�� Ŭ���̾�Ʈ ��ġ ���� : %d %d\n",
                    client_ID[Thread_idx], Send_P.x, Send_P.y);
                EnterCriticalSection(&cs1);
                step = Robby;
                LeaveCriticalSection(&cs1);
                SetEvent(hRecvEvent);
            }
        }
        else if (step == Robby)
        {
            //printf("��ȣ�����...\n");
        }
        else if (step == InGame)
        {

            WaitForSingleObject(hSendEvent, INFINITE);
            EnterCriticalSection(&cs2);
            retval = send(client_sock, (char*)&Send_P, sizeof(InputPacket), 0);
            if (retval == SOCKET_ERROR) {
                m_SF.err_display("send()");
                break;
            }
            printf("Send [%d] S->C: type = %d %d %d\n", ntohs(clientaddr.sin_port),
                Send_P.type, Send_P.x, Send_P.y);
            m_PF.InitPacket(&Send_P);
            LeaveCriticalSection(&cs2);
            SetEvent(hRecvEvent);
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

    int Thread_idx = Thread_Count - 1;

    int recv_ClientID = 0; // �����͸� ������ ������ Ŭ���� ID ����

    int startsign = 0;

    // Ŭ���̾�Ʈ ���� ���
    addrlen = sizeof(clientaddr);
    getpeername(client_sock, (SOCKADDR*)&clientaddr, &addrlen);

    while (1) {
        // Ŭ���̾�Ʈ�� ������ ���

        if (step == Robby)
        {
            WaitForSingleObject(hRecvEvent, INFINITE);
            retval = m_SF.recvn(client_sock, (char*)&Recv_P, sizeof(Recv_P), 0);
            if (retval == SOCKET_ERROR) {
                m_SF.err_display("recv()");
                break;
            }
            printf("Recv [%d] S<-C: %d = ���� ��ȣ\n", ntohs(clientaddr.sin_port), Recv_P.type);
            if (Recv_P.type == ready)
            {
                EnterCriticalSection(&cs1);
                step = InGame;
                m_PF.InitPacket(&Recv_P);
                LeaveCriticalSection(&cs1);
                SetEvent(hRecvEvent);
            }
        }
        if (step == InGame)
        {
            WaitForSingleObject(hRecvEvent, INFINITE);
            EnterCriticalSection(&cs2);
            retval = m_SF.recvn(client_sock, (char*)&Recv_P, sizeof(InputPacket), 0);
            if (retval == SOCKET_ERROR) {
                m_SF.err_display("recv()");
                break;
            }
            printf("Recv [%d] S<-C: %d\n", ntohs(clientaddr.sin_port), Recv_P.x);
            Send_P = Recv_P;
            m_PF.InitPacket(&Recv_P);
            LeaveCriticalSection(&cs2);
            printf("����\n");
            SetEvent(hSendEvent);
        }

    }
    // closesocket()
    closesocket(client_sock);
    printf("[TCP ����] %d��° Ŭ���̾�Ʈ ����: IP �ּ�=%s, ID=%d\n",
        client_ID[Thread_idx], inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
    ThreadOn[Thread_idx] = FALSE;
    Thread_Count = Thread_idx - 1;

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

    hSendEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
    hRecvEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

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
                ThreadOn[Thread_Count] = TRUE;
                Thread_Count++;
                // ������ Ŭ���̾�Ʈ ���� ���
                printf("\n[TCP ����] %d�� Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
                    Thread_Count, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
                m_Map.Init_Map();

                // ������ ����
                hSendThread[Thread_Count - 1] = CreateThread(NULL, 0, SendThreadFunc,
                    (LPVOID)client_sock, 0, NULL);
                hRecvThread[Thread_Count - 1] = CreateThread(NULL, 0, RecvThreadFunc,
                    (LPVOID)client_sock, 0, NULL);
                if (hRecvThread[Thread_Count - 1] == NULL && hSendThread[Thread_Count - 1] == NULL) {
                    closesocket(client_sock);
                }
                else {
                    CloseHandle(hRecvThread[Thread_Count - 1]);
                    CloseHandle(hSendThread[Thread_Count - 1]);
                }

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