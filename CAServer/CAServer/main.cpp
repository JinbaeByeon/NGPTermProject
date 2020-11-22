#define _WINSOCK_DEPRECATED_NO_WARNINGS // �ֽ� VC++ ������ �� ��� ����
#include "stdafx.h"
#include "ServerFunc.h"
#include "CClientPacket.h"


int client_ID[3] = { 0 };

int Thread_Count;

HANDLE hSendEvent;
HANDLE hRecvEvent;

Packet p;
Packet temp;

CRITICAL_SECTION cs;

SocketFunc m_SF;


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

    u_long on = 1;
    retval = ioctlsocket(listen_sock, FIONBIO, &on);
    if (retval == SOCKET_ERROR) m_SF.err_display("ioctlsocket()");

    // ������ ��ſ� ����� ����
    FD_SET rset, wset;
    SOCKET client_sock;
    SOCKADDR_IN clientaddr;
    int addrlen, i, j;
    HANDLE hThread[3];
    int step = 0;
    int startsign = 0;

    int recv_ClientID;

    hSendEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    hRecvEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    InitializeCriticalSection(&cs);

    while (1) {
        // ���� �� �ʱ�ȭ
        FD_ZERO(&rset);
        FD_ZERO(&wset);
        FD_SET(listen_sock, &rset);
        for (int i = 0; i < m_SF.nTotalSockets; i++) {
            FD_SET(m_SF.SocketInfoArray[i]->sock, &rset);
        }

        // select()
        retval = select(0, &rset, &wset, NULL, NULL);
        if (retval == SOCKET_ERROR) m_SF.err_quit("select()");

        // ���� �� �˻�(1): Ŭ���̾�Ʈ ���� ����
        if (FD_ISSET(listen_sock, &rset)) {
            addrlen = sizeof(clientaddr);
            client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
            if (client_sock == INVALID_SOCKET) {
                m_SF.err_display("accept()");
            }
            else {
                printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
                    inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
                // ���� ���� �߰�
                m_SF.AddSocketInfo(client_sock);
                if (m_SF.nTotalSockets>0)
                    client_ID[m_SF.nTotalSockets - 1] = m_SF.nTotalSockets;
                printf("%d\n", m_SF.nTotalSockets);

            }
            if (step == 0)

            {
                if (m_SF.nTotalSockets >= 2)
                    startsign = 1;
                for (i = 0; i < m_SF.nTotalSockets; i++) {
                    FD_SET(m_SF.SocketInfoArray[i]->sock, &wset);
                    SocketFunc::SOCKETINFO* ptr = m_SF.SocketInfoArray[i];
                    if (FD_ISSET(ptr->sock, &wset)) {
                        retval = send(ptr->sock, (char*)&startsign, sizeof(int), 0);
                        if (retval == SOCKET_ERROR) {
                            m_SF.err_display("send()");
                            m_SF.RemoveSocketInfo(i);
                            continue;
                        }
                    }
                }
                if (m_SF.nTotalSockets >= 2)
                {
                    printf("%d\n", m_SF.nTotalSockets);
                    step = 1;
                }
            }
        }
        // ���� �� �˻�(2): ������ ���
        if (step == 1) // �� Ŭ���̾�Ʈ�� ID ����
        {
            for (i = 0; i < m_SF.nTotalSockets; i++) {
                SocketFunc::SOCKETINFO* ptr = m_SF.SocketInfoArray[i];
                if (FD_ISSET(ptr->sock, &wset)) {
                    retval = send(ptr->sock, (char*)&client_ID[i], sizeof(int), 0);
                    if (retval == SOCKET_ERROR) {
                        m_SF.err_display("send()");
                        m_SF.RemoveSocketInfo(i);
                        continue;
                    }
                    else
                    {
                        printf("[TCP ����] %d�� Ŭ���̾�Ʈ ID ���� : %d\n",
                            client_ID[i], ntohs(clientaddr.sin_port));

                    }
                }
            }
            step = 2;
        }
        else if (step == 2) // ������ �ְ� ����
        {
            for (i = 0; i < m_SF.nTotalSockets; i++) {
                FD_SET(m_SF.SocketInfoArray[i]->sock, &rset);
                SocketFunc::SOCKETINFO* ptr = m_SF.SocketInfoArray[i];
                if (FD_ISSET(ptr->sock, &rset)) { 
                    // ������ �ޱ�
                    retval = m_SF.recvn(ptr->sock, (char*)&recv_ClientID, sizeof(recv_ClientID), 0);
                    if (retval == SOCKET_ERROR) {
                        m_SF.err_display("recv()");
                        m_SF.RemoveSocketInfo(i);
                        continue;
                    }
                    retval = m_SF.recvn(ptr->sock, (char*)&temp, sizeof(temp), 0);
                    if (retval == SOCKET_ERROR) {
                        m_SF.err_display("recv()");
                        m_SF.RemoveSocketInfo(i);
                        continue;
                    }
                    if (temp.type != 0)
                    {
                        EnterCriticalSection(&cs);
                        p = temp;
                        LeaveCriticalSection(&cs);
                    }
                    printf("ID: %d S<-C: %d\n", recv_ClientID, temp.type);


                    // ���� ������ ��� Ŭ���̾�Ʈ���� �����͸� ����!
                    for (j = 0; j < m_SF.nTotalSockets; j++) {
                        FD_SET(m_SF.SocketInfoArray[j]->sock, &wset);
                        SocketFunc::SOCKETINFO* ptr2 = m_SF.SocketInfoArray[j];
                        retval = send(ptr2->sock, (char*)&recv_ClientID, BUFSIZE, 0);
                        if (retval == SOCKET_ERROR) {
                            m_SF.err_display("send()");
                            m_SF.RemoveSocketInfo(j);
                            --j; // ���� �ε��� ����
                            continue;
                        }
                        retval = send(ptr2->sock, (char*)&p, BUFSIZE, 0);
                        if (retval == SOCKET_ERROR) {
                            m_SF.err_display("send()");
                            m_SF.RemoveSocketInfo(j);
                            --j; // ���� �ε��� ����
                            continue;
                        }
                        printf("ID: %d S->C: %d\n", client_ID[j], temp.type);

                    }
                    p.type = 0;
                }
            }
        }
        Sleep(1000);
    }
    DeleteCriticalSection(&cs);

    // closesocket()
    closesocket(listen_sock);


    // ���� ����
    WSACleanup();
    return 0;
}
