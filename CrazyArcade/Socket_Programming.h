#pragma once
#include "stdafx.h"

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 8000


// ���� ��� ������ �Լ�
DWORD WINAPI SendClient(LPVOID arg);
DWORD WINAPI RecvClient(LPVOID arg);

// �̺�Ʈ
HANDLE hRecvEvent, hPressEvent, hConnectEvent;

SOCKET sock;

// ����� ���� ������ ���� �Լ�
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

DWORD WINAPI RecvClient(LPVOID arg)
{
    int retval;

    // ���� �ʱ�ȭ
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

    // ������ ��ſ� ����� ����
    SOCKET client_sock;
    SOCKADDR_IN clientaddr;
    int addrlen;
    RECT rt;

    while (1) {

    }

    // closesocket()
    closesocket(client_sock);

    // ���� ����
    WSACleanup();
    return 0;
}

DWORD WINAPI SendClient(LPVOID arg)
{
    WaitForSingleObject(hConnectEvent, INFINITE);
}