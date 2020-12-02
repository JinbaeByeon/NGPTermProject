#pragma once
#include "stdafx.h"

#define SERVERIP   "25.10.9.113"
#define SERVERPORT 9000
#define BUFSIZE	   4096


// 소켓 통신 스레드 함수
DWORD WINAPI SendClient(LPVOID arg);
DWORD WINAPI RecvClient(LPVOID arg);

// 사용자 정의 데이터 수신 함수
int recvn(SOCKET s, char* buf, int len, int flags);

// Receive를 수행할 스레드. 
DWORD WINAPI RecvClient(LPVOID arg);

// 센드를 수행할 스레드
DWORD WINAPI SendClient(LPVOID arg);