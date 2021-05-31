//
// Created by 🍎 on 2021/5/27.
//

#ifndef LIZHI_CLIENT_H
#define LIZHI_CLIENT_H


struct client_conf_st
{
    char* recport;
    char* mgroup;
    char* player;
};

extern struct client_conf_st conf;

#endif //LIZHI_CLIENT_H
