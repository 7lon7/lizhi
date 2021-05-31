//
// Created by 🍎 on 2021/5/27.
//

#include <sys/socket.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>


#include "../include/proto.h"
#include "client.h"



#define DEFAULT_INTERFACE   "eth0"
#define DEFAULT_PLAYER      "/opt/homebrew/bin/mpg123 -   > /Users/cl/tmp/t.txt"

struct client_conf_st conf=
        {
            .mgroup=DEFAULT_MGROUP,
            .recport=DEFAULT_RCVPORT,
            .player=DEFAULT_PLAYER
        };


static ssize_t writen(int fd,void* buf,size_t size)
{
    ssize_t ret;
    int pos=0;
    while(size>0)
    {
        ret= write(fd,buf+pos,size);
        if(ret<0)
        {
            if(errno==EINTR)
            {
                continue;
            }
            perror("write : ");
            return -1;
        }
        size-=ret;
        pos+=ret;
    }
    return pos;
}

static void print_help()
{

}

int main(int argc,char* argv[])
{
    char c;
    ssize_t len;
    pid_t pid;
    int index=0;
    int cfd;
    int val=1;
    struct option opt[]={{"port",1,NULL,'P'},{"mgroup",1,NULL,'M'},{"player",1,NULL,'p'},{"help",0,NULL,'H'},{NULL,0,NULL,0}};
    int pipe_fd[2];
    struct ip_mreqn mreq;
    socklen_t server_addr_len=sizeof (struct sockaddr_in),remote_addr_len= sizeof(struct sockaddr_in);
    struct sockaddr_in client_addr,server_addr,remote_addr;
    int choose;
    int ret=0;
    struct msg_list_st* msg_list;
    struct msg_channel_info_st* pos;
    struct msg_channel_st* msg_channel;
    int totalsize=0;


    while(1)
    {
        c=(char)getopt_long(argc,argv,"P:M:p:H",opt,&index);
        if(c<0)
        {
            break;
        }
        switch (c)
        {
            case 'P' :
            {
                conf.recport=optarg;
                break;
            }
            case 'M' :
            {
                conf.mgroup=optarg;
                break;
            }
            case 'p' :
            {
                conf.player=optarg;
                break;
            }
            case 'H' :
            {
                print_help();
                exit(0);
            }
            default :
            {
                abort();
                break;
            }
        }
    }
    cfd=socket(AF_INET,SOCK_DGRAM,0);
    if(cfd<0)
    {
        perror("socket : ");
        exit(1);
    }
    inet_pton(AF_INET,conf.mgroup,&mreq.imr_multiaddr);
    inet_pton(AF_INET,"0.0.0.0",&mreq.imr_address);
    mreq.imr_ifindex=(int)if_nametoindex(DEFAULT_INTERFACE);
    if(setsockopt(cfd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq, sizeof(mreq))<0)
    {
        perror("setsocket : ");
        exit(1);
    }
    setsockopt(cfd,IPPROTO_IP,IP_MULTICAST_LOOP,&val,sizeof(val));
    client_addr.sin_family=AF_INET;
    client_addr.sin_port= htons(atoi(conf.recport));
    inet_pton(AF_INET,"0.0.0.0",&client_addr.sin_addr.s_addr);
    if(bind(cfd,(struct sockaddr*)&client_addr, sizeof(client_addr))<0)
    {
        perror("bind() : ");
        exit(1);
    }
    if(pipe(pipe_fd)<0)
    {
        perror("pipe() : ");
        exit(1);
    }
    pid=fork();
    if(pid<0)
    {
        perror("fork() : ");
        exit(1);
    }

    if(pid==0)
    {
        close(cfd);
        close(pipe_fd[1]);
        dup2(pipe_fd[0], 0);
        if (pipe_fd[0] > 0)
        {
            close(pipe_fd[0]);
        }
        if (execl("/bin/zsh", "zsh", "-c", conf.player, NULL) < 0)
        {
            perror("execl : ");
            exit(1);
        }
    }
    msg_list= malloc(MAX_LIST_MSG);
    if(msg_list==NULL)
    {
        perror("malloc : ");
        exit(1);
    }
    while(1)
    {
        len=recvfrom(cfd,msg_list,MAX_LIST_MSG,0,(struct sockaddr*)&server_addr,&server_addr_len);
        if(len<sizeof(struct msg_list_st))
        {
            fprintf(stderr,"msg too small\n");
            continue;
        }
        if(msg_list->chnid!=LISTCHNID)
        {
            fprintf(stderr,"isn't list msg\n");
            continue;
        }
        break;
    }
    for(pos=msg_list->chninfo;(char*)pos<((char*)msg_list)+len;pos=(void*)(((char*)pos)+ ntohs(pos->len)))
    {
        printf("channel %d : %s\n",pos->chnid,pos->info);
    }
    free(msg_list);
    fprintf(stdout,"please choose channel:\n");
    while(ret<1)
    {
        ret=scanf("%d",&choose);
        if(ret!=1)
        {
            exit(1);
        }
    }
    msg_channel= malloc(MAX_CHN_MSG);
    if(msg_channel==NULL)
    {
        perror("malloc : ");
        exit(1);
    }
    while(1)
    {
        len=recvfrom(cfd,msg_channel,MAX_CHN_MSG,0,(struct sockaddr*)&remote_addr,&remote_addr_len);
        if(remote_addr.sin_addr.s_addr!=server_addr.sin_addr.s_addr)
        {
            fprintf(stderr,"address don't match\n");
            continue;
        }
        if(len<sizeof(struct msg_channel_st))
        {
            fprintf(stderr,"msg too small\n");
            continue;
        }
        if(msg_channel->chnid==choose)
        {
            fprintf(stdout,"accept from channel %d,%d bytes\n",msg_channel->chnid,(int)len);
            totalsize+=len;
            fprintf(stdout,"total recv %d\n",totalsize);
            ret=writen(pipe_fd[1],msg_channel->data,len-sizeof(chnid_t));
            if(ret<0)
            {
                exit(-1);
            }
            //fprintf(stdout,"write to player %d bytes\n",ret);
        }
    }
    free(msg_channel);
    close(cfd);
    exit(0);
}