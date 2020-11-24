#pragma once
#include"stdafx.h"


class SocketFunc {
public:
    SocketFunc() {};
    ~SocketFunc() {};


    void err_quit(char* msg);
    void err_display(char* msg);

    int recvn(SOCKET s, char* buf, int len, int flags);

};