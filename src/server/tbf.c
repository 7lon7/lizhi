//
// Created by 🍎 on 2021/5/27.
//

#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>

#include "tbf.h"

struct tbf_st
{
    int cps;
    int burst;
    int token;
    int pos;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};


static struct tbf_st* tbfs[TBF_MAX];
static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t once=PTHREAD_ONCE_INIT;
static pthread_t tid_token_increase;

static int min(int a,int b)
{
    return a<b?a:b;
}

static int get_pos_inlock()
{
    int i;
    for(i=0;i<TBF_MAX;i++)
    {
        if(tbfs[i]==NULL)
        {
            return i;
        }
    }
    return -1;
}

_Noreturn static void* token_increase(void* arg)
{
    int i;
    while(1)
    {
        pthread_mutex_lock(&mutex);
        for(i=0;i<TBF_MAX;i++)
        {
            if(tbfs[i]!=NULL)
            {
                pthread_mutex_lock(&tbfs[i]->mutex);
                tbfs[i]->token+=tbfs[i]->cps;
                if(tbfs[i]->token>tbfs[i]->burst)
                {
                    tbfs[i]->token= tbfs[i]->burst;
                }
                pthread_cond_broadcast(&tbfs[i]->cond);
                pthread_mutex_unlock(&tbfs[i]->mutex);
            }
        }
        pthread_mutex_unlock(&mutex);
        usleep(500000);
    }
}


static void module_unload()
{
    int i;
    pthread_cancel(tid_token_increase);
    pthread_join(tid_token_increase,NULL);
    for(i=0;i<TBF_MAX;i++)
    {
        if(tbfs[i]!=NULL)
        {
            free(tbfs[i]);
        }
    }
}


static void module_load()
{
    int err;
    err=pthread_create(&tid_token_increase,NULL,token_increase,NULL);
    if(err<0)
    {
        syslog(LOG_ERR,"token_increase thread_create : %s\n", strerror(errno));
        exit(1);
    }
    atexit(module_unload);
}



tbf_t* tbf_init(int cps,int burst)
{
    pthread_once(&once,module_load);
    int i;
    struct tbf_st* me;
    me= malloc(sizeof(struct tbf_st));
    if(me==NULL)
    {
        return NULL;
    }
    me->cps=cps;
    me->burst=burst;
    me->token=0;
    pthread_mutex_init(&me->mutex,NULL);
    pthread_cond_init(&me->cond,NULL);
    pthread_mutex_lock(&mutex);
    i=get_pos_inlock();
    if(i<0)
    {
        pthread_mutex_unlock(&mutex);
        free(me);
        return NULL;
    }
    tbfs[i]=me;
    me->pos=i;
    pthread_mutex_unlock(&mutex);
    return me;
}
int tbf_fetch_token(tbf_t* tbf ,int num)
{
    int n;
    struct tbf_st* me=tbf;
    pthread_mutex_lock(&me->mutex);
    while(me->token<=0)
    {
        pthread_cond_wait(&me->cond,&me->mutex);
    }
    n=min(me->token,num);
    me->token-=n;
    pthread_mutex_unlock(&me->mutex);
    return n;
}
int tbf_return_token(tbf_t* tbf,int num)
{
    struct tbf_st* me=tbf;
    pthread_mutex_lock(&me->mutex);
    me->token+=num;
    if(me->token>me->burst)
    {
        me->token=me->burst;
    }
    pthread_cond_broadcast(&me->cond);
    pthread_mutex_unlock(&me->mutex);
    return 0;
}
int tbf_destroy(tbf_t* tbf)
{
    struct tbf_st* me=tbf;
    pthread_mutex_lock(&mutex);
    tbfs[me->pos]=NULL;
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&me->mutex);
    pthread_cond_destroy(&me->cond);
    free(me);
    return 0;
}