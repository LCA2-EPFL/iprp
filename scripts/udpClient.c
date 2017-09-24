//senddata.c
  
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define NUM 10000000
//#define NUM 20000

void error(const char *msg)
{
    perror(msg);
    exit(0);
}
 
int main(int argc, char** argv)
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr, srcaddr;
    struct hostent *server;
    int slen  = sizeof(serv_addr);

    char buf[256];
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }

    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if (sockfd < 0) 
        error("ERROR opening socket");

    memset(&srcaddr, 0, sizeof(srcaddr));
    srcaddr.sin_family = AF_INET;
    srcaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    srcaddr.sin_port = htons(1005);

    if (bind(sockfd, (struct sockaddr *) &srcaddr, sizeof(srcaddr)) < 0) {
        perror("bind");
        exit(1);
    }
    
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    if (inet_aton(argv[1], &serv_addr.sin_addr)==0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    struct timeval tv;
    int jj = 0;
    long t1, ut1;
    int ctr;

    int id;
    long t2,ut2;
    
    int x;
    while(jj < NUM)
    {
	   bzero(buf,256);
        gettimeofday(&tv, NULL);
        t1  = tv.tv_sec;
        ut1  = tv.tv_usec;

        getchar();
        scanf("%d",&x);
        sprintf(buf,"%d %d %d\0",jj,t1,ut1);
        printf("buf: %s\n",buf);
        
        if (sendto(sockfd, buf, 255, 0, (struct sockaddr*)&serv_addr, slen)==-1)
            err("sendto()");
        jj++;
        usleep(1000*1000);
    }
    close(sockfd);
    return 0;
}
