#define _WINSOCK_DEPRECATED_NO_WARNINGS // 최신 VC++ 컴파일 시 경고 방지
#include "stdafx.h"
#include "ServerFunc.h"
#include "CClientPacket.h"


int client_ID[3] = { 1, 2, 3 };
BOOL ThreadOn[3] = { FALSE }; // 스레드 생성 확인

BOOL InitPosition[3] = { FALSE }; // 시작 위치 초기화 확인

int Thread_Count = -1;

HANDLE hSendEvent[3];
HANDLE hRecvEvent;


InputPacket Send_P;
InputPacket Recv_P;

InputPacket Player_P[3];

CMap m_Map;

CRITICAL_SECTION cs1, cs2;

SocketFunc m_SF;
PacketFunc m_PF;

int GameState = Robby; // 게임 흐름
int SendPacket_Idx = 0;
int count = 0;
int Accept_count = 0; // 클라이언트가 서버에 접속한 횟수


DWORD WINAPI SendThreadFunc(LPVOID arg)
{
    SOCKET client_sock = (SOCKET)arg;
    int retval;
    SOCKADDR_IN clientaddr;
    int addrlen;
    char buf[BUFSIZE + 1];

    int len;

    int Thread_idx = Thread_Count; // 0, 1, 2

    int startsign = 0;

    // 클라이언트 정보 얻기
    addrlen = sizeof(clientaddr);
    getpeername(client_sock, (SOCKADDR*)&clientaddr, &addrlen);

    m_PF.InitPacket(&Send_P);
    m_PF.InitPacket(&Recv_P);
    for (int i = 0;i<3;i++)
        m_PF.InitPacket(&Player_P[i]);

    while (1) {
        if (GameState == Robby)
        {
            if (!InitPosition[Thread_idx])
            {
                Accept_count++;
                for (int i = 0; i < 3; i++)
                {
                    if (Thread_idx != i)
                        SetEvent(hSendEvent[i]);
                }
                SetEvent(hSendEvent[Thread_idx]);
                EnterCriticalSection(&cs2);
                m_PF.InitPlayer(m_Map, &Player_P[Thread_idx], Thread_idx);
                retval = send(client_sock, (char*)&Player_P[Thread_idx], sizeof(InputPacket), 0);
                if (retval == SOCKET_ERROR) {
                    m_SF.err_display("send()");
                    break;
                }
                printf("[TCP 서버] %d번 클라이언트 위치 전송 : %d %d %d\n",
                    client_ID[Thread_idx], Player_P[Thread_idx].x
                    , Player_P[Thread_idx].y, Player_P[Thread_idx].type);
                InitPosition[Thread_idx] = TRUE;
                for (int i = 0; i < 3; i++)
                {
                    if (Thread_Count != i)
                        ResetEvent(hSendEvent[i]);
                }
                LeaveCriticalSection(&cs2);
            }
            if (Accept_count - Thread_Count < 1) {
                if (ThreadOn[Thread_Count])
                {
                    WaitForSingleObject(hSendEvent[Thread_Count], INFINITE);
                    EnterCriticalSection(&cs2);
                    m_PF.InitPlayer(m_Map, &Send_P, Thread_Count);
                    retval = send(client_sock, (char*)&Send_P, sizeof(InputPacket), 0);
                    if (retval == SOCKET_ERROR) {
                        m_SF.err_display("send()");
                        break;
                    }
                    printf("Send %d번에게 %d번 클라이언트의 위치 전송 : %d %d\n",
                        client_ID[Thread_idx], Send_P.idx_player+1, Send_P.x, Send_P.y);
                    InitPosition[Thread_idx] = TRUE;
                    LeaveCriticalSection(&cs2);
                }
            }
            if (Recv_P.type == ready)
            {
                if (InitPosition[0] || InitPosition[1])
                {
                    EnterCriticalSection(&cs1);;
                    m_PF.InitPacket(&Send_P);
                    Send_P.type = start;
                    LeaveCriticalSection(&cs1);
                    retval = send(client_sock, (char*)&Send_P, sizeof(InputPacket), 0);
                    if (retval == SOCKET_ERROR) {
                        m_SF.err_display("send()");
                        break;
                    }
                    printf("Send %d번 게임 시작 신호 보냄\n", client_ID[Thread_idx]);
                    SetEvent(hRecvEvent);
                    GameState = InGame;
                }
            }
        }
        if (GameState == InGame)
        {
            WaitForSingleObject(hSendEvent[Thread_idx], INFINITE);
            retval = send(client_sock, (char*)&Send_P, sizeof(InputPacket), 0);
            if (retval == SOCKET_ERROR) {
                m_SF.err_display("send()");
                break;
            }
            printf("Send %d번: [%d] S->C: type = %d %d %d\n", client_ID[Thread_idx], ntohs(clientaddr.sin_port),
                Send_P.type, Send_P.x, Send_P.y);
            ResetEvent(hSendEvent[Thread_idx]);
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

    int Thread_idx = Thread_Count;

    int recv_ClientID = 0; // 데이터를 서버로 보내는 클라의 ID 저장

    int startsign = 0;

    // 클라이언트 정보 얻기
    addrlen = sizeof(clientaddr);
    getpeername(client_sock, (SOCKADDR*)&clientaddr, &addrlen);

    while (1) {
        // 클라이언트와 데이터 통신

        if (GameState == Robby)
        {
            //WaitForSingleObject(hRecvEvent, INFINITE);
            retval = m_SF.recvn(client_sock, (char*)&Recv_P, sizeof(InputPacket), 0);
            if (retval == SOCKET_ERROR) {
                m_SF.err_display("recv()");
                break;
            }
            printf("Recv %d번: [%d] S<-C: %d = 시작 신호\n", client_ID[Thread_idx], ntohs(clientaddr.sin_port), Recv_P.type);
        }
        if (GameState == InGame)
        {
            //WaitForSingleObject(hRecvEvent, INFINITE);
            EnterCriticalSection(&cs2);
            retval = m_SF.recvn(client_sock, (char*)&Recv_P, sizeof(InputPacket), 0);
            if (retval == SOCKET_ERROR) {
                m_SF.err_display("recv()");
                break;
            }
            printf("Recv %d번:[%d] S<-C: type = %d %d %d\n", client_ID[Thread_idx], ntohs(clientaddr.sin_port),
                Recv_P.type, Recv_P.x, Recv_P.y);
            m_PF.InitPacket(&Send_P);
            Send_P = Recv_P;
            m_PF.InitPacket(&Recv_P);
            LeaveCriticalSection(&cs2);
            for (int i = 0; i < 3; i++)
                SetEvent(hSendEvent[i]);
            /*if (!ThreadOn[0] || !ThreadOn[1])
                break;*/
        }

    }
    // closesocket()
    closesocket(client_sock);
    printf("[TCP 서버] %d번째 클라이언트 종료: IP 주소=%s, ID=%d\n",
        client_ID[Thread_idx], inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
    ThreadOn[Thread_idx] = FALSE;
    Thread_Count = 0;

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

    int count = 0;

    hSendEvent[0] = CreateEvent(NULL, TRUE, TRUE, NULL);
    hSendEvent[1] = CreateEvent(NULL, TRUE, TRUE, NULL);
    hSendEvent[2] = CreateEvent(NULL, TRUE, TRUE, NULL);
    hRecvEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

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
                Thread_Count++;
                printf("\n[TCP 서버] %d번 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
                    Thread_Count+1, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
                // 접속한 클라이언트 정보 출력
                m_Map.Init_Map();

                // 스레드 생성
                hSendThread[Thread_Count] = CreateThread(NULL, 0, SendThreadFunc,
                    (LPVOID)client_sock, 0, NULL);
                hRecvThread[Thread_Count] = CreateThread(NULL, 0, RecvThreadFunc,
                    (LPVOID)client_sock, 0, NULL);
                if (hRecvThread[Thread_Count] == NULL && hSendThread[Thread_Count] == NULL) {
                    closesocket(client_sock);
                }
                else {
                    CloseHandle(hRecvThread[Thread_Count]);
                    CloseHandle(hSendThread[Thread_Count]);
                }
                ThreadOn[Thread_Count] = TRUE;

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