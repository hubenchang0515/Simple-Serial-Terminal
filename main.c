#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>

struct PortState
{
	char nCom[20];
	DWORD BaudRate;
	BYTE DataBits;
	BYTE Parity;
	BYTE StopBits;
};

HANDLE hCom;
struct PortState setting;
void config(const char* file);
void Init(void);
DWORD WINAPI Transmit(LPVOID lpParam);
DWORD WINAPI Receive(LPVOID lpParam);

int main(int argc,char* argv[])
{
	if(argc == 1)
	{
		config("config.ini");
		Init();
	}
	else if(argc == 2)
	{
		config(argv[1]);
		Init();
	}
	else
	{
		printf("Arguments is too many.\n");
		exit(1);
	}

	HANDLE hThread[2];
	hThread[0] = CreateThread(NULL,0,Transmit,NULL,0,NULL); 
	hThread[1] = CreateThread(NULL,0,Receive,NULL,0,NULL);
	WaitForMultipleObjects(2,hThread,TRUE,INFINITE);
	CloseHandle(hThread[0]);
	CloseHandle(hThread[1]);
	return 0;
}

void config(const char* file)
{
	FILE *p = NULL;
	p = fopen(file,"r");
	
	if(p == NULL)
	{
		printf("Configuration file [%s] is not found.\n",file);
		exit(2);
	}

	char ncom[10];
	char parity = 0;
	char stopbits[5];
	fseek(p,0L,SEEK_SET);
	fscanf(p,"[COM]%s\n",ncom);
	fscanf(p,"[BaudRate]%ld\n",&setting.BaudRate);
	fscanf(p,"[DataBits]%c\n",&setting.DataBits);
	fscanf(p,"[Parity]%c\n",&parity);
	fscanf(p,"[StopBits]%s",stopbits);
	fclose(p);

	strcpy(setting.nCom,"\\\\.\\");
	strcat(setting.nCom,ncom);
	setting.DataBits = setting.DataBits - 48;
	switch(parity)
	{
		case 'N': //No
		case 'n': setting.Parity = NOPARITY;break;
		
		case 'O': //Odd
		case 'o': setting.Parity = ODDPARITY;break;
		
		case 'E': //Even
		case 'e': setting.Parity = EVENPARITY;break;

		default: printf("Unknown Parity(only support N O E).\n");
		exit(3);
	}
	if(!strcmp(stopbits,"1"))
		setting.StopBits = ONESTOPBIT;
	else if(!strcmp(stopbits,"1.5"))
		setting.StopBits = ONE5STOPBITS;
	else if(!strcmp(stopbits,"2"))
		setting.StopBits = TWOSTOPBITS;
	else
	{
		printf("Unknown StopBits(only support 1 1.5 2).\n");
		exit(4);
	}
}

void Init(void)
{
	printf("Simple Serial Terminal(hubenchang0515@outlook.com)\n");

		/* 打开串口 */
	hCom = CreateFile(setting.nCom,
						GENERIC_READ|GENERIC_WRITE,
						0,
						NULL,
						OPEN_EXISTING,
						FILE_FLAG_OVERLAPPED,
						NULL);
	if(hCom == (HANDLE)(-1))
	{
		printf("Serial port [%s] cannot be opened.\n",setting.nCom);
		exit(5);
	}


	COMMTIMEOUTS TimeOuts;
	TimeOuts.ReadIntervalTimeout = 1;  
	TimeOuts.ReadTotalTimeoutMultiplier = 1;
	TimeOuts.ReadTotalTimeoutConstant = 100;
	TimeOuts.WriteTotalTimeoutMultiplier = 1;
	TimeOuts.WriteTotalTimeoutConstant = 100;
	SetCommTimeouts(hCom,&TimeOuts);

	/* 设置串口属性 */
	DCB DCB_buf;
	GetCommState(hCom,&DCB_buf);
	DCB_buf.BaudRate = setting.BaudRate;
	DCB_buf.ByteSize = setting.DataBits;
	DCB_buf.Parity = setting.Parity;
	DCB_buf.StopBits = setting.StopBits;
	SetCommState(hCom,&DCB_buf);
	PurgeComm(hCom,PURGE_TXCLEAR|PURGE_RXCLEAR);

}

DWORD WINAPI Transmit(LPVOID lpParam)
{
	char cmd[1];
	DWORD num;
	DWORD error;
	COMSTAT state;
	OVERLAPPED write_lapped = {0};
	write_lapped.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	while(1)
	{
		cmd[0] = getch();
		ClearCommError(hCom,&error,&state);
		WriteFile(hCom,cmd,1,&num,&write_lapped);
		GetOverlappedResult(hCom,&write_lapped,&num,TRUE);
	}
}

DWORD WINAPI Receive(LPVOID lpParam)
{
	DWORD num;
	DWORD error;
	COMSTAT state;
	OVERLAPPED read_lapped = {0};
	char str[1024];
	read_lapped.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	while(1)
	{
		memset(str,0,1024);
		ClearCommError(hCom,&error,&state);
		ReadFile(hCom,str,1024,&num,&read_lapped);
		GetOverlappedResult(hCom,&read_lapped,&num,TRUE);
		printf("%s",str);
	}
}
