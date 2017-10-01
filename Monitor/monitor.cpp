/*******************************************************

Project: CloudScale (Team 4)

File Name: monitor.cpp

Author: Bharath Banglaore Veeranna
		migrate_trigger() function added by Suhaskrishna Gopalkrishna
		
Description:
This file handles the monitoring module decribed in the CloudScale paper. 
This is to be executed in Domain-0 of all the hosts.
This program collects CPU and memory usage of all hosts and sends it to the predictor		


********************************************************/

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
#include <iostream>
#include <list>
#include <algorithm>
#include <errno.h>
#include <sys/socket.h>

using namespace std;

#define MAX_DATA_SIZE 65535
#define NAME_SIZE 100
#define SAMPLE_INTERVAL 10
#define SAMPLE_STEPS 10
#define MAX_CPU_PCT 90
#define CPU_ALLOWED 80

#define MAX_VM 10
#define PRED_TIME 30
#define K_SIZE 3

const float weight[] = {0.0181818, 0.0363637, 0.0545455, 0.0727273, 0.1090909, 0.1272728, 0.1454546, 0.1636364, 0.1818182};

extern "C"
{
#include <xenstat.h>

double monitor(char name[], double timeElapsed, xenstat_node **prevNode, xenstat_node **curNode);
}

struct stat
{
    double cpu_value;
    double memory_value;
    struct timeval time;
};

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

/* 
Structure used to define a linked list which contains 
details of all VMs running on the host 
*/
struct domain
{
    char name[NAME_SIZE];
    std::list<struct stat> pred;
    double cpu_cur;
    double memory_cur;
    int cpu_pad;
    int memory_pad;
    double cpu_pred;
    double memory_pred;
};

std::list<struct domain> domainList;

std::list<int> threadList;

pthread_mutex_t mutexThread;

/*
Author: Bharath Banglaore Veeranna
Description:
The following functions maintains a list of all active threads
Collection of statistics from each VM is done on individual threads
*/
void addToThread(int sockfd)
{
    pthread_mutex_lock(&mutexThread);

    std::list<int>::iterator iter = threadList.begin();

    threadList.push_back(sockfd);

    pthread_mutex_unlock(&mutexThread);    

}

void deleteFromThread(int sockfd)
{
    
    pthread_mutex_lock(&mutexThread);

    std::list<int>::iterator iter = threadList.begin();

    while(iter != threadList.end())
    {
        if(*iter == sockfd)
        {
            threadList.erase(iter);
            pthread_mutex_unlock(&mutexThread);
            return;
        }
        iter++;
    }

    pthread_mutex_unlock(&mutexThread);    
}

pthread_mutex_t mutexList;

/*
Author: Bharath Banglaore Veeranna
Description:
functions to add/delete entries to a linked list which contains VM information
*/

