#pragma once
#include "stdafx.h"

#define SERVERIP   "25.10.9.113"
#define SERVERPORT 9000
#define BUFSIZE	   4096


// ���� ��� ������ �Լ�
DWORD WINAPI SendClient(LPVOID arg);
DWORD WINAPI RecvClient(LPVOID arg);

// ����� ���� ������ ���� �Լ�
int recvn(SOCKET s, char* buf, int len, int flags);

// Receive�� ������ ������. 
DWORD WINAPI RecvClient(LPVOID arg);

// ���带 ������ ������
DWORD WINAPI SendClient(LPVOID arg);