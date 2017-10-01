/***************************************************************************************

Project: CloudScale (Team 4)

File Name: handler.cpp

Author: Bharath Banglaore Veeranna

Description:
This file handles the prediction request from all the host.
Each host maintains a TCP connection with this program. This program is to be executed on
a seperate machine. When a host sends the current CPU and memory usage of a VM, the 
prediction algorithm is triggered. The predicted and padding value is returned to the host
using the same TCP socket


***************************************************************************************/


#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include<iostream>
#include<list>

using namespace std;

#define NAME_SIZE 100
#define MAX_DATA_SIZE 65535
#define SAMPLE_INTERVAL 10

/*
Message struture used for prediction request and response from/to
the hosts
*/
struct predictionResponse
{
    double cpu_pred;
    double cpu_conflict_pred;
    double cpu_pad;
    double memory_pred;
    double memory_conflict_pred;
    double memory_pad;
};

struct predictionRequest
{
    float cpu;
    bool cpuRetrain;
    float memory;
    bool memoryRetrain;
};

struct domain
{
    pthread_t threadId;
    char name[NAME_SIZE];
};

std::list<struct domain> domainList;

pthread_mutex_t mutexList;



void addToList(struct domain node )
{
    pthread_mutex_lock(&mutexList);

    std::list<struct domain>::iterator iter = domainList.begin();

    while(iter != domainList.end())
    {
        if(strcmp(iter->name, node.name) == 0)
        {
            pthread_mutex_unlock(&mutexList);
            return;
        }
        iter++;
    }
    domainList.push_back(node);

    pthread_mutex_unlock(&mutexList);
}

void deleteFromList(char name[])
{
    pthread_mutex_lock(&mutexList);

    std::list<struct domain>::iterator iter = domainList.begin();

    while(iter != domainList.end())
    {
        if(strcmp(iter->name, name) == 0)
        {
            domainList.erase(iter);
            pthread_mutex_unlock(&mutexList);
            return;
        }
        iter++;
    }

    pthread_mutex_unlock(&mutexList);
}

struct threadArg
{
    pthread_t threadId;
    int sockfd;
    struct sockaddr_in clientAddr;
};

/*
Description:
For each VM running on a host, a seperate thread is allocated which handles
the prediction request. Each thread calls CloudScale.py when it receives a request
to predict the future resource usage
*/