void addToList(char name[])
{
    struct domain node;

    node.cpu_cur = 0;
    node.memory_cur = 0;
    node.cpu_pad = 0;
    node.memory_pad = 0;
    node.cpu_pred = 0;
    node.memory_pred = 0;

    memset(node.name, 0, NAME_SIZE);

    strcpy(node.name, name);
    
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

/*
Author: Bharath Banglaore Veeranna
Description:
When a new CPU and memory usage is obtained, the same is updated 
in the linked list by ths function
*/
void addToCpuPred(char name[], struct stat cpu)
{
    pthread_mutex_lock(&mutexList);

    std::list<struct domain>::iterator iter = domainList.begin();

    while(iter != domainList.end())
    {
        if(strcmp(iter->name, name) == 0)
        {
            iter->pred.push_back(cpu);
            break;
        }
        iter++;
    }

    pthread_mutex_unlock(&mutexList);
}

/*
Author: Suhaskrishna Gopalkrishna
Description:
This function triggers the migration of a VM when scaling conflict occurs

*/
void migrate_trigger(char* vmname)
{
        printf("\n ***** Conflict will occur, migrate *****");
        
        int udp_fd;
        if ((udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        {
            printf("\nCannot create socket. Migration Trigger failed.");
            return;
        }
        int udpServerPort = 7000;

        struct sockaddr_in migAddr;
        memset((char *)&migAddr, 0, sizeof(migAddr));
        migAddr.sin_family = AF_INET;
        migAddr.sin_port = htons(udpServerPort);
        migAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        if (sendto(udp_fd, vmname, strlen(vmname), 0, (struct sockaddr *)&migAddr, sizeof(migAddr)) < 0)
        {
            printf("\nSendto failed. Migration Trigger failed");
        }
        return;
}

/*
Author: Bharath Banglaore Veeranna
Description:
This function checks regularly if scaling conflict will occur
Also, it determines which VM to be migrated in case of conflict
*/
int checkConflict()
{
    pthread_mutex_lock(&mutexList);

    std::list<struct domain>::iterator iter = domainList.begin();

    char name[MAX_VM][100];
    double cpuValues[MAX_VM][K_SIZE];
    double memValues[MAX_VM][K_SIZE];

    struct timeval cur;
    gettimeofday(&cur, NULL);

    int i,j;

    i = -1;

    while(iter != domainList.end())
    {
        std::list<struct stat>::iterator statIter = iter->pred.begin();

        while(statIter != iter->pred.end())
        {
            if(statIter->time.tv_sec >= cur.tv_sec + PRED_TIME  && statIter->time.tv_sec < cur.tv_sec + SAMPLE_INTERVAL + PRED_TIME)
            {
                i++;
                memset(name[i], 0, 100);
                strcpy(name[i], iter->name);
                cpuValues[i][0] = statIter->cpu_value;
                memValues[i][0] = statIter->memory_value;
                j = 1;

                for(; j < K_SIZE; j++)
                {
                    statIter++;
                    cpuValues[i][j] = 0;
                    if(statIter != iter->pred.end())
                    {
                        cpuValues[i][j] = statIter->cpu_value;
                        memValues[i][j] = statIter->memory_value;
                    }
                }
                break;
            }  

            else if(statIter->time.tv_sec < cur.tv_sec)
            {   
                statIter = iter->pred.erase(statIter);
                continue;
            }

            statIter++;
        }

        iter++;
    }

    pthread_mutex_unlock(&mutexList);
    

    int len = i+1;

    int totalCpu[K_SIZE];
    int totalMem[K_SIZE];
    int count = 0;

    for(j = 0; j < K_SIZE; j++)
    {
        totalCpu[j] = 0;
        totalMem[j] = 0;
    }

    for(i = 0; i < len; i++)
    {
        for(j = 0; j < K_SIZE; j++)
        {
            totalCpu[j]+= cpuValues[i][j];
            totalMem[j] += memValues[i][j];
        }
    }

    int migrate = -1;

    if(totalCpu[0] > MAX_CPU_PCT && totalCpu[1] > MAX_CPU_PCT && totalCpu[2] > MAX_CPU_PCT)
    {
        int tempCpu[MAX_VM];
        int pos[MAX_VM];
        for(i = 0; i < len; i++)
            tempCpu[i] = cpuValues[i][K_SIZE-1];

        sort(tempCpu, tempCpu+len);

        for(i = 0; i < len; i++)
        {
            if(totalCpu[K_SIZE-1] - tempCpu[i] <= CPU_ALLOWED)
            {
                for(j = 0; j < len; j++)
                {
                    if(tempCpu[i] == cpuValues[j][K_SIZE-1])
                        break;
                }
                migrate = j;
                break;
            }
        }

    }

    if(totalMem[0] > MAX_CPU_PCT && totalMem[1] > MAX_CPU_PCT && totalMem[2] > MAX_CPU_PCT)
    {
        int tempMem[MAX_VM];
        int pos[MAX_VM];
        for(i = 0; i < len; i++)
            tempMem[i] = memValues[i][K_SIZE-1];

        sort(tempMem, tempMem+len);

        for(i = 0; i < len; i++)
        {
            if(totalMem[K_SIZE-1] - tempMem[i] <= CPU_ALLOWED)
            {
                for(j = 0; j < len; j++)
                {
                    if(tempMem[i] == memValues[j][K_SIZE-1])
                        break;
                }
                migrate = j;
                break;
            }
        }

    }

    if(migrate >= 0)
    {
        for(i=0;i<len;i++)
        {
            char sysCall[100];
            memset(sysCall, 0, 100);
            sprintf(sysCall, "xl sched-credit -d %s -c 0", name[i]);
            system(sysCall);
        }
        migrate_trigger(name[migrate]);
    }

    return 0;
}

xenstat_handle *xen_handle = NULL;

pthread_mutex_t mutexMonitor;

/*
Author: Bharath Banglaore Veeranna
Description:
This function is used to collect CPU usage of a VM using libxenstat functions
*/

double monitor(char name[], double timeElapsed, xenstat_node **prev_node, xenstat_node **cur_node)
{
    //pthread_mutex_lock(&mutexMonitor);
    xenstat_domain *cur_domain, *prev_domain;
    unsigned int i, j, domain_id,  num_domains = 0;
    unsigned long long cpu_ns;
    double cpu_pct;

    char domainName[NAME_SIZE];

    *prev_node = *cur_node;

    //if(*cur_node != NULL)
        //xenstat_free_node(*cur_node);

    *cur_node = xenstat_get_node(xen_handle, XENSTAT_ALL);

    if(*prev_node == NULL)
    {
        //printf("\n It is NULL");
        //pthread_mutex_unlock(&mutexMonitor);
        return 0;
    }

    num_domains = xenstat_node_num_domains(*cur_node);

    for(i = 0; i < num_domains; i++)
    {
        cur_domain = xenstat_node_domain_by_index(*cur_node, i);
        prev_domain = xenstat_node_domain_by_index(*prev_node, i);

        if(prev_domain == NULL || cur_domain == NULL)
            continue;

        domain_id = xenstat_domain_id(cur_domain);

        memset(domainName, 0, NAME_SIZE);

        strcpy(domainName, xenstat_domain_name(cur_domain));

        if(strcmp(domainName, name) != 0)
            continue;

        cpu_ns = xenstat_domain_cpu_ns(cur_domain) - xenstat_domain_cpu_ns(prev_domain);
        cpu_pct = cpu_ns / (timeElapsed * 10);
        //printf("\n        Domain %s:  usage: %f",domainName,  cpu_pct);
        //fflush(stdout);
        
        //pthread_mutex_unlock(&mutexMonitor);

        return cpu_pct;
 
    }

    //pthread_mutex_unlock(&mutexMonitor);
    printf("\n Cannot find: %s",name);
}

struct threadArg
{
    pthread_t threadId;
    int sockfd;
    sockaddr_in clientAddr;
    char serverIP[NAME_SIZE];
};

/*
Author: Bharath Banglaore Veeranna
Description:
This a thread function. Each VM will have an instance of this thread.
This thread communicates with the VM using TCP socket to get the memory usage
After obtaining CPU and memory usage, it sends the data to the prediction algorithm
running on a different host. The predicted value is read and capping is done.
Fast underestimation correction and remedial padding are done in this function
*/
void* clientTask(void* data)
{
    struct threadArg *arg = (struct threadArg*)data;
    char name[NAME_SIZE];
    char buffer[MAX_DATA_SIZE];

    int sockfd = arg->sockfd;

    memset(buffer, 0, MAX_DATA_SIZE);
    memset(name, 0, NAME_SIZE);
    
    int status = read(sockfd, buffer, MAX_DATA_SIZE);

    double timeElapsed;
    struct timeval prev, cur;

    strncpy(name, buffer, NAME_SIZE);

    addToList(name);

    addToThread(sockfd);

    gettimeofday(&cur, NULL);

    //---------------------------------------------------

    int handlerFd = socket(AF_INET, SOCK_STREAM, 0);

    int serverPortNum = 9000;

    struct sockaddr_in serverAddr;

    serverAddr.sin_family = AF_INET;
    inet_pton(AF_INET, arg->serverIP, &serverAddr.sin_addr);
    serverAddr.sin_port = htons(serverPortNum);

    int enable = 1;
    if (setsockopt(handlerFd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        printf("\n Error setting reuse addr");
    }

    if(connect(handlerFd,(struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0)
    {
        printf("\n Connect Failed");
        return 0;
    }

    char sendData[MAX_DATA_SIZE];

    memset(sendData, 0, MAX_DATA_SIZE);
    strncpy(sendData, name, strlen(name));
    
    sendData[strlen(sendData)]='\0';
    fflush(stdout);

    write(handlerFd, sendData, strlen(sendData)+1);
    
    //--------------------------------------------------

    double cpu, memory;
    double cpu_pred, memory_pred;
    double cpu_pad, memory_pad;
    double cpu_error, memory_error;

    bool valid = false;

    int cpu_error_count = 0;
    int memory_error_count = 0;

    float alpha = 1;

    cpu = 0;
    memory = 0;
    cpu_pred = 0;
    memory_pred = 0;
    cpu_pad = 0;
    memory_pad = 0;


    std::list<double> cpuErrorList;
    std::list<double> memoryErrorList;

    int cpuListLen = 0;
    int memoryListLen = 0;

    xenstat_node *prev_node = NULL;
    xenstat_node *cur_node = NULL;
    while(1)
    {
        
        memset(buffer, 0, MAX_DATA_SIZE);
        status = read(sockfd, buffer, MAX_DATA_SIZE);  


        if(status > 0)
        {
            //printf( "\n %d : %s %s",sockfd, name, buffer);    
            memory = atof(buffer);

            prev = cur;
    
            char fileName[100];
            memset(fileName, 0, 100);
            sprintf(fileName, "result/%s_console.txt",name);
            FILE *writeFile = fopen(fileName, "a+");

            gettimeofday(&cur, NULL);
            timeElapsed = ((cur.tv_sec-prev.tv_sec)*1000000.0 + (cur.tv_usec - prev.tv_usec));
            cpu = monitor(name, timeElapsed, &prev_node, &cur_node); 
            //printf("\n%f", cpu);
            fprintf(writeFile, "%f\n", cpu);

            if(cpu < 0.01)
            {
                //printf("\n less than 0.001");
                continue;
            }

            bool memoryRetrain = false;
            bool cpuRetrain = false;

            if(valid == true)
            {
                cpu_error = cpu_pred - cpu;
                if(cpu_error < 0)
                {
                    cpu_error_count++;
                    cpu_error *= -1;
                } 
                else
                {
                    cpu_error_count = 0;
                    cpu_error = 0;
                }

                cpuErrorList.push_back(cpu_error);
                cpuListLen++;

                if(cpuListLen >= 10)
                {
                    cpuErrorList.pop_front();
                    cpuListLen--;
                }

                if(cpu_error_count == 3)
                {
                    cpuRetrain = true;   
                    cpu_error_count = 0;
                    printf("\n %s : Error Correction by retraining", name);
                }

                cpu_error = (cpu/(cpu_pred + cpu_pad)) * 100;
    
                if(cpu_error > 70)
                {   
                    
                    printf("\n %s: Underestimation correction: alpha = %f", name, alpha);

                    alpha += 0.1;
                    if(alpha > 2)
                    {
                        alpha = 2;
                    }
                }
                else
                {
                    alpha = 1;
                }
            }
            else
            {

            }

            //printf("\n %s: %f", name, cpu);
            fflush(stdout);
            
            struct predictionRequest cpu_mem;
            cpu_mem.cpu = cpu;
            cpu_mem.cpuRetrain = cpuRetrain;
            cpu_mem.memory = memory;
            cpu_mem.memoryRetrain = memoryRetrain;

            write(handlerFd, &cpu_mem, sizeof(struct predictionRequest));

            memset(sendData, 0, MAX_DATA_SIZE);

            int status = read(handlerFd, sendData, MAX_DATA_SIZE);
            if(status > 0)
            {
                struct predictionResponse *cpu_mem_resp;
                
                cpu_mem_resp = (struct predictionResponse*) sendData;
  
                //printf("\n %s : %f", name, cpu_mem_resp->cpu_pred);

                cpu_pred = cpu_mem_resp->cpu_pred;
                cpu_pad = cpu_mem_resp->cpu_pad;

                if(cpuListLen == 10)
                {
                    double remedial_pad = 0;

                    std::list<double>::iterator iter = cpuErrorList.begin();

                    for(int i = 0; i < 10; i++)
                    {
                        remedial_pad += weight[i] * (*iter);
                        iter++;
                    }

                    if(remedial_pad > cpu_pad)
                    {
                        cpu_pad = remedial_pad;
                    }

                }

                //cpu_pad *= alpha;

                if(cpu_pred > 900)
                    cpu_pred = 0;
                else
                    valid = true;

                if(cpu_pad > 900)
                    cpu_pad = 0;

                fprintf(writeFile, "%f,%f,", cpu_pred+cpu_pad, cpu_pad);
                //fprintf(writeFile, "\nMemory \t: Predicted: %f Padded: %f Actual: ", memory_pred, memory_pad);
                
                if(cpu_pred+cpu_pad < 10)
                {
                    cpu_pred = 5;
                    cpu_pad = 5;
                }

                char systemCall[500];
                memset(systemCall, 0, 500);
                //printf("credit scheduler: %f", cpu_pred+cpu_pad);
                sprintf(systemCall, "sudo xl sched-credit -d %s -c %f", name, cpu_pred+cpu_pad);

                //system(systemCall);

                fclose(writeFile);

                struct stat pred;
                pred.cpu_value = cpu_mem_resp->cpu_conflict_pred;
                pred.memory_value = cpu_mem_resp->memory_conflict_pred;
                pred.time = cur;

                pred.time.tv_sec += SAMPLE_STEPS * SAMPLE_INTERVAL;
                if(pred.cpu_value < 900)
                    addToCpuPred(name, pred);
            }
            else
            {
                printf("\n Read failed.....");
            }
            fflush(stdout);
        }
        else
        {
            printf( "\n Read NULL, errno = %d", errno);
            fflush(stdout);
            
            deleteFromThread(sockfd);

            deleteFromList(name);
            close(sockfd);
            close(handlerFd);
            return data;
        }
        
    }
}

/*
Author: Bharath Banglaore Veeranna
Description:
This function is used to provide synchronization to all the threads. This triggers
all the VMs to send memory usage at the same time. The response from the VM is handled in 
clientTask() function mentioned above
*/
void *trigger(void *data)
{
    std::list<int>::iterator iter;

    struct timeval cur, prev;

    gettimeofday(&cur, NULL);

    while(1)
    {
        pthread_mutex_lock(&mutexThread);
    
        iter = threadList.begin();

        while(iter != threadList.end())
        {
            try
            {
                //printf("\n \t\tWrite to : %d", *iter);
                fflush(stdout); 
                write(*iter, "1\0", 2);
                iter++;
            }
            catch (const std::exception& e)
            {   
                printf("\n deleting sockfd");
                fflush(stdout);
                deleteFromThread(*iter);
                continue;
            }
        }

        pthread_mutex_unlock(&mutexThread);
    
        sleep(SAMPLE_INTERVAL/2);

        //audit();

       int ret =  checkConflict();

        sleep(SAMPLE_INTERVAL/2);

        if( ret > 0)
        {
            sleep(30);
        }

    }
}

/*
Author: Bharath Banglaore Veeranna
Description:
The main program which opens a TCP socket and listens for any connection from a VM
When a new VM is brought up, it connects to this socket. For each VM connected, a seperate 
thread is created
*/
int  main(int argc, char *argv[])
{
    xen_handle = xenstat_init();

    char serverIP[NAME_SIZE];

    memset(serverIP, 0, NAME_SIZE);

    if(argc < 2)
    {
        printf("\n Pass Server IP\n");
        return 0;
    }

    strncpy(serverIP, argv[1], NAME_SIZE);

    int count = 0;

    int sockfd, portNum, clientFd;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t len;

    portNum = 6000;

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

    pthread_t triggerThread;

    pthread_create(&triggerThread, NULL, trigger, NULL);

    while(1)
    {
        len = sizeof(clientAddr);
        clientFd = accept(sockfd, (struct sockaddr *)&clientAddr, &len);
    
        if(clientFd > 0)
        {
            printf("\n Accepted client");
            fflush(stdout); 
        
            struct threadArg arg;
            arg.sockfd = clientFd;
            arg.clientAddr = clientAddr;
            strcpy(arg.serverIP, serverIP);

            pthread_create(&(arg.threadId), NULL, clientTask, &arg);
        }       
        else
        {
            printf("\n Not accepted.. Error : %d", errno);
            fflush(stdout);
        } 
    }
    
}

