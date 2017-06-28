//		2017�� 1�б� ��Ʈ��ũ���α׷��� ���� 3��
//      ����: ������ �й�: 122021

#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "resource.h"

#define SERVERIPV4  "127.0.0.1"
#define SERVERPORT  9000

#define BUFSIZE     256                    // ���� �޽��� ��ü ũ��
#define MSGSIZE     (BUFSIZE-sizeof(int))  // ä�� �޽��� �ִ� ����

#define CHATTING    1000                   // �޽��� Ÿ��: ä��

char nick1[BUFSIZE+1];	//	room1 nickname
char nick2[BUFSIZE+1];	//	room2 nickname

char roomNick1[BUFSIZE+1];
char roomNick2[BUFSIZE+1];
char tempNick1[BUFSIZE+1];
char tempNick2[BUFSIZE+1];

bool ErrorIP(HWND hdlg,char ip[]);
bool ErrorPort(HWND hdlg,char _port[]);

static char ipaddr[64]; // ���� IP �ּ�
static int port; // ���� ��Ʈ ��ȣ
static char port2[50];//��Ʈ��ȣ����Ȯ�� ����

// ���� �޽��� ����
// sizeof(COMM_MSG) == 256
struct COMM_MSG
{
	int  type;
	char dummy[MSGSIZE];
};

// ä�� �޽��� ����
// sizeof(CHAT_MSG) == 256
struct CHAT_MSG
{
	int  type;
	char buf[MSGSIZE];
};

int i=0;
char buf[51];
char buf2[51];
char* line_p;
int signal=0;

static HINSTANCE     g_hInst; // ���� ���α׷� �ν��Ͻ� �ڵ�
static HWND          g_hButtonSendMsg; // '�޽��� ����' ��ư
static HWND          g_hButtonSendMsg2; // '�޽��� ����' ��ư
static HWND          g_hEditStatus, g_hEditStatus2; // ���� �޽��� ���
static char          g_ipaddr[64]; // ���� IP �ּ�
static u_short       g_port; // ���� ��Ʈ ��ȣ
static HANDLE        g_hClientThread, g_hClientThread2; // ������ �ڵ�
static volatile BOOL g_bStart; // ��� ���� ����
static SOCKET        g_sock, g_sock2; // Ŭ���̾�Ʈ ����
static HANDLE        g_hReadEvent, g_hWriteEvent, g_hReadEvent2, g_hWriteEvent2; // �̺�Ʈ �ڵ�
static CHAT_MSG      g_chatmsg, g_chatmsg2; // ä�� �޽��� ����

// ��ȭ���� ���ν���
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
// ���� ��� ������ �Լ�
DWORD WINAPI ClientMain(LPVOID arg);
DWORD WINAPI ClientMain2(LPVOID arg);
DWORD WINAPI ReadThread(LPVOID arg);
DWORD WINAPI WriteThread(LPVOID arg);
DWORD WINAPI ReadThread2(LPVOID arg);
DWORD WINAPI WriteThread2(LPVOID arg);

// ���� ��Ʈ�� ��� �Լ�
void DisplayText(char *fmt, ...);
void DisplayText2(char *fmt, ...);
// ����� ���� ������ ���� �Լ�
int recvn(SOCKET s, char *buf, int len, int flags);
// ���� ��� �Լ�
void err_quit(char *msg);
void err_display(char *msg);

// ���� �Լ�
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
	// ���� �ʱ�ȭ
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0) return 1;

	// �̺�Ʈ ����
	g_hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	if(g_hReadEvent == NULL) return 1;
	g_hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if(g_hWriteEvent == NULL) return 1;

	g_hReadEvent2 = CreateEvent(NULL, FALSE, TRUE, NULL);
	if(g_hReadEvent2 == NULL) return 1;
	g_hWriteEvent2 = CreateEvent(NULL, FALSE, FALSE, NULL);
	if(g_hWriteEvent2 == NULL) return 1;

	// ���� �ʱ�ȭ(�Ϻ�)
	g_chatmsg.type = CHATTING;
	g_chatmsg2.type = CHATTING+1;

	// ��ȭ���� ����
	g_hInst = hInstance;
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	// �̺�Ʈ ����
	CloseHandle(g_hReadEvent);
	CloseHandle(g_hWriteEvent);

	CloseHandle(g_hReadEvent2);
	CloseHandle(g_hWriteEvent2);

	// ���� ����
	WSACleanup();
	return 0;
}

