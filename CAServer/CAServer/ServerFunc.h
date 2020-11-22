#pragma once
#include"stdafx.h"


class SocketFunc {
public:
    SocketFunc() {};
    ~SocketFunc() {};

    BOOL AddSocketInfo(SOCKET sock);
    void RemoveSocketInfo(int nIndex);

    void err_quit(char* msg);
    void err_display(char* msg);

    int recvn(SOCKET s, char* buf, int len, int flags);

    int nTotalSockets = 0;

    struct SOCKETINFO {
        SOCKET sock;
        char buf[BUFSIZE + 1];
        int recvbytes;
        int sendbytes;
    };

    SOCKETINFO* SocketInfoArray[FD_SETSIZE];
};

//struct SOCKETINFO {
//    SOCKET sock;
//    char buf[BUFSIZE + 1];
//    int recvbytes;
//    int sendbytes;
//};