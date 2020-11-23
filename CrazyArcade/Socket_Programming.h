#pragma once
#include "stdafx.h"

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 8000


// 소켓 통신 스레드 함수
DWORD WINAPI SendClient(LPVOID arg);
DWORD WINAPI RecvClient(LPVOID arg);

// 이벤트
HANDLE hRecvEvent, hPressEvent, hConnectEvent;
SOCKET sock;

BOOL Ready_For_Start = false, LEFT_PRESS, RIGHT_PRESS, UP_PRESS, DOWN_PRESS, 

// 사용자 정의 데이터 수신 함수
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
    int retval;

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
    
    SetEvent(hConnectEvent);    //connect가 끝나면 송신에서도 변수 사용가능하다는 이벤트 발생시킨다.

    // 데이터 통신에 쓰일 while, 이 위에 처음 서버와 연결했을 때의 패킷을 받아오는 작업 필요
    while (1) {

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
    if (GameState == ROBBY)
    {
        while (1)
        {
            if (GameState == INGAME)
            {
                break;
            }
        }
    }
    // ingame 진입하고 난 이후에는 입력에 따라 데이터 송신
    if (GameState == INGAME)
    {
        while (1)
        {

        }
    }
    return 0;
}