// ��ȭ���� ���ν���
BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HWND hEditIPaddr;
	static HWND hEditPort;
	static HWND hButtonConnect;
	static HWND hEditMsg, hEditMsg2, hEditNick1, hEditNick2, hNick1, hNick2, hCheck1, hCheck2, Room1, Room2, rAll, hPort, hIP, hR1, hR2;

	switch(uMsg){
	case WM_INITDIALOG:
		// ��Ʈ�� �ڵ� ���
		hEditIPaddr = GetDlgItem(hDlg, IDC_ADDR);
		hEditPort = GetDlgItem(hDlg, IDC_PORT);
		hButtonConnect = GetDlgItem(hDlg, IDC_CONNECT);
		hNick1 = GetDlgItem(hDlg, IDC_NICK1);
		hNick2 = GetDlgItem(hDlg, IDC_NICK2);
		hPort = GetDlgItem(hDlg, IDC_PORTOK);
		hIP = GetDlgItem(hDlg, IDC_IPOK);
		hEditNick1 = GetDlgItem(hDlg, IDC_NIC1);
		hEditNick2 = GetDlgItem(hDlg, IDC_NIC2);
		g_hButtonSendMsg = GetDlgItem(hDlg, IDC_SEND1);
		g_hButtonSendMsg2 = GetDlgItem(hDlg, IDC_SEND2);
		hEditMsg = GetDlgItem(hDlg, IDC_CON1);
		hEditMsg2 = GetDlgItem(hDlg, IDC_CON2);
		g_hEditStatus = GetDlgItem(hDlg, IDC_R1);
		g_hEditStatus2 = GetDlgItem(hDlg, IDC_R2);
		hCheck1=GetDlgItem(hDlg,IDC_CHECK1);
		hCheck2=GetDlgItem(hDlg,IDC_CHECK2);
		Room1 = GetDlgItem(hDlg,IDC_R1M);
		Room2 = GetDlgItem(hDlg,IDC_R2M);
		hR1 = GetDlgItem(hDlg, IDC_ROOM1);
		hR2 = GetDlgItem(hDlg, IDC_ROOM2);
		rAll = GetDlgItem(hDlg,IDC_ALL);
		// ��Ʈ�� �ʱ�ȭ
		SendMessage(hEditMsg, EM_SETLIMITTEXT, MSGSIZE, 0);
		EnableWindow(g_hButtonSendMsg, FALSE);
		EnableWindow(g_hButtonSendMsg2, FALSE);
		EnableWindow(hButtonConnect, FALSE);
		EnableWindow(hPort,FALSE);
		EnableWindow(hEditNick1, FALSE);
		EnableWindow(hEditNick2, FALSE);
		EnableWindow(hNick1, FALSE);
		EnableWindow(hNick2, FALSE);
		EnableWindow(hEditMsg, FALSE);
		EnableWindow(hEditMsg2, FALSE);
		SetDlgItemText(hDlg, IDC_ADDR, SERVERIPV4);
		SetDlgItemInt(hDlg, IDC_PORT, SERVERPORT, FALSE);
		EnableWindow(hR1, FALSE);
		EnableWindow(hR2, FALSE);
		EnableWindow(Room1,FALSE);
		EnableWindow(Room2,FALSE);

		return TRUE;

	case WM_COMMAND:
		switch(LOWORD(wParam)){


		case IDC_IPOK:
			GetDlgItemText(hDlg, IDC_ADDR, g_ipaddr, sizeof(g_ipaddr));
			
			if(ErrorIP(hDlg,g_ipaddr))
			{				
				EnableWindow(hEditIPaddr,FALSE);
				EnableWindow(hPort,TRUE);
				EnableWindow(hButtonConnect,FALSE);
				EnableWindow(hIP,FALSE);
			}
			else{
				SetFocus(hEditIPaddr);
					MessageBox(hDlg,"�ּҸ� �ٽ��Է��Ͻÿ�","�߸��� �ּ��Է�", MB_ICONERROR);
			}
			return TRUE;

		case IDC_PORTOK:
			
			GetDlgItemText(hDlg, IDC_PORT, port2, sizeof(port2));
			if(ErrorPort(hDlg, port2))//������ �����ʾҴٸ� 
			{
				g_port = GetDlgItemInt(hDlg, IDC_PORT, NULL, FALSE);
				EnableWindow(hEditPort,FALSE);
				EnableWindow(hButtonConnect,TRUE);
				int num;
				num=atoi(port2);
				port=num;//��Ʈ������
				EnableWindow(hEditPort,FALSE);
				EnableWindow(hButtonConnect, TRUE);
				EnableWindow(hPort,FALSE);
			}
			else
			{
				SetFocus(hEditPort);
				MessageBox(hDlg,"��Ʈ��ȣ�� �ٽ��Է��Ͻÿ�","�߸��� ��Ʈ��ȣ�Է�", MB_ICONERROR);
			
			}				
			return TRUE;
			
		case IDC_CONNECT:
			GetDlgItemText(hDlg, IDC_ADDR, g_ipaddr, sizeof(g_ipaddr));
			g_port = GetDlgItemInt(hDlg, IDC_PORT, NULL, FALSE);
			EnableWindow(hButtonConnect, FALSE);
			EnableWindow(hR1, TRUE);
			EnableWindow(hR2, TRUE);
			FILE *f;
			FILE *fp;
			FILE *ALL;
			
			return TRUE;

		case IDC_ROOM1:
			EnableWindow(hEditNick1, TRUE);
			EnableWindow(hNick1, TRUE);
			signal += 1;
			// ���� ��� ������ ����
			g_hClientThread = CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);
			if(g_hClientThread == NULL){
				MessageBox(hDlg, "Ŭ���̾�Ʈ�� ������ �� �����ϴ�."
					"\r\n���α׷��� �����մϴ�.", "����!", MB_ICONERROR);
				EndDialog(hDlg, 0);
			}
			else{
				EnableWindow(hButtonConnect, FALSE);
				while(g_bStart == FALSE); // ���� ���� ���� ��ٸ�
				EnableWindow(hEditIPaddr, FALSE);
				EnableWindow(hEditPort, FALSE);
				EnableWindow(g_hButtonSendMsg, FALSE);
				EnableWindow(hR1, FALSE);
				
			}
			return TRUE;

		case IDC_R1M:
			DisplayText("\n====1���� �����ο�====\n");
			f=fopen("nickname.txt", "r");
				   while( !feof( f )){
						//fgets(buf, sizeof(buf), f);
						fscanf(f, "%s\n", buf);
						

						DisplayText("\n%s\n",buf);
				   }

				fclose(f);
				   return TRUE;


		case IDC_R2M:
			DisplayText2("\n====2���� �����ο�====\n");
			fp=fopen("nickname2.txt", "r");
				   while( !feof( fp )){
						//fgets(buf, sizeof(buf), fp);
						fscanf(fp, "%s\n", buf2);
						

						DisplayText2("\n%s\n",buf2);
				   }

				fclose(fp);

			return TRUE;

		case IDC_ALL:
			if(signal == 2)
			{
				DisplayText("\n====1���� �����ο�====\n");
				f=fopen("nickname.txt", "r");
					   while( !feof( f )){
							//fgets(buf, sizeof(buf), f);
							fscanf(f, "%s\n", buf);
						

							DisplayText("\n%s\n",buf);
					   }

				
				  
				DisplayText2("\n====2���� �����ο�====\n");
				fp=fopen("nickname2.txt", "r");
					   while( !feof( fp )){
							//fgets(buf, sizeof(buf), fp);
							fscanf(fp, "%s\n", buf2);
						

							DisplayText2("\n%s\n",buf2);
					   }

					fclose(f);
					fclose(fp);
			}
			return TRUE;
			
		case IDC_ROOM2:
			EnableWindow(hEditNick2, TRUE);
			EnableWindow(hNick2, TRUE);
			signal += 1;
			// ���� ��� ������ ����
			g_hClientThread2 = CreateThread(NULL, 0, ClientMain2, NULL, 0, NULL);
			if(g_hClientThread2 == NULL){
				MessageBox(hDlg, "Ŭ���̾�Ʈ�� ������ �� �����ϴ�."
					"\r\n���α׷��� �����մϴ�.", "����!", MB_ICONERROR);
				EndDialog(hDlg, 0);
			}
			else{
				EnableWindow(hButtonConnect, FALSE);
				while(g_bStart == FALSE); // ���� ���� ���� ��ٸ�
				EnableWindow(hEditIPaddr, FALSE);
				EnableWindow(hEditPort, FALSE);
				EnableWindow(g_hButtonSendMsg2, FALSE);
				EnableWindow(hR2, FALSE);
			}
			return TRUE;

		case IDC_NICK1:
			GetDlgItemText(hDlg, IDC_NIC1, nick1, BUFSIZE+1);
			// ���� �ϷḦ �˸�

			
			f=fopen("nickname.txt", "a+");
				   while( !feof( f )){
						   fgets(buf, sizeof(buf), f);
						if((line_p = strchr(buf, '\n')) != NULL)
							*line_p ='\0';

					 
								if(strcmp(buf, nick1) == 0)
								{
									DisplayText("�ߺ��� �г����� �̹� �����մϴ�.\n");
									return FALSE;
								}  
						   
				   }
				   strcpy(tempNick1, nick1);

			// �Էµ� �ؽ�Ʈ ��ü�� ���� ǥ��
			EnableWindow(hNick1,FALSE);//üũ�� �ߴٸ� �г��� ��ưȰ��ȭ
			EnableWindow(hEditNick1,FALSE);//�г��� ��Ȱ��ȭ
			EnableWindow(hCheck1,TRUE);//üũ Ȱ��ȭ
			EnableWindow(hEditMsg, TRUE);
			EnableWindow(g_hButtonSendMsg, TRUE); // ������ ��ư Ȱ��ȭ
			CheckDlgButton(hDlg,IDC_CHECK1,FALSE);
			EnableWindow(Room1,TRUE);
			strcpy(roomNick1, nick1);
	
			fprintf(f,roomNick1);
			fprintf(f,"\n");

			fclose(f);
			return TRUE;

		case IDC_NICK2:
			GetDlgItemText(hDlg, IDC_NIC2, nick2, BUFSIZE+1);
			// ���� �ϷḦ �˸�

			
			fp=fopen("nickname2.txt", "a+");
				   while( !feof( fp )){
						   fgets(buf2, sizeof(buf2), fp);
						if((line_p = strchr(buf2, '\n')) != NULL)
							*line_p ='\0';

					 
								if(strcmp(buf2, nick2) == 0)
								{
									DisplayText2("�ߺ��� �г����� �̹� �����մϴ�.\n");
									return FALSE;
								}  
						   
				   }
				   strcpy(tempNick2, nick2);


			// �Էµ� �ؽ�Ʈ ��ü�� ���� ǥ��
			EnableWindow(hNick2,FALSE);//üũ�� �ߴٸ� �г��� ��ưȰ��ȭ
			EnableWindow(hEditNick2,FALSE);//�г��� ��Ȱ��ȭ
			EnableWindow(hCheck2,TRUE);//üũ Ȱ��ȭ
			EnableWindow(hEditMsg2, TRUE);
			EnableWindow(g_hButtonSendMsg2, TRUE); // ������ ��ư Ȱ��ȭ
			CheckDlgButton(hDlg,IDC_CHECK2,FALSE);
			EnableWindow(Room2,TRUE);
			strcpy(roomNick2, nick2);

			fprintf(fp,roomNick2);
			fprintf(fp,"\n");
			fclose(fp);
			return TRUE;

		case IDC_CHECK1:
			EnableWindow(hNick1,TRUE);//üũ�� �ߴٸ� �г��� ��ưȰ��ȭ
			EnableWindow(hEditMsg, FALSE);
			EnableWindow(hCheck1,FALSE);//üũ ��Ȱ��ȭ
			EnableWindow(g_hButtonSendMsg, FALSE);
			EnableWindow(hEditNick1,TRUE);//�г��Ӿ����ְ� Ȱ��ȭ
			return TRUE;

		case IDC_CHECK2:
			EnableWindow(hNick2,TRUE);//üũ�� �ߴٸ� �г��� ��ưȰ��ȭ
			EnableWindow(hEditMsg2, FALSE);
			EnableWindow(hCheck2,FALSE);//üũ ��Ȱ��ȭ
			EnableWindow(g_hButtonSendMsg2, FALSE);
			EnableWindow(hEditNick2,TRUE);//�г��Ӿ����ְ� Ȱ��ȭ
			return TRUE;

		case IDC_SEND1:
			// �б� �ϷḦ ��ٸ�
			WaitForSingleObject(g_hReadEvent, INFINITE);
			GetDlgItemText(hDlg, IDC_NIC1, nick1, MSGSIZE);
			GetDlgItemText(hDlg, IDC_CON1, g_chatmsg.buf, MSGSIZE);
			// ���� �ϷḦ �˸�
			SetEvent(g_hWriteEvent);
			// �Էµ� �ؽ�Ʈ ��ü�� ���� ǥ��
			SendMessage(hEditMsg, EM_SETSEL, 0, -1);
			return TRUE;

		case IDC_SEND2:
			// �б� �ϷḦ ��ٸ�
			WaitForSingleObject(g_hReadEvent2, INFINITE);
			GetDlgItemText(hDlg, IDC_NIC2, nick2, MSGSIZE);
			GetDlgItemText(hDlg, IDC_CON2, g_chatmsg2.buf, MSGSIZE);
			// ���� �ϷḦ �˸�
			SetEvent(g_hWriteEvent2);
			// �Էµ� �ؽ�Ʈ ��ü�� ���� ǥ��
			SendMessage(hEditMsg2, EM_SETSEL, 0, -1);
			return TRUE;

		case IDCANCEL:
			if(MessageBox(hDlg, "������ �����Ͻðڽ��ϱ�?",
				"����", MB_YESNO|MB_ICONQUESTION) == IDYES)
			{
				remove("nickname.txt"); //1�� �� ������ ������ �����.
				remove("nickname2.txt"); //2�� �� ������ ������ �����.

				closesocket(g_sock);
				closesocket(g_sock2);
				EndDialog(hDlg, IDCANCEL);
				
			}
			return TRUE;

		}
		return FALSE;
	}

	return FALSE;
}

