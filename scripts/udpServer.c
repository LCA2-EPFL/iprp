#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
 
void error(const char *msg)
{
    perror(msg);
    exit(1);
}
 
int main(int argc, char** argv)
{
    struct sockaddr_in my_addr;
    socklen_t slen1= sizeof(my_addr);
    
    if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }

    
    int sockfd,i;
    char buffer[256]; 
    int portno = atoi(argv[1]);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
      err("socket");
    //else
    //  printf("Server : Socket() successful\n");

    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(portno);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
     
    if (bind(sockfd, (struct sockaddr* ) &my_addr, sizeof(my_addr))==-1)
      err("bind");
    //else
     // printf("Server : bind() successful\n");
    
    int n;
    struct timeval tv2;
    int id = 0, old;
    long t1,ut1;
    double delay;
    int size;

    while(1)
    {
        printf("Here\n");
        bzero(buffer,256);
        size = recv(sockfd, buffer, 255, 0);
        if (size ==-1)
            err("recv()");
        
        /*gettimeofday(&tv2, NULL);

        //if (n < 0) error("ERROR reading from socket");
        //printf("Here is the message: %s\n",buffer);

        sscanf(buffer,"%d %d %d",&id,&t1,&ut1);
        if(old==id)
        {
            continue;
        }
        delay = (tv2.tv_sec - t1)*1000000 + (tv2.tv_usec - ut1);
        delay = delay/1000;
        //printf("id: %d, t1: %d.%d, t2: %d.%d, delay: %f\n", id, t1,ut1,tv2.tv_sec,tv2.tv_usec,delay);
        printf("%d, %f\n", id, delay);

        old = id; 
        */
        printf("Received %d bytes: %s\n", size, buffer);
    }
    close(sockfd);
    return 0; 
}
