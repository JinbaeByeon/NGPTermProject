#include "Socket_Programming.h"
#include "Packet.h"

// �̺�Ʈ
extern HANDLE hRecvEvent, hSendEvent, hConnectEvent;
// ����
extern SOCKET sock;
// ��Ŷ
extern PlayerPacket *Recv_Player_Packet;
extern BubblePacket *Recv_Bubble_Packet;
extern Packet Send_Client_Packet;
Packet *Recv_Packet_Type;

extern HWND hwnd;

// ���� ������Ʈ enum GAME_BG { MENU = 1, ROBBY = 2, INGAME = 3};
extern int GameState;


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

// Receive�� ������ ������. 
DWORD WINAPI RecvClient(LPVOID arg)
{
    // �޴� -> �κ�� ���� ������ ��ٸ���.
    WaitForSingleObject(hRecvEvent, INFINITE);
    ResetEvent(hRecvEvent);
    int retval;

    char buf[BUFSIZE];

    // ���� �ʱ�ȭ
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
    if (retval == SOCKET_ERROR)
        MessageBox(hwnd, L"���� �� ��", L"�����", MB_OK);
    else
        MessageBox(hwnd, L"�����", L"�����?", MB_OK);
    
    // Ŀ��Ʈ ���� �ڽ��� �÷��̾� ��Ŷ ����
    retval = recvn(sock, buf, sizeof(PlayerPacket), 0);
    SetEvent(hConnectEvent);    //connect�� ������ �۽ſ����� ���� ��밡���ϴٴ� �̺�Ʈ �߻���Ų��.

    // ������ ��ſ� ���� while, �� ���� ó�� ������ �������� ���� ��Ŷ�� �޾ƿ��� �۾� �ʿ�
    while (1) {
        if (GameState == 2) // ���� ������Ʈ�� �޴��� ��
        {
            // �ٸ� �÷��̾ �����Ѵٸ� �޾ƿ´�.
            retval = recvn(sock, buf, sizeof(PlayerPacket), 0);
            buf[retval] = '\0';
            Recv_Player_Packet = (PlayerPacket*)buf;
            if (Recv_Player_Packet->idx_player == 3)
            {
                printf("Packet ID : %d\nPacket x : %d\nPacket y : %d\nPacket type : %d\n", Recv_Player_Packet->idx_player, Recv_Player_Packet->x, Recv_Player_Packet->y, Recv_Player_Packet->type);
            }
        }
        else if (GameState == 3)
        {
            retval = recvn(sock, buf, sizeof(Packet), 0);
            buf[retval] = '\0';
            Recv_Packet_Type = (Packet*)buf;
            if (Recv_Packet_Type->type == 1)
            {
                retval = recvn(sock, buf, sizeof(PlayerPacket), 0);
                buf[retval] = '\0';
                Recv_Player_Packet = (PlayerPacket*)buf;
                SetEvent(hRecvEvent);
            }
            else if (Recv_Packet_Type->type == 2)
            {
                retval = recvn(sock, buf, sizeof(BubblePacket), 0);
                buf[retval] = '\0';
                Recv_Bubble_Packet = (BubblePacket*)buf; 
                SetEvent(hRecvEvent);
            }
        }
    }

    // closesocket()
    closesocket(sock);

    // ���� ����
    WSACleanup();
    return 0;
}

// ���带 ������ ������
DWORD WINAPI SendClient(LPVOID arg)
{
    WaitForSingleObject(hConnectEvent, INFINITE);   // RecvClient���� Connect �� ������ wait
    // ó���� �κ� ȭ�鿡�� Ŭ���� ���� ����, game state �� ingame�̸� break�ϴ� ����
    WaitForSingleObject(hSendEvent, INFINITE);
    printf("ClientPacket Send Value : %d", Send_Client_Packet);
    send(sock, (char*)&Send_Client_Packet, sizeof(Packet), 0);
    ResetEvent(hSendEvent);
    if (GameState == 2)
    {
        while (1)
        {
            if (GameState == 3)
            {
                break;
            }
        }
    }
    // ingame �����ϰ� �� ���Ŀ��� �Է¿� ���� ������ �۽�
    if (GameState == 3)
    {
        while (1)
        {
            WaitForSingleObject(hSendEvent, INFINITE);
            send(sock, (char*)&Send_Client_Packet, sizeof(Packet), 0);
            ResetEvent(hSendEvent);
        }
    }
    return 0;
}