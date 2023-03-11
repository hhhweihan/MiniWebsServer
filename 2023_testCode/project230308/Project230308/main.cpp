#include<string.h>
#include<stdlib.h>
#include<stdio.h>

#ifdef WIN32
#include<windows.h>
#else 
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<arpa/inet.h>
#define closesocket close
#endif

#include<thread>
using namespace std;
class TcpThread
{
public:
    void Main()
    {
        char buf[1024] = {0};
        for(;;)
        {
            int recvlen = recv(client, buf, sizeof(buf)-1, 0);
            if(recvlen <= 0) break;
            buf[recvlen] = '\0';
            if(strstr(buf, "quit") != NULL)
            {
                char re[] = "quit success!\n";
                send(client, re, strlen(re)+1, 0);
                break;
            }
            int sendlen = send(client, "ok\n", 4, 0);
            printf("recv %s\n", buf);
        }
        closesocket(client);
        delete this;
    }
    int client = 0;
};

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
    unsigned short port = 8080;
    if(argc > 1)
    {
        port = atoi(argv[1]);
    }
    sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    //close(sock);
    //getchar();
    saddr.sin_addr.s_addr = htonl(0);
    if(bind(sock, (sockaddr*)&saddr, sizeof(saddr)) != 0)
    {
        printf("bind port %d failed!\n", port);
    }
    printf("bind port %d success!\n", port); 
    listen(sock, 10);
   
    for(;;)
    {
        sockaddr_in caddr;
        socklen_t len = sizeof(caddr);
        int client = accept(sock,(sockaddr*)&caddr, &len);
        if(client <= 0) break;
        printf("accept client %d\n", client);
        char * ip = inet_ntoa(caddr.sin_addr);
        unsigned short cport = ntohs(caddr.sin_port);
        printf("client ip is %s, port is %d\n", ip, cport);
        TcpThread *th = new TcpThread();
        th->client = client;
        thread sth(&TcpThread::Main, th);
        sth.detach(); // shifang ziyuan
    }
   closesocket(sock);
    getchar();
    return 0;
}