void* clientTask(void *data)
{
    struct threadArg *arg = (struct threadArg*) data;

    char buffer[MAX_DATA_SIZE];
    char name[NAME_SIZE];

    int sockfd = arg->sockfd;

    memset(buffer, 0, MAX_DATA_SIZE);
    memset(name, 0, NAME_SIZE);

    int status = read(sockfd, buffer, MAX_DATA_SIZE);

    strncpy(name, buffer, NAME_SIZE);

    while(1)
    {
        status = read(sockfd, buffer, MAX_DATA_SIZE);
        
        if(status > 0)
        {
            struct predictionRequest *cpu_mem;
            
            cpu_mem = (struct predictionRequest *) buffer;

            double cpu = cpu_mem->cpu;
            double memory = cpu_mem->memory;
            
            cpu = (cpu > 100) ? 50 : cpu;

            fflush(stdout);     
            char fileName[NAME_SIZE];
            memset(fileName, 0, NAME_SIZE);
            sprintf(fileName, "result/%s_cpu_input.txt", name);
    
            FILE *fp = fopen(fileName, "a+");

            fprintf(fp, "%f,",cpu);
            fclose(fp);

            memset(fileName, 0, NAME_SIZE);
            sprintf(fileName, "result/%s_memory_input.txt", name);
    
            fp = fopen(fileName, "a+");

            fprintf(fp, "%f,",memory);
            fclose(fp);

            memset(fileName, 0, NAME_SIZE);
            sprintf(fileName, "result/%s_cpu_result.txt",name);
            FILE * outFile = fopen(fileName, "a+");

            fprintf(outFile, "%f\n", cpu);

            memset(fileName, 0, NAME_SIZE);
            sprintf(fileName, "result/%s_memory_result.txt",name);
            FILE *outMemory = fopen(fileName, "a+");

            fprintf(outMemory, "%f\n", memory);

            if(cpu_mem->cpuRetrain == true)
            {
                memset(fileName, 0, NAME_SIZE);
                sprintf(fileName, "sudo rm -rf result/%s_cpu_mm.txt");
                system(fileName);
            }

            //-------------------------------- CPU Pred -----------------------------------------------------------------------

            memset(fileName, 0, NAME_SIZE);
            sprintf(fileName, "./CloudScale.py result/%s_cpu_input.txt result/%s_cpu_out.txt result/%s_cpu_mm.txt %d %s", name, name, name, SAMPLE_INTERVAL, name);

            system(fileName);

            fflush(stdout);

            memset(fileName, 0, NAME_SIZE);

            sprintf(fileName, "result/%s_cpu_out.txt",name);

            fp = fopen(fileName, "r");

            if(fp == NULL)
            {
                printf("\n %s Not found");
                fflush(stdout);
                write(sockfd, "999\0", 4);    
                continue;
            }

            /*while(fp == NULL)
            {
                fp = fopen(fileName, "r");
            }*/

            char readData[20];
            memset(readData, 0, 20);

            fgets(readData, 20, fp);

            float cpu_pred = atof(readData);

            float cpu_conflict_pred = 0;
            float cpu_pad = 0;

            if(fgets(readData, 20, fp) != NULL)
            {
                cpu_conflict_pred = atof(readData);
            }
            
            if(fgets(readData, 20, fp) != NULL)
            {
                cpu_pad = atof(readData);
            }

            fclose(fp);
            //-------------------------------- Memory Pred -----------------------------------------------------------------------

            memset(fileName, 0, NAME_SIZE);
            sprintf(fileName, "./CloudScale.py result/%s_memory_input.txt result/%s_memory_out.txt result/%s_memory_mm.txt %d %s", name, name, name, SAMPLE_INTERVAL, name);

            system(fileName);

            fflush(stdout);

            memset(fileName, 0, NAME_SIZE);

            sprintf(fileName, "result/%s_memory_out.txt",name);

            fp = fopen(fileName, "r");

            if(fp == NULL)
            {
                printf("\n %s Not found");
                fflush(stdout);
                write(sockfd, "999\0", 4);    
                continue;
            }

            /*while(fp == NULL)
            {
                fp = fopen(fileName, "r");
            }*/

            memset(readData, 0, 20);

            fgets(readData, 20, fp);

            float memory_pred = atof(readData);

            float memory_conflict_pred = 0;
            float memory_pad = 0;

            if(fgets(readData, 20, fp) != NULL)
            {
                memory_conflict_pred = atof(readData);
            }
            
            if(fgets(readData, 20, fp) != NULL)
            {
                memory_pad = atof(readData);
            }
            fclose(fp);
            //----------------------------------------------------------------------------------------------

            struct predictionResponse cpu_mem_resp;
            cpu_mem_resp.cpu_pred = cpu_pred;
            cpu_mem_resp.cpu_conflict_pred = cpu_conflict_pred;
            cpu_mem_resp.cpu_pad = cpu_pad;
            cpu_mem_resp.memory_pred = memory_pred;
            cpu_mem_resp.memory_conflict_pred = memory_conflict_pred;
            cpu_mem_resp.memory_pad = memory_pad;

            write(sockfd, &cpu_mem_resp, sizeof(struct predictionResponse));    

            //----------------------------------------------------------------------------------------

            
            memset(fileName, 0, NAME_SIZE);
            sprintf(fileName, "./otherPrediction.py result/%s_cpu_input.txt result/%s_cpu_mean.txt", name, name);

            system(fileName);

            fflush(stdout);

            memset(fileName, 0, NAME_SIZE);

            sprintf(fileName, "result/%s_cpu_mean.txt",name);

            fp = fopen(fileName, "r");

            if(fp == NULL)
            {
                printf("\n %s Not found");
                fflush(stdout);
                continue;
            }

            /*while(fp == NULL)
            {
                fp = fopen(fileName, "r");
            }*/

            memset(readData, 0, 20);

            fgets(readData, 20, fp);

            float mean, max, hist, reg, corr;
            mean = max = hist = hist = reg = corr = 0;

            mean = atof(readData);
      
            if(fgets(readData, 20, fp) != NULL)
            {
                max = atof(readData);
            }
       
            if(fgets(readData, 20, fp) != NULL)
            {
                hist = atof(readData);
            }
            if(fgets(readData, 20, fp) != NULL)
            {
                reg = atof(readData);
            }
            if(fgets(readData, 20, fp) != NULL)
            {
                corr = atof(readData);
            }
            fclose(fp);
            //############################################################


            memset(fileName, 0, NAME_SIZE);
            sprintf(fileName, "./otherPrediction.py result/%s_memory_input.txt result/%s_memory_mean.txt", name, name);

            system(fileName);

            fflush(stdout);

            memset(fileName, 0, NAME_SIZE);

            sprintf(fileName, "result/%s_memory_mean.txt",name);

            fp = fopen(fileName, "r");

            if(fp == NULL)
            {
                printf("\n %s Not found");
                fflush(stdout);
                continue;
            }

            /*while(fp == NULL)
            {
                fp = fopen(fileName, "r");
            }*/

            memset(readData, 0, 20);

            fgets(readData, 20, fp);

            float meanM, maxM, histM, regM, corrM;
            meanM = maxM = histM = histM = regM = corrM = 0;

            meanM = atof(readData);
                        
            if(fgets(readData, 20, fp) != NULL)
            {
                maxM = atof(readData);
            }
       
            if(fgets(readData, 20, fp) != NULL)
            {
                histM = atof(readData);
            }
            if(fgets(readData, 20, fp) != NULL)
            {
                regM = atof(readData);
            }
            if(fgets(readData, 20, fp) != NULL)
            {
                corrM = atof(readData);
            }
            fclose(fp);
            //---------------------------------------------------------------------------------------

            fprintf(outFile, "%f,%f,%f,%f,%f,%f,%f,", cpu_pred, cpu_pad, mean, max, hist, reg, corr);
            fprintf(outMemory, "%f,%f,%f,%f,%f,%f,%f,", memory_pred, memory_pad, meanM, maxM, histM, regM, corrM);

            fclose(outMemory);
            fclose(outFile);

            printf("\t CPU: %f : %f \t Memory: %f : %f\n", cpu_pred,  cpu_pad);
            fflush(stdout);
        }
        else
        {
            //deleteFromList(name);
            printf("\n Closing thread");
            return data;
        }
    }

}


int main(int argc, char *argv[])
{
    int sockfd, portNum, clientFd;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t len;

    portNum = 9000;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd < 1)
    {
        printf("\n Server Socket creation failed....");
        return 0;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(portNum);

    int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        printf("\n Error setting reuse addr");
    }

    if(bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        printf("\n Binding to port %d failed at server....", portNum);
        return 0;
    }

    listen(sockfd, 100);

    while(1)
    {
        clientFd = accept(sockfd, (struct sockaddr *)&clientAddr, &len);

        if( clientFd > 0)
        {
            printf("\n Accepted client");
            fflush(stdout);
            struct domain node;
            memset(node.name, 0, NAME_SIZE);
            struct threadArg argv;
            argv.sockfd = clientFd;
            argv.clientAddr = clientAddr;
            pthread_create(&(argv.threadId), NULL, clientTask, &argv);
        }
    }
    
    return 1;
}