// ���� ��� ������ �Լ�
DWORD WINAPI ClientMain(LPVOID arg)
{
	int retval;


		// socket()
		g_sock = socket(AF_INET, SOCK_STREAM, 0);
		if(g_sock == INVALID_SOCKET) err_quit("socket()");

		// connect()
		SOCKADDR_IN serveraddr;
		ZeroMemory(&serveraddr, sizeof(serveraddr));
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_addr.s_addr = inet_addr(g_ipaddr);
		serveraddr.sin_port = htons(g_port);
		retval = connect(g_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
		if(retval == SOCKET_ERROR) err_quit("connect()");

	
	MessageBox(NULL, "1�� �濡 �����߽��ϴ�.", "����!", MB_ICONINFORMATION);

	

	// �б� & ���� ������ ����
	HANDLE hThread[2];
	hThread[0] = CreateThread(NULL, 0, ReadThread, NULL, 0, NULL);
	hThread[1] = CreateThread(NULL, 0, WriteThread, NULL, 0, NULL);
	if(hThread[0] == NULL || hThread[1] == NULL){
		MessageBox(NULL, "�����带 ������ �� �����ϴ�."
			"\r\n���α׷��� �����մϴ�.",
			"����!", MB_ICONERROR);
		exit(1);
	}

	g_bStart = TRUE;

	// ������ ���� ���
	retval = WaitForMultipleObjects(2, hThread, FALSE, INFINITE);
	retval -= WAIT_OBJECT_0;
	if(retval == 0)
		TerminateThread(hThread[1], 1);
	else
		TerminateThread(hThread[0], 1);
	CloseHandle(hThread[0]);
	CloseHandle(hThread[1]);

	g_bStart = FALSE;
	remove("nickname.txt"); //1�� �� ������ ������ �����.
	remove("nickname2.txt"); //2�� �� ������ ������ �����.
	MessageBox(NULL, "������ ������ �������ϴ�", "�˸�", MB_ICONINFORMATION);
	EnableWindow(g_hButtonSendMsg, FALSE);

	closesocket(g_sock);
	return 0;
}

// ���� ��� ������ �Լ�
DWORD WINAPI ClientMain2(LPVOID arg)
{
	int retval;


		// socket()
		g_sock2 = socket(AF_INET, SOCK_STREAM, 0);
		if(g_sock2 == INVALID_SOCKET) err_quit("socket()");

		// connect()
		SOCKADDR_IN serveraddr;
		ZeroMemory(&serveraddr, sizeof(serveraddr));
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_addr.s_addr = inet_addr(g_ipaddr);
		serveraddr.sin_port = htons(g_port);
		retval = connect(g_sock2, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
		if(retval == SOCKET_ERROR) err_quit("connect()");

	
	MessageBox(NULL, "2�� �濡 �����߽��ϴ�.", "����!", MB_ICONINFORMATION);

	// �б� & ���� ������ ����
	HANDLE hThread[2];
	hThread[0] = CreateThread(NULL, 0, ReadThread2, NULL, 0, NULL);
	hThread[1] = CreateThread(NULL, 0, WriteThread2, NULL, 0, NULL);
	if(hThread[0] == NULL || hThread[1] == NULL){
		MessageBox(NULL, "�����带 ������ �� �����ϴ�."
			"\r\n���α׷��� �����մϴ�.",
			"����!", MB_ICONERROR);
		exit(1);
	}

	g_bStart = TRUE;

	// ������ ���� ���
	retval = WaitForMultipleObjects(2, hThread, FALSE, INFINITE);
	retval -= WAIT_OBJECT_0;
	if(retval == 0)
		TerminateThread(hThread[1], 1);
	else
		TerminateThread(hThread[0], 1);
	CloseHandle(hThread[0]);
	CloseHandle(hThread[1]);

	g_bStart = FALSE;
	remove("nickname.txt"); //1�� �� ������ ������ �����.
	remove("nickname2.txt"); //2�� �� ������ ������ �����.
	MessageBox(NULL, "������ ������ �������ϴ�", "�˸�", MB_ICONINFORMATION);
	EnableWindow(g_hButtonSendMsg2, FALSE);
	
	closesocket(g_sock2);
	return 0;
}

// ������ �ޱ�
DWORD WINAPI ReadThread(LPVOID arg)
{
	int retval;
	COMM_MSG comm_msg;
	CHAT_MSG *chat_msg;

	while(1){
		retval = recvn(g_sock, nick1, BUFSIZE, 0);
		if(retval == 0 || retval == SOCKET_ERROR){
			break;
		}

		retval = recvn(g_sock, (char *)&comm_msg, BUFSIZE, 0);
		if(retval == 0 || retval == SOCKET_ERROR){
			break;
		}

		if(comm_msg.type == CHATTING){
			chat_msg = (CHAT_MSG *)&comm_msg;
			DisplayText("[%s] %s\r\n", nick1, chat_msg->buf);
		}

	}

	return 0;
}

// ������ �ޱ�
DWORD WINAPI ReadThread2(LPVOID arg)
{
	int retval;
	COMM_MSG comm_msg2;
	CHAT_MSG *chat_msg2;

	while(1){
		retval = recvn(g_sock2, nick2, BUFSIZE, 0);
		if(retval == 0 || retval == SOCKET_ERROR){
			break;
		}

		retval = recvn(g_sock2, (char *)&comm_msg2, BUFSIZE, 0);
		if(retval == 0 || retval == SOCKET_ERROR){
			break;
		}

		if(comm_msg2.type == CHATTING+1){
			chat_msg2 = (CHAT_MSG *)&comm_msg2;
			DisplayText2("[%s] %s\r\n", nick2, chat_msg2->buf);
		}

	}

	return 0;
}

// ������ ������
DWORD WINAPI WriteThread(LPVOID arg)
{
	int retval;

	// ������ ������ ���
	while(1){
		// ���� �Ϸ� ��ٸ���
		WaitForSingleObject(g_hWriteEvent, INFINITE);

		// ���ڿ� ���̰� 0�̸� ������ ����
		if(strlen(g_chatmsg.buf) == 0){
			// '�޽��� ����' ��ư Ȱ��ȭ
			EnableWindow(g_hButtonSendMsg, TRUE);
			// �б� �Ϸ� �˸���
			SetEvent(g_hReadEvent);
			continue;
		}

		// ������ ������
		retval = send(g_sock, nick1, BUFSIZE, 0);
		if(retval == SOCKET_ERROR){
			break;
		}

		retval = send(g_sock, (char *)&g_chatmsg, BUFSIZE, 0);
		if(retval == SOCKET_ERROR){
			break;
		}

		// '�޽��� ����' ��ư Ȱ��ȭ
		EnableWindow(g_hButtonSendMsg, TRUE);
		// �б� �Ϸ� �˸���
		SetEvent(g_hReadEvent);
	}

	return 0;
}

// ������ ������
DWORD WINAPI WriteThread2(LPVOID arg)
{
	int retval;

	// ������ ������ ���
	while(1){
		// ���� �Ϸ� ��ٸ���
		WaitForSingleObject(g_hWriteEvent2, INFINITE);

		// ���ڿ� ���̰� 0�̸� ������ ����
		if(strlen(g_chatmsg2.buf) == 0){
			// '�޽��� ����' ��ư Ȱ��ȭ
			EnableWindow(g_hButtonSendMsg2, TRUE);
			// �б� �Ϸ� �˸���
			SetEvent(g_hReadEvent2);
			continue;
		}

		// ������ ������
		retval = send(g_sock2, nick2, BUFSIZE, 0);
		if(retval == SOCKET_ERROR){
			break;
		}

		retval = send(g_sock2, (char *)&g_chatmsg2, BUFSIZE, 0);
		if(retval == SOCKET_ERROR){
			break;
		}

		// '�޽��� ����' ��ư Ȱ��ȭ
		EnableWindow(g_hButtonSendMsg2, TRUE);
		// �б� �Ϸ� �˸���
		SetEvent(g_hReadEvent2);
	}

	return 0;
}


// ����Ʈ ��Ʈ�ѿ� ���ڿ� ���
void DisplayText(char *fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);

	char cbuf[1024];
	vsprintf(cbuf, fmt, arg);

	int nLength = GetWindowTextLength(g_hEditStatus);
	SendMessage(g_hEditStatus, EM_SETSEL, nLength, nLength);
	SendMessage(g_hEditStatus, EM_REPLACESEL, FALSE, (LPARAM)cbuf);

	va_end(arg);
}

void DisplayText2(char *fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);

	char cbuf[1024];
	vsprintf(cbuf, fmt, arg);

	int nLength = GetWindowTextLength(g_hEditStatus2);
	SendMessage(g_hEditStatus2, EM_SETSEL, nLength, nLength);
	SendMessage(g_hEditStatus2, EM_REPLACESEL, FALSE, (LPARAM)cbuf);

	va_end(arg);
}

