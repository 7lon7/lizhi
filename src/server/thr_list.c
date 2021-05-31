//
// Created by 🍎 on 2021/5/27.
//


#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>


#include "../include/proto.h"
#include "thr_list.h"
#include "server_conf.h"



static pthread_t tid_list;
static int nr_list_ent;
static struct medialib_listinfo_st* list_ent;


_Noreturn static void* thr_list(void* arg)
{
    int i;
    ssize_t ret;
    int size;
    int totalsize;
    struct msg_list_st* entlistp;
    struct msg_channel_info_st* entryp;


    totalsize= sizeof(chnid_t);
    for(i=0;i<nr_list_ent;i++)
    {
        totalsize+= (int)(sizeof(struct msg_channel_info_st)+strlen(list_ent[i].info));
    }
    entlistp=malloc(totalsize);
    if(entlistp==NULL)
    {
        syslog(LOG_ERR,"[thr_list.c] thr_list()@malloc() : %s\n", strerror(errno));
        exit(1);
    }
    entlistp->chnid=LISTCHNID;
    entryp=entlistp->chninfo;
    for(i=0;i<nr_list_ent;i++)
    {
        size= (int)(sizeof(struct msg_channel_info_st)+ strlen(list_ent[i].info));
        entryp->chnid=list_ent[i].chnid;
        entryp->len= htons(size);
        strcpy((char*)entryp->info,list_ent[i].info);
        write(STDOUT_FILENO,entryp,size);
        entryp=(void*)(((char*)entryp)+size);
    }
    while(1)
    {
        ret=sendto(sfd,entlistp,totalsize,0,(struct sockaddr*)&client_addr, sizeof(client_addr));
        if(ret<0)
        {
            syslog(LOG_WARNING,"[thr_list.c] sendto channel list : %s\n", strerror(errno));
        }
        else
        {
            syslog(LOG_DEBUG,"[thr_list.c] channel list sendto...\n");
        }
        sleep(1);
    }
}


int thr_list_create(struct medialib_listinfo_st* listinfo,int num)
{
    int err;
    list_ent=listinfo;
    nr_list_ent=num;

    err= pthread_create(&tid_list,NULL,thr_list,NULL);
    if(err)
    {
        syslog(LOG_ERR,"[thr_list.c] thr_list_create()@pthread_create() error : %s\n", strerror(errno));
        return -1;
    }
    return 0;
}
int thr_list_destroy()
{
    pthread_cancel(tid_list);
    pthread_join(tid_list,NULL);
    return 0;
}