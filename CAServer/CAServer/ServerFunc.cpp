#pragma once
#pragma warning(disable : 4996)
#include "ServerFunc.h"
#include "CClientPacket.h"

// ���� ���� �߰�
BOOL SocketFunc::AddSocketInfo(SOCKET sock)
{
    if (nTotalSockets >= FD_SETSIZE) {
        printf("[����] ���� ������ �߰��� �� �����ϴ�!\n");
        return FALSE;
    }

    SOCKETINFO* ptr = new SOCKETINFO;
    if (ptr == NULL) {
        printf("[����] �޸𸮰� �����մϴ�!\n");
        return FALSE;
    }

    ptr->sock = sock;
    ptr->recvbytes = 0;
    ptr->sendbytes = 0;
    SocketInfoArray[nTotalSockets++] = ptr;

    return TRUE;
}

// ���� ���� ����
void SocketFunc::RemoveSocketInfo(int nIndex)
{
    SOCKETINFO* ptr = SocketInfoArray[nIndex];

    // Ŭ���̾�Ʈ ���� ���
    SOCKADDR_IN clientaddr;
    int addrlen = sizeof(clientaddr);
    getpeername(ptr->sock, (SOCKADDR*)&clientaddr, &addrlen);
    printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
        inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

    closesocket(ptr->sock);
    delete ptr;

    if (nIndex != (nTotalSockets - 1))
        SocketInfoArray[nIndex] = SocketInfoArray[nTotalSockets - 1];

    --nTotalSockets;
}

// ���� �Լ� ���� ��� �� ����
void SocketFunc::err_quit(char* msg)
{
    LPVOID lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
    MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
    LocalFree(lpMsgBuf);
    exit(1);
}

// ���� �Լ� ���� ���
void SocketFunc::err_display(char* msg)
{
    LPVOID lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
    printf("[%s] %s", msg, (char*)lpMsgBuf);
    LocalFree(lpMsgBuf);
}

// ����� ���� ������ ���� �Լ�
int SocketFunc::recvn(SOCKET s, char* buf, int len, int flags)
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
