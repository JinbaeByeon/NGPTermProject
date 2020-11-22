#pragma once
#pragma warning(disable : 4996)
#include "ServerFunc.h"
#include "CClientPacket.h"

// 소켓 정보 추가
BOOL SocketFunc::AddSocketInfo(SOCKET sock)
{
    if (nTotalSockets >= FD_SETSIZE) {
        printf("[오류] 소켓 정보를 추가할 수 없습니다!\n");
        return FALSE;
    }

    SOCKETINFO* ptr = new SOCKETINFO;
    if (ptr == NULL) {
        printf("[오류] 메모리가 부족합니다!\n");
        return FALSE;
    }

    ptr->sock = sock;
    ptr->recvbytes = 0;
    ptr->sendbytes = 0;
    SocketInfoArray[nTotalSockets++] = ptr;

    return TRUE;
}

// 소켓 정보 삭제
void SocketFunc::RemoveSocketInfo(int nIndex)
{
    SOCKETINFO* ptr = SocketInfoArray[nIndex];

    // 클라이언트 정보 얻기
    SOCKADDR_IN clientaddr;
    int addrlen = sizeof(clientaddr);
    getpeername(ptr->sock, (SOCKADDR*)&clientaddr, &addrlen);
    printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",
        inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

    closesocket(ptr->sock);
    delete ptr;

    if (nIndex != (nTotalSockets - 1))
        SocketInfoArray[nIndex] = SocketInfoArray[nTotalSockets - 1];

    --nTotalSockets;
}

// 소켓 함수 오류 출력 후 종료
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

// 소켓 함수 오류 출력
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

// 사용자 정의 데이터 수신 함수
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
