//senddata.c
 
/*
 * ./senddata <IP1> <Interval in milliseconds> <No. of Pkts>
 */
 
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
#define BUFLEN 500
#define PORT 1002

void err(char *s)
{
    perror(s);
    exit(1);
}
 
int main(int argc, char** argv)
{
    struct sockaddr_in serv_addr;
    int sockfd, i, slen=sizeof(serv_addr);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
        err("socket");
 
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    //int port = atoi(argv[4]);
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET,argv[1], &serv_addr.sin_addr)==0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
    
    //int val = IPV6_PMTUDISC_DO;
    //setsockopt(sockfd,IPPROTO_IPV6,IPV6_MTU_DISCOVER, &val, sizeof(val));

   //int val1 = 1;
   //setsockopt(sockfd,IPPROTO_IPV6,IPV6_DONTFRAG, &val1, sizeof(val1));

   /* int typ, siz;
    siz = sizeof(int);

    if(getsockopt(sockfd,IPPROTO_IPV6,IPV6_PATHMTU,(char *)&typ,&siz) < 0)
    {
        err("getsockopt");
    }
    else
    {
        printf("obtained: %d\n", typ);
    }
    */

    char buf[BUFLEN];
    struct timeval tv;
    struct timeval now;
    time_t curtime;
    char Sec[30];
    char Msec[12];
    char lnxTime[14];
    long jj = 1;
    long dlay;
    //double avg = 0,sdlay = 0;
    int interval = atoi(argv[2]);
    long num = atoi(argv[3]);
  

    while(jj < num) //To be replaced with while(1)
    {
    	//scanf("%s",buf);
        strcpy(buf,"");

        gettimeofday(&tv, NULL);
        curtime=tv.tv_sec;
        //strftime(Sec,30,"%m-%d-%Y,%T,",localtime(&curtime));
        sprintf(buf,"%-10ld %d.%d",jj,(int)tv.tv_sec,(int)tv.tv_usec);
        //strcat(buf,Sec);
        //strcat(buf,"Aa");
            
        if (sendto(sockfd, buf, BUFLEN, 0, (struct sockaddr*)&serv_addr, slen)==-1)
            perror("Error: ");

	    printf("Sent Packet %10ld. %s\n",jj,buf);
	    jj++;
	   
        gettimeofday(&now, NULL);
        //dlay = tv.tv_usec+ (interval*1000) -now.tv_usec;
        dlay  = interval *1000;
        usleep(dlay);
        }
 
    close(sockfd);
    return 0;
}