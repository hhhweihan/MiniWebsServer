#ifdef WIN32
#include<windows.h>
#else 
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#define closesocket close
#endif


#include<stdio.h>

int main(int argc, char* argv[])
{
#ifdef WIN32
    WSADATA ws;
    WSAStartup(MAKEWORD(2, 2), &ws);
#endif 
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        printf("create socket failed!\n");
    }
    printf("[%d]", sock);
   close(sock);
    getchar();
    return 0;
}
