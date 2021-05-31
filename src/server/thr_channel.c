//
// Created by 🍎 on 2021/5/27.
//


#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <netinet/in.h>



#include "thr_channel.h"
#include "server_conf.h"

struct channel_threadid_st
{
    chnid_t chnid;
    pthread_t tid;
};


static int tid_pos;
struct channel_threadid_st channelthrs[CHNNR];


_Noreturn static void* channel_send(void* arg)
{
    ssize_t len,realsend;
    struct medialib_listinfo_st* listinfo=arg;
    struct msg_channel_st* buf;


    buf= malloc(MAX_CHN_MSG);
    if(buf==NULL)
    {
        syslog(LOG_ERR,"[thr_channel.c] channel_send()@malloc() error : %s\n", strerror(errno));
        exit(1);
    }
    buf->chnid=listinfo->chnid;
    while(1)
    {
        bzero(buf->data,MAX_DATA);
        len=medialib_ReadChannel(listinfo->chnid,buf->data,MAX_DATA);
        realsend=sendto(sfd,buf,len+ sizeof(chnid_t),0,(struct sockaddr*)&client_addr, sizeof(client_addr));
        if(realsend<0)
        {
            syslog(LOG_ERR,"[thr_channel.c] sendto channel %d msg : %s\n",listinfo->chnid,strerror(errno));
            //exit(1);
        }
        else
        {
            syslog(LOG_DEBUG,"[thr_channel.c] channel %d sendto %d bytes\n",listinfo->chnid,(int)realsend);
        }
        sched_yield();
    }
    pthread_exit(NULL);
}

int thr_channel_create(struct medialib_listinfo_st* listinfo)
{
    int err;
    int i;

    for(i=0;i<CHNNR;i++)
    {
        channelthrs[i].chnid=-1;
    }
    err=pthread_create(&channelthrs[tid_pos].tid,NULL,channel_send,listinfo);
    if(err)
    {
        syslog(LOG_WARNING,"[thr_channel.c] thr_channel_create()@pthread_create() error : %s\n", strerror(errno));
        return -err;
    }
    channelthrs[tid_pos].chnid=listinfo->chnid;
    tid_pos++;
    return 0;
}
int thr_channel_destroy(struct medialib_listinfo_st* listinfo)
{
    int i;
    for(i=0;i<CHNNR;i++)
    {
        if(channelthrs[i].chnid== listinfo->chnid)
        {
            if(!pthread_cancel(channelthrs[i].tid))
            {
                syslog(LOG_ERR,"[thr_channel.c] thr_channel_destroy()@pthread_cancel() error : %s\n", strerror(errno));
                return -ESRCH;
            }
            pthread_join(channelthrs[i].tid,NULL);
            channelthrs[i].chnid=-1;
            return 0;
        }
    }
    return 0;
}
int thr_channel_destroy_all()
{
    int i;
    for(i=0;i<tid_pos;i++)
    {
        if(channelthrs[i].chnid>0)
        {
            if(pthread_cancel(channelthrs[i].tid))
            {
                syslog(LOG_ERR,"[thr_channel.c] thr_channel_destroy_all()@pthread_cancel() error : %s\n", strerror(errno));
                return -ESRCH;
            }
            pthread_join(channelthrs[i].tid,NULL);
            channelthrs[i].chnid=-1;
        }
    }
    return 0;
}