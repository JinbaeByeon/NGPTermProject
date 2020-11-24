#define _WINSOCK_DEPRECATED_NO_WARNINGS // 최신 VC++ 컴파일 시 경고 방지
#include "stdafx.h"
#include "ServerFunc.h"
#include "CClientPacket.h"


int client_ID[3] = { 1, 2, 3 };
int ThreadOn[3] = { 0 };

int Thread_Count;

HANDLE hSendEvent;
HANDLE hRecvEvent;

PlayerPacket Pp;
BubblePacket Bp;

ClientPacket CP;

CRITICAL_SECTION cs;

SocketFunc m_SF;

DWORD WINAPI ThreadFunc(LPVOID arg)
{
    SOCKET client_sock = (SOCKET)arg;
    int retval;
    SOCKADDR_IN clientaddr;
    int addrlen;
    char buf[BUFSIZE + 1];

    int len;

    int Thread_idx = Thread_Count - 1;

    int recv_ClientID = 0; // 데이터를 서버로 보내는 클라의 ID 저장

    int step = 0;
    int startsign = 0;

    // 클라이언트 정보 얻기
    addrlen = sizeof(clientaddr);
    getpeername(client_sock, (SOCKADDR*)&clientaddr, &addrlen);

    while (1) {
        // 클라이언트와 데이터 통신
        if (step == 0)
        {
            if (ThreadOn[Thread_idx])
            {
                Pp.idx_player = client_ID[Thread_idx];
                if(client_ID[Thread_idx] == 1)
                    Pp.x = 3, Pp.y = 0, Pp.status = 1;
                retval = send(client_sock, (char*)&Pp, sizeof(Pp), 0);
                if (retval == SOCKET_ERROR) {
                    m_SF.err_display("send()");
                    break;
                }
                else
                {
                    printf("[TCP 서버] %d번 클라이언트 위치 전송 : %d %d\n",
                        client_ID[Thread_idx], Pp.x, Pp.y);
                    SetEvent(hRecvEvent);
                    step = 2;
                }
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
            WaitForSingleObject(hRecvEvent, INFINITY);
            retval = m_SF.recvn(client_sock, (char*)&CP, sizeof(CP), 0);
            if (retval == SOCKET_ERROR) {
                m_SF.err_display("recv()");
                break;
            }
            else
                SetEvent(hSendEvent);
            printf("Recv [%d] S<-C: %d\n", ntohs(clientaddr.sin_port),CP);

            WaitForSingleObject(hSendEvent, INFINITY);
            EnterCriticalSection(&cs);
            if (CP <= 8)
            {
                Pp.idx_player = client_ID[Thread_idx];
                if (CP == 1)
                    Pp.x -= 1;
                else if (CP == 2)
                    Pp.x += 1;
                else if (CP == 4)
                    Pp.y += 1;
                else if (CP == 8)
                    Pp.y -= 1;

                retval = send(client_sock, (char*)&Pp, sizeof(Pp), 0);
                if (retval == SOCKET_ERROR) {
                    m_SF.err_display("send()");
                    break;
                }
                else
                    SetEvent(hRecvEvent);
                printf("Send [%d] S->C: %d %d\n", ntohs(clientaddr.sin_port), Pp.x, Pp.y);
            }
            else if (CP == 16)
            {
                Bp.power = 1;
                Bp.x = 1;
                Bp.y = 2;

                retval = send(client_sock, (char*)&Bp, sizeof(Bp), 0);
                if (retval == SOCKET_ERROR) {
                    m_SF.err_display("send()");
                    break;
                }
                else
                    SetEvent(hRecvEvent);
                printf("Send [%d] S->C: %d %d\n", ntohs(clientaddr.sin_port), Bp.x, Bp.y);
            }
            LeaveCriticalSection(&cs);
        }

    }
    // closesocket()
    closesocket(client_sock);
    printf("[TCP 서버] %d번째 클라이언트 종료: IP 주소=%s, ID=%d\n",
        client_ID[Thread_idx], inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
    ThreadOn[Thread_idx] = FALSE;
    Thread_Count = Thread_idx - 1;

    return 0;
}


int main(int argc, char* argv[])
{
    int retval;

    // 윈속 초기화
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

    // 데이터 통신에 사용할 변수
    SOCKET client_sock;
    SOCKADDR_IN clientaddr;
    int addrlen, i, j;
    HANDLE hThread[3];
    int step = 0;
    int startsign = 0;

    int recv_ClientID = 0;

    hSendEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    hRecvEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    InitializeCriticalSection(&cs);

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

            }

            // 접속한 클라이언트 정보 출력
            printf("\n[TCP 서버] %d번 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
                Thread_Count, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

            // 스레드 생성
            hThread[Thread_Count - 1] = CreateThread(NULL, 0, ThreadFunc,
                (LPVOID)client_sock, 0, NULL);
            if (hThread[Thread_Count - 1] == NULL) { closesocket(client_sock); }
            else { CloseHandle(hThread[Thread_Count - 1]); }
        }
    }
    DeleteCriticalSection(&cs);

    // closesocket()
    closesocket(listen_sock);


    // 윈속 종료
    WSACleanup();
    return 0;
}
