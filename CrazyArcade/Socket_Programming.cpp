#include "Socket_Programming.h"
#include "Packet.h"
 
// �̺�Ʈ
extern HANDLE hRecvEvent, hSendEvent, hConnectEvent, hPlayerEvent, hBubbleEvent;
// ����
extern SOCKET sock;
// ��Ŷ
extern PlayerPacket *Recv_Player_Packet;
extern BubblePacket *Recv_Bubble_Packet;
extern Packet* Send_Client_Packet;
Packet *Recv_Packet_Type;
// �Ұ�
extern BOOL Bubble_Arrive, Player_Arrive;
//�ڵ鰪
extern HWND hwnd;
// �÷��̾�
extern RECT Player1, Player2;
// ���� ������Ʈ enum GAME_BG { MENU = 1, ROBBY = 2, INGAME = 3};
extern int GameState;
extern const int Player_CX = 34;
extern const int Player_CY = 34;
extern int Client_Idx;


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
    buf[retval] = '\0';
    Recv_Player_Packet = (PlayerPacket*)buf;
    SetEvent(hConnectEvent);    //connect�� ������ �۽ſ����� ���� ��밡���ϴٴ� �̺�Ʈ �߻���Ų��.
    printf("�� ��ġ �޾Ҵ�. %d %d\n\n", Recv_Player_Packet->x, Recv_Player_Packet->y);
    printf("Packet ID : %d\nPacket x : %d\nPacket y : %d\nPacket type : %d\n", Recv_Player_Packet->idx_player, Recv_Player_Packet->x, Recv_Player_Packet->y, Recv_Player_Packet->type);
    Client_Idx = Recv_Player_Packet->idx_player;
    // �ڱ� �ڽ��� ��ġ ����
    Player1.left = Recv_Player_Packet->x;
    Player1.top = Recv_Player_Packet->y;
    Player1.right = Player1.left + Player_CX;
    Player1.bottom = Player1.top + Player_CY;


    // ������ ��ſ� ���� while, �� ���� ó�� ������ �������� ���� ��Ŷ�� �޾ƿ��� �۾� �ʿ�
    while (1) {
        if (GameState == 2) // ���� ������Ʈ�� �޴��� ��
        {
            // �ٸ� �÷��̾ �����Ѵٸ� �޾ƿ´�.
            //retval = recvn(sock, buf, sizeof(PlayerPacket), 0);
            //buf[retval] = '\0';
            Recv_Player_Packet = (PlayerPacket*)buf;
        }
        else if (GameState == 3)
        {
            retval = recvn(sock, buf, sizeof(Packet), 0);
            buf[retval] = '\0';
            Recv_Packet_Type = (PlayerPacket*)buf;
            printf("%d �� ��Ŷ ����\n\n", Recv_Packet_Type->type);
            if (Recv_Packet_Type->type == 0 || Recv_Packet_Type->type == 1)
            {
                printf("��������� �Ѿ�Դ�\n");
                retval = recvn(sock, buf, sizeof(PlayerPacket), 0);
                buf[retval] = '\0';
                Recv_Player_Packet = (PlayerPacket*)buf;
                printf("�޾Ҵ�? %d, %d\n", Recv_Player_Packet->x, Recv_Player_Packet->y);
                SetEvent(hPlayerEvent);
                Player_Arrive = true;
            }
            else if (Recv_Packet_Type->type == 16)
            {
                retval = recvn(sock, buf, sizeof(BubblePacket), 0);
                buf[retval] = '\0';
                Recv_Bubble_Packet = (BubblePacket*)buf; 
                SetEvent(hBubbleEvent);
                printf("Power : %d\nX : %d Y : %d\n\n", Recv_Bubble_Packet->power, Recv_Bubble_Packet->x, Recv_Bubble_Packet->y);
                Bubble_Arrive = true;
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
    printf("ClientPacket Send Value : %d\n", Send_Client_Packet);
    send(sock, (char*)&Send_Client_Packet, sizeof(ClientPacket), 0);
    delete Send_Client_Packet;
    SetEvent(hSendEvent);
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
            printf("ClientPacket Send Value : %d\n", Send_Client_Packet);
            send(sock, (char*)&Send_Client_Packet, sizeof(ClientPacket), 0);
            delete Send_Client_Packet;
            SetEvent(hSendEvent);
        }
    }
    return 0;
}