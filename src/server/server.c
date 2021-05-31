#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>




#include "../include/proto.h"
#include "server_conf.h"
#include "medialib.h"
#include "thr_list.h"
#include "thr_channel.h"


int sfd;
struct sockaddr_in client_addr;
struct server_conf_st conf=
        {
            .rcvport=DEFAULT_RCVPORT,
            .mgroup=DEFAULT_MGROUP,
            .media_dir=DEFAULT_MEIDADIR,
            .run_mode=DAEMON,
            .interface=DEFAULE_INTERFACE
        };


static struct medialib_listinfo_st* list;

static void print_help()
{

}

static int socket_init()
{
    struct ip_mreqn mreq;

    inet_pton(AF_INET,conf.mgroup,&mreq.imr_multiaddr);
    inet_pton(AF_INET,"0,0,0,0",&mreq.imr_address);
    mreq.imr_ifindex=(int)if_nametoindex(conf.interface);
    sfd=socket(AF_INET,SOCK_DGRAM,0);
    if(sfd<0)
    {
        syslog(LOG_ERR,"[server.c] socket_init()@socket() error : %s\n", strerror(errno));
        exit(1);
    }

    if(setsockopt(sfd,IPPROTO_IP,IP_MULTICAST_IF,&mreq,sizeof mreq)<0)
    {
        syslog(LOG_ERR,"[server.c] socket_init()@setsockopt() broadcast error : %s\n", strerror(errno));
        exit(1);
    }

    client_addr.sin_family=AF_INET;
    client_addr.sin_port= htons(atoi(conf.rcvport));
    inet_pton(AF_INET,conf.mgroup,&client_addr.sin_addr.s_addr);
    return 0;
}

static void daemon_exit(int s)
{
    thr_list_destroy();
    thr_channel_destroy_all();
    medialib_FreeChannelList(list);
    syslog(LOG_WARNING,"[server.c] catch signal %d",s);
    closelog();
    exit(0);
}

static int daemonize()
{
    int fd;
    pid_t pid;


    pid=fork();
    if(pid<0)
    {
        syslog(LOG_ERR,"[server.c] daemonize()@fork() error : %s\n",strerror(errno));
        return -1;
    }
    if(pid>0)
    {
        exit(0);
    }
    fd=open("/Users/cl/tmp/nu",O_RDWR);
    if(fd<0)
    {
        syslog(LOG_WARNING,"[server.c] daemonize()@open() error: %s\n", strerror(errno));
    }
    else
    {
        dup2(fd,0);
        dup2(fd,1);
        dup2(fd,2);
        if (fd>2)
        {
            close(fd);
        }
    }
    setsid();
    chdir("/Users/cl");
    umask(0);
    return 0;
}


int main(int argc,char* argv[])
{
    int c;
    struct sigaction action;
    int list_size;
    int err;
    int i;


    action.sa_flags=0;
    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask,SIGTERM);
    sigaddset(&action.sa_mask,SIGINT);
    sigaddset(&action.sa_mask,SIGQUIT);
    action.sa_handler=daemon_exit;
    sigaction(SIGTERM,&action,NULL);
    sigaction(SIGQUIT,&action,NULL);
    sigaction(SIGINT,&action,NULL);
    openlog("lzfm",LOG_PID | LOG_PERROR,LOG_DAEMON);
    while(1)
    {
        c=getopt(argc,argv,"M:P:FD:I:H");
        if(c<0)
        {
            break;
        }
        switch (c)
        {
            case 'M' :
            {
                conf.mgroup=optarg;
                break;
            }
            case 'P' :
            {
                conf.rcvport=optarg;
                break;
            }
            case 'F' :
            {
                conf.run_mode=FOREGROUND;
                break;
            }
            case 'D' :
            {
                conf.media_dir=optarg;
                break;
            }
            case 'I' :
            {
                conf.interface=optarg;
                break;
            }
            case 'H' :
            {
                print_help();
                break;
            }
            default:
            {
                abort();
            }
        }
    }
    if(conf.run_mode==DAEMON)
    {
        if(daemonize() != 0)
        {
            exit(1);
        }
    }
    else if(conf.run_mode==FOREGROUND)
    {

    }
    else
    {
        syslog(LOG_ERR,"[server.c] argument illegal(unknown mode)\n");
        exit(1);
    }
    socket_init();
    err= medialib_GetChannelList(&list,&list_size);
    if(err)
    {
        syslog(LOG_ERR,"[server.c] medialib_GetChannelList()\n");
        exit(1);
    }
    err=thr_list_create(list,list_size);
    if(err)
    {
        syslog(LOG_ERR,"[server.c] thr_list_create()\n");
        exit(1);
    }
    for(i=0;i<list_size;i++)
    {
        err=thr_channel_create(list+i);
        if(err)
        {
            syslog(LOG_ERR,"[server.c] thr_channel_create() : %s\n", strerror(err));
            exit(1);
        }
    }
    syslog(LOG_DEBUG,"[server.c] %d threads create\n",list_size);
    while(1)
    {
        pause();
    }

    closelog();
    exit(0);
}
