//serA.c
 
/*
 *  usage: ./serA <No. to receive>
 */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#define BUFLEN 512
#define PORT 1002

void err(char *str)
{
    perror(str);
    exit(1);
}
 
int main(int argc, char** argv)
{
    int recvLimit = atoi(argv[1]);
    
    struct sockaddr_in my_addr, cli_addr;
    int sockfd, i;
    socklen_t slen=sizeof(cli_addr);
    char buf[BUFLEN];
    char *clientAddr = malloc(INET_ADDRSTRLEN);
    //struct in6_addr anyaddr = IN6ADDR_ANY_INIT;
 
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
      err("socket");
    else
      printf("Initializing PDC...\n");
 
    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(PORT);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY); //in6addr_loopback;
     
    if (bind(sockfd, (struct sockaddr* ) &my_addr, sizeof(my_addr))==-1)
      err("bind");
    else
      printf("PDC started successfully.\n");
 
    char dummy[BUFLEN];
    char logMsg[50]="";
    char frCPU[59]="";
    char outstat[85] = "";
    char wsta[33] = "";
    char ts[45] = "";

    struct timeval tv1;
    int recv=0, old_id=0, new_id,recv1_loss=0;
    double dlay, pkt1_time,cursec;

    
    while(recv < recvLimit) //To be replaced with while(1)
    {  
       if (recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr*)&cli_addr, &slen)==-1)
          {
            perror("recvError: ");
          }
       else
       { 
         new_id = atoi(strncpy(dummy,buf,10)); 
         pkt1_time = atof(strncpy(dummy,buf+31,18));
         printf("Received Pkt #%d\n",new_id);
         if(old_id != new_id-1)
         {
         recv1_loss += new_id-old_id-1; //For all packets which never came
         printf("Lost %i packets from ID %i to %i\n", new_id - old_id-1, old_id, new_id);
         gettimeofday(&tv1, NULL);

         
         strftime(ts,45,"%d,%m,%Y,%H,%M,%S,%s",localtime(&tv1.tv_sec));
         sprintf(outstat,"%s.%ld",ts,tv1.tv_usec);
         
         sprintf(dummy,"./loss_d.sh %i %s %i&",old_id+1,outstat,new_id-old_id-1);
         //printf("dummy: %s",dummy);
         system(dummy);

         sprintf(logMsg,"%10i lost. Log wireless!",old_id+1);     
        }
         old_id = new_id;
         recv++;
       }
     
    }
    printf("Loss: %i/%i\n",recv1_loss,recv); //To be removed
    close(sockfd);
    return 0;
}

