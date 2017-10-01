/*******************************************************

Project: CloudScale (Team 4)

File Name: monitor.cpp

Author: Bharath Banglaore Veeranna
		
Description:
This code is the client side code for the monitoring tool. This should be executed in each of the VMs individually.
The client running on the VM collects memory usage statistics and sends it to the Domain-0		


********************************************************/


#include<iostream>
#include<stdio.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include<string>
#include <arpa/inet.h>
#include<stdlib.h>

using namespace std;

#define MAX_DATA_SIZE 65535
#define NAME_SIZE 100

int main(int argc, char *argv[])
{
    int sockfd = -1;
    struct sockaddr_in serverAddr;
    socklen_t len;

    if(argc < 2)
    {
        printf("\n Pass  VM name\n");
        printf("Format: ./client.o VM_Name");
        return 0;
    }

    char name[NAME_SIZE];
    memset(name, 0, NAME_SIZE);

    strcpy(name, argv[1]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd < 1)
    {
        printf("\n Socket Creation Failed");
        return 0;
    }
    else
    {
        printf("\n Socket Created");
    }

    int serverPortNum = 6000;

    serverAddr.sin_family = AF_INET;
    inet_pton(AF_INET, "192.168.122.1", &serverAddr.sin_addr);
    serverAddr.sin_port = htons(serverPortNum);

    int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        printf("\n Error setting reuse addr");
    }

    if(connect(sockfd,(struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0)
    {
        printf("\n Connect Failed");
        return 0;
    }

    sleep(1);

    char buffer[MAX_DATA_SIZE];
    memset(buffer, 0, MAX_DATA_SIZE);
    
    strcpy(buffer, name);
    write(sockfd, buffer, strlen(buffer));
    sleep(2);

    int ret;

    char tempData[100];
    memset(tempData, 0, 100);

    double mem_usage = 0;

    while(1)
    {
        memset(buffer, 0, MAX_DATA_SIZE);
        ret = read(sockfd, &buffer, MAX_DATA_SIZE);

        //printf("\n Received: %s", buffer);

        if(ret < 0)
        {
            while(connect(sockfd,(struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0)
            {
                printf("\n Trying to reconnect...");
                sleep(2);
            }
            memset(buffer, 0, MAX_DATA_SIZE);
    
            strcpy(buffer, name);
            write(sockfd, buffer, strlen(buffer));
        }

		system("./stat.sh");

        FILE *fp = fopen("mem_temp.txt", "r");

        if(fp == NULL)
        {
            mem_usage = 0;
        }

        memset(tempData, 0, 100);
        fgets(tempData, 100, fp);
        mem_usage = atof(tempData);

        memset(tempData, 0, 100);
        sprintf(tempData, "%f", mem_usage);

        ret = write(sockfd, tempData, strlen(tempData)+1);
    
        if(ret < 0)
        {
            
            while(connect(sockfd,(struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0)
            {
                printf("\n Trying to reconnect...");
                sleep(2);
            }
            memset(buffer, 0, MAX_DATA_SIZE);
    
            strcpy(buffer, name);
            write(sockfd, buffer, strlen(buffer));
        }   
        sleep(5); 
    }

}
