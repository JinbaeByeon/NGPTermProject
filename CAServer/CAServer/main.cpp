#define _WINSOCK_DEPRECATED_NO_WARNINGS // �ֽ� VC++ ������ �� ��� ����
#include "stdafx.h"
#include "ServerFunc.h"
#include "CClientPacket.h"


int client_ID[3] = { 1, 2, 3 };
int ThreadOn[3] = { 0 };

int Thread_Count;

HANDLE hSendEvent;
HANDLE hRecvEvent;


PlayerPacket Pp[3]; // �÷��̾� ���� ��Ŷ
BubblePacket Bp;    // ��ǳ�� ���� ��Ŷ
ClientPacket CP;
Packet P;

CMap m_Map;

CRITICAL_SECTION cs1, cs2;

SocketFunc m_SF;
PacketFunc m_PF;

int step = 0;
int SendPacket_Idx = 0;


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

    while (1) {
        // Ŭ���̾�Ʈ�� ������ ���
        if (step == 0)
        {
            if (ThreadOn[Thread_idx])
            {
                WaitForSingleObject(hSendEvent, INFINITY);
            
                Pp[Thread_idx].idx_player = client_ID[Thread_idx];
                if (client_ID[Thread_idx] == 1)
                    Pp[Thread_idx].left = m_Map.Tile[0][0].left, Pp[Thread_idx].top = m_Map.Tile[0][0].top
                    , Pp[Thread_idx].status = 1;
                retval = send(client_sock, (char*)&Pp[Thread_idx], sizeof(Pp[Thread_idx]), 0);
                if (retval == SOCKET_ERROR) {
                    m_SF.err_display("send()");
                    break;
                }
                printf("[TCP ����] %d�� Ŭ���̾�Ʈ ��ġ ���� : %d %d\n",
                    client_ID[Thread_idx], Pp[Thread_idx].left, Pp[Thread_idx].top);
                SetEvent(hRecvEvent);
                EnterCriticalSection(&cs1);
                step = 1;
                LeaveCriticalSection(&cs1);
            }
        }
        /*else if (step == 1)
        {
            if (ThreadOn[0] && ThreadOn[1] && ThreadOn[2])
            {
                startsign = 1;
                retval = send(client_sock, (char*)&startsign, sizeof(startsign), 0);
                if (retval == SOCKET_ERROR) {
                    m_SF.err_display("send()");
                    break;
                }
                break;
            }
            step = 2;
        }*/
        else if (step == 2)
        {
            /*printf("%d\n", CP);*/
            WaitForSingleObject(hSendEvent, INFINITY);
            EnterCriticalSection(&cs2);
            if (CP >= ClientPacket::input_left && CP <= ClientPacket::input_bottom)
            {
                Pp[SendPacket_Idx].type = 1;
                printf("������ type �� : %d\n", Pp[SendPacket_Idx].type);
                retval = send(client_sock, (char*)&Pp[SendPacket_Idx].type, sizeof(Pp[SendPacket_Idx].type), 0);
                if (retval == SOCKET_ERROR) {
                    m_SF.err_display("send()");
                    break;
                }
                printf("������ type �� : %d\n", Pp[SendPacket_Idx].type);
                retval = send(client_sock, (char*)&Pp[SendPacket_Idx], sizeof(Pp[SendPacket_Idx]), 0);
                if (retval == SOCKET_ERROR) {
                    m_SF.err_display("send()");
                    break;
                }
                printf("Send [%d] S->C: %d %d\n", ntohs(clientaddr.sin_port)
                    , Pp[SendPacket_Idx].left, Pp[SendPacket_Idx].top);
            }
            else if (CP == 16)
            {
                retval = send(client_sock, (char*)&Bp.type, sizeof(Bp.type), 0);
                if (retval == SOCKET_ERROR) {
                    m_SF.err_display("send()");
                    break;
                }
                retval = send(client_sock, (char*)&Bp, sizeof(Bp), 0);
                if (retval == SOCKET_ERROR) {
                    m_SF.err_display("send()");
                    break;
                }
                printf("Send [%d] %d: S->C: %d %d\n", ntohs(clientaddr.sin_port), SendPacket_Idx
                    , Bp.left, Bp.top);
            }
            CP = ClientPacket::empty;
            
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

        if (step == 1)
        {
            WaitForSingleObject(hRecvEvent, INFINITY);
            retval = m_SF.recvn(client_sock, (char*)&P, sizeof(P), 0);
            if (retval == SOCKET_ERROR) {
                m_SF.err_display("recv()");
                break;
            }
            printf("Recv [%d] S<-C: %d = ���� ��ȣ\n", ntohs(clientaddr.sin_port), P.type);
            EnterCriticalSection(&cs1);
            step = 2;
            LeaveCriticalSection(&cs1);
        }
        if (step == 2)
        {
            WaitForSingleObject(hRecvEvent, INFINITY);
            EnterCriticalSection(&cs1);
            retval = m_SF.recvn(client_sock, (char*)&CP, sizeof(CP), 0);
            if (retval == SOCKET_ERROR) {
                m_SF.err_display("recv()");
                break;
            }

            if (CP >= ClientPacket::input_left && CP <= ClientPacket::input_bottom)
            {
                m_PF.PlayerPacketProcess(m_Map,CP, &Pp[Thread_idx], client_ID[Thread_idx]);
            }
            else if (CP == ClientPacket::input_space)
            {
                m_PF.BubblePacketProcess(m_Map,CP, &Bp);
            }
            SendPacket_Idx = Thread_idx;
            printf("Recv [%d] S<-C: %d\n", ntohs(clientaddr.sin_port), CP);
            LeaveCriticalSection(&cs1);
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
    int step = 0;
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
