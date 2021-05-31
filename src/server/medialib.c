//
// Created by 🍎 on 2021/5/27.
//

#include <glob.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>


#include "medialib.h"
#include "tbf.h"
#include "server_conf.h"


#define PATHSIZE    128
#define LINEBUFSIZE 1024


struct channel_context_st
{
    chnid_t chnid;
    char* desc;
    glob_t mp3glob;
    int pos;
    int fd;
    off_t offset;
    tbf_t* tbf;
};

static struct channel_context_st channel[CHNNR+1];

static int open_next_media(chnid_t chnid)
{
    int i;
    for(i=0;i<channel[chnid].mp3glob.gl_pathc;i++)
    {
        channel[chnid].pos++;
        if(channel[chnid].pos==channel[chnid].mp3glob.gl_pathc)
        {
            channel[chnid].pos=0;
        }
        close(channel[chnid].fd);
        channel[chnid].fd=open(channel[chnid].mp3glob.gl_pathv[channel[chnid].pos],O_RDONLY);
        if(channel[chnid].fd<0)
        {
            syslog(LOG_WARNING,"[medialib.c] open_next()@open()%s error: %s",channel[chnid].mp3glob.gl_pathv[channel[chnid].pos],strerror(errno));
        }
        else
        {
            syslog(LOG_DEBUG,"[medialib.c] read next media file %s\n",channel[chnid].mp3glob.gl_pathv[channel[chnid].pos]);
            channel[chnid].offset=0;
            return 0;
        }
    }
    syslog(LOG_DEBUG,"[medialib.c] channel %d ?\n",chnid);
    return -1;
}

static struct channel_context_st* path2entry(const char* path)
{
    char pathstr[PATHSIZE];
    char linebuf[LINEBUFSIZE];
    FILE* fp;
    struct channel_context_st* me;
    static chnid_t curr_id=MINCHNID;

    strncpy(pathstr,path,PATHSIZE);
    strncat(pathstr,"/desc.txt",PATHSIZE);

    fp= fopen(pathstr,"r");
    if(fp==NULL)
    {
        syslog(LOG_ERR,"[medialib.c] path2entry()@fopen()%s error: %s\n",pathstr,strerror(errno));
        return NULL;
    }

    if(fgets(linebuf,LINEBUFSIZE,fp)==NULL)
    {
        syslog(LOG_WARNING,"[medialib.c] %s?\n",pathstr);
        fclose(fp);
        return NULL;
    }
    fclose(fp);
    me= malloc(sizeof(struct channel_context_st));
    if(me==NULL)
    {
        syslog(LOG_ERR,"[medialib.c] path2entry()@malloc() error : %s\n", strerror(errno));
        return NULL;
    }
    me->tbf= tbf_init(MP3_BITRATE/8,MP3_BITRATE/8*10);
    if(me->tbf==NULL)
    {
        syslog(LOG_ERR,"[medialib.c] tbf_init() : %s\n", strerror(errno));
        free(me);
        return NULL;
    }
    me->desc= strdup(linebuf);              //TODO free
    bzero(pathstr,PATHSIZE);
    strncpy(pathstr,path,PATHSIZE);
    strncat(pathstr,"/*.mp3",PATHSIZE);
    if(glob(pathstr,0,NULL,&me->mp3glob)!=0)
    {
        curr_id++;
        syslog(LOG_ERR,"[medialib.c] %s?\n",pathstr);
        free(me);
        return NULL;
    }
    me->pos=0;
    me->offset=0;
    me->fd=open(me->mp3glob.gl_pathv[me->pos],O_RDONLY);
    if(me->fd<0)
    {
        syslog(LOG_ERR,"[medialib.c] path2entry()@open()%s error : %s\n",me->mp3glob.gl_pathv[me->pos],strerror(errno));
        free(me);
        return NULL;
    }
    me->chnid=curr_id;
    curr_id++;
    return me;
}


int medialib_GetChannelList(struct medialib_listinfo_st** listinfo, int* num)
{
    int i;
    int count=0;
    char path[PATHSIZE];
    glob_t globres;
    struct medialib_listinfo_st* listinfost;
    struct channel_context_st*   detinfo;


    for(i=0;i<CHNNR+1;i++)
    {
        channel[i].chnid=-1;
    }
    snprintf(path,PATHSIZE,"%s/*",conf.media_dir);
    if(glob(path,0,NULL,&globres))
    {
        syslog(LOG_ERR,"[medialib.c] medialib_GetChannelList()@glob()%s error : %s\n",path,strerror(errno));
        return -1;
    }
    listinfost=malloc(sizeof(struct medialib_listinfo_st)*globres.gl_pathc);
    if(listinfost==NULL)
    {
        syslog(LOG_ERR,"[medialib.c] medialib_GetChannelList()@malloc() error : %s\n", strerror(errno));
        exit(1);
    }
    for(i=0;i<globres.gl_pathc;i++)
    {
        detinfo= path2entry(globres.gl_pathv[i]);
        if(detinfo!=NULL)
        {
            syslog(LOG_DEBUG,"[medialib.c] path2entry() returned [%d %s]\n",detinfo->chnid,detinfo->desc);
            memcpy(channel+detinfo->chnid,detinfo, sizeof(struct channel_context_st));
            listinfost[i].chnid=detinfo->chnid;
            listinfost[i].info= strdup(detinfo->desc);      //TODO free
            count++;
        }
    }
    *listinfo= realloc(listinfost, sizeof(struct medialib_listinfo_st)*count);
    if(*listinfo==NULL)
    {
        syslog(LOG_ERR,"[medialib.c] medialib_GetChannelList()@realloc() error : %s\n", strerror(errno));
        return -1;
    }
    *num=count;
    return 0;
}
int medialib_FreeChannelList(struct medialib_listinfo_st* listinfo)
{
    free(listinfo);
    return 0;
}
ssize_t medialib_ReadChannel(chnid_t chnid,void* buf,size_t size)
{
    int tbf_num;
    ssize_t len;


    tbf_num= tbf_fetch_token(channel[chnid].tbf,(int)size);
    while(1)
    {
        len=pread(channel[chnid].fd,buf,tbf_num,channel[chnid].offset);
        if(len<0)
        {
            syslog(LOG_DEBUG,"[medialib.c] medialib_ReadChannel()@pread() error : %s\n", strerror(errno));
            open_next_media(chnid);
        }
        else if(len==0)
        {
            syslog(LOG_DEBUG,"[medialib.c] media file read end\n");
            open_next_media(chnid);
        }
        else
        {
            channel[chnid].offset+=len;
            break;
        }
    }
    if(tbf_num>len)
    {
        tbf_return_token(channel[chnid].tbf,tbf_num-len);
    }
    return len;
}