//
// Created by 🍎 on 2021/5/27.
//

#ifndef LIZHI_SERVER_CONF_H
#define LIZHI_SERVER_CONF_H

#define DEFAULT_MEIDADIR    "/Users/cl/media"
#define DEFAULE_INTERFACE   "eth0"

#include <sys/socket.h>

enum RUN_MODE
{
    DAEMON=1,
    FOREGROUND
};

struct server_conf_st
{
    char* rcvport;
    char* mgroup;
    char* media_dir;
    enum RUN_MODE run_mode;
    char* interface;
};

extern struct server_conf_st conf;
extern int sfd;
extern struct sockaddr_in client_addr;

#endif //LIZHI_SERVER_CONF_H