// ����� ���� ������ ���� �Լ�
int recvn(SOCKET s, char *buf, int len, int flags)
{
	int received;
	char *ptr = buf;
	int left = len;

	while(left > 0){
		received = recv(s, ptr, left, flags);
		if(received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if(received == 0)
			break;
		left -= received;
		ptr += received;
	}

	return (len - left);
}

// ���� �Լ� ���� ��� �� ����
void err_quit(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// ���� �Լ� ���� ���
void err_display(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

bool ErrorIP(HWND hdlg,char ip[]){

	char arr[4][4]={0,};
	int j=0;
	int k=0;
	int cnt=0;
	int num[4];
	for(int i=0;i<strlen(ip);i++)
	{
		if(ip[i]=='.')	//	������ ����
		{	
			num[k]=atoi(arr[k]);
			if(num[k]<0||num[k]>255)//����üũ 
				return false;//FALSE
			j=0;
			k++;
			cnt++;	//	���� ��			
		}
		else{
			if(isdigit(ip[i])){		//.�� �����ϰ� ��������Ȯ����, ���ڰ� �ƴѴٸ� �����ϰ�� FALSE
			arr[k][j]=ip[i];		//����
			j++;}
			else
				return false;
		}
	}
	num[k]=atoi(arr[k]);				//���������� ����� �������� 
	if(cnt!=3||num[k]<0||num[k]>255)	//����Ȯ��, ���� 3������ Ȯ��
		return false;

	return true;
}
bool ErrorPort(HWND hdlg,char _port[]){

	int i=0;
	int num;
	int cnt=0;
	for(i=0;i<strlen(_port);i++)
	{
		if(isdigit(_port[i]))	//	��Ʈ�� ��������Ȯ��, �ٸ� ���ڶ�� FALSE
			cnt++;
		else
			return FALSE;		//���ڰ� �ϳ��� �ƴϸ� FALSE
			
	}
	if(cnt==strlen(_port))	//port���̿� ���ٸ�
	{
		num=atoi(_port);			//���ڰ����� �ٲ���
		if(num>=1024&&num<=49151)	//���ٶ����� ��Ʈ��ȣ
			return TRUE;			// ������ ������ TRUE,

	}
	return FALSE;
}
