#define _WINSOCK_DEPRECATED_NO_WARNINGS // 최신 VC++ 컴파일 시 경고 방지
#include "stdafx.h"
#include "ServerFunc.h"
#include "CClientPacket.h"


int client_ID[3] = { 1, 2, 3 };
BOOL ThreadOn[3] = { FALSE }; // 스레드 생성 확인

int Thread_Count;

HANDLE hSendEvent;
HANDLE hRecvEvent;


InputPacket Send_P;
InputPacket Recv_P;

CMap m_Map;

CRITICAL_SECTION cs1, cs2;

SocketFunc m_SF;
PacketFunc m_PF;

int step = Accept; // 게임 흐름
int SendPacket_Idx = 0;
int count = 0;


DWORD WINAPI SendThreadFunc(LPVOID arg)
{
    SOCKET client_sock = (SOCKET)arg;
    int retval;
    SOCKADDR_IN clientaddr;
    int addrlen;
    char buf[BUFSIZE + 1];

    int len;

    int Thread_idx = Thread_Count - 1;

    int startsign = 0;

    // 클라이언트 정보 얻기
    addrlen = sizeof(clientaddr);
    getpeername(client_sock, (SOCKADDR*)&clientaddr, &addrlen);

    m_PF.InitPacket(&Send_P);
    m_PF.InitPacket(&Recv_P);

    while (1) {
        // 클라이언트와 데이터 통신
        if (step == Accept)
        {
            if (ThreadOn[Thread_idx])
            {
                WaitForSingleObject(hSendEvent, INFINITE);
                m_PF.InitPlayer(m_Map, &Send_P, Thread_idx);
                retval = send(client_sock, (char*)&Send_P, sizeof(InputPacket), 0);
                if (retval == SOCKET_ERROR) {
                    m_SF.err_display("send()");
                    break;
                }

                printf("[TCP 서버] %d번 클라이언트 위치 전송 : %d %d\n",
                    client_ID[Thread_idx], Send_P.x, Send_P.y);
                EnterCriticalSection(&cs1);
                step = Robby;
                LeaveCriticalSection(&cs1);
                SetEvent(hRecvEvent);
            }
        }
        else if (step == Robby)
        {
            //printf("신호대기중...\n");
        }
        else if (step == InGame)
        {

            WaitForSingleObject(hSendEvent, INFINITE);
            EnterCriticalSection(&cs2);
            retval = send(client_sock, (char*)&Send_P, sizeof(InputPacket), 0);
            if (retval == SOCKET_ERROR) {
                m_SF.err_display("send()");
                break;
            }
            printf("Send [%d] S->C: type = %d %d %d\n", ntohs(clientaddr.sin_port),
                Send_P.type, Send_P.x, Send_P.y);
            m_PF.InitPacket(&Send_P);
            LeaveCriticalSection(&cs2);
            SetEvent(hRecvEvent);
        }

    }

    return 0;
}

DWORD WINAPI RecvThreadFunc(LPVOID arg)
{
    SOCKET client_sock = (SOCKET)arg;
    int retval;
    SOCKADDR_IN clientaddr;
    int addrlen;

    int Thread_idx = Thread_Count - 1;

    int recv_ClientID = 0; // 데이터를 서버로 보내는 클라의 ID 저장

    int startsign = 0;

    // 클라이언트 정보 얻기
    addrlen = sizeof(clientaddr);
    getpeername(client_sock, (SOCKADDR*)&clientaddr, &addrlen);

    while (1) {
        // 클라이언트와 데이터 통신

        if (step == Robby)
        {
            WaitForSingleObject(hRecvEvent, INFINITE);
            retval = m_SF.recvn(client_sock, (char*)&Recv_P, sizeof(Recv_P), 0);
            if (retval == SOCKET_ERROR) {
                m_SF.err_display("recv()");
                break;
            }
            printf("Recv [%d] S<-C: %d = 시작 신호\n", ntohs(clientaddr.sin_port), Recv_P.type);
            if (Recv_P.type == ready)
            {
                EnterCriticalSection(&cs1);
                step = InGame;
                m_PF.InitPacket(&Recv_P);
                LeaveCriticalSection(&cs1);
                SetEvent(hRecvEvent);
            }
        }
        if (step == InGame)
        {
            WaitForSingleObject(hRecvEvent, INFINITE);
            EnterCriticalSection(&cs2);
            retval = m_SF.recvn(client_sock, (char*)&Recv_P, sizeof(InputPacket), 0);
            if (retval == SOCKET_ERROR) {
                m_SF.err_display("recv()");
                break;
            }
            printf("Recv [%d] S<-C: %d\n", ntohs(clientaddr.sin_port), Recv_P.x);
            Send_P = Recv_P;
            m_PF.InitPacket(&Recv_P);
            LeaveCriticalSection(&cs2);
            printf("여기\n");
            SetEvent(hSendEvent);
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
    HANDLE hRecvThread[3];
    HANDLE hSendThread[3];
    int startsign = 0;

    int recv_ClientID = 0;

    hSendEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
    hRecvEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    InitializeCriticalSection(&cs1);
    InitializeCriticalSection(&cs2);

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
                // 접속한 클라이언트 정보 출력
                printf("\n[TCP 서버] %d번 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
                    Thread_Count, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
                m_Map.Init_Map();

                // 스레드 생성
                hSendThread[Thread_Count - 1] = CreateThread(NULL, 0, SendThreadFunc,
                    (LPVOID)client_sock, 0, NULL);
                hRecvThread[Thread_Count - 1] = CreateThread(NULL, 0, RecvThreadFunc,
                    (LPVOID)client_sock, 0, NULL);
                if (hRecvThread[Thread_Count - 1] == NULL && hSendThread[Thread_Count - 1] == NULL) {
                    closesocket(client_sock);
                }
                else {
                    CloseHandle(hRecvThread[Thread_Count - 1]);
                    CloseHandle(hSendThread[Thread_Count - 1]);
                }

            }

        }
    }
    DeleteCriticalSection(&cs1);
    DeleteCriticalSection(&cs2);

    // closesocket()
    closesocket(listen_sock);

    // 윈속 종료
    WSACleanup();
    return 0;
}