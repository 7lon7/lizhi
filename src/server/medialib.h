//
// Created by 🍎 on 2021/5/27.
//

#ifndef LIZHI_MEDIALIB_H
#define LIZHI_MEDIALIB_H



#include <stdio.h>


#include "../include/proto.h"


#define MP3_BITRATE 65536

struct medialib_listinfo_st
{
    chnid_t chnid;
    char* info;
};


int medialib_GetChannelList(struct medialib_listinfo_st** listinfo, int* size);
int medialib_FreeChannelList(struct medialib_listinfo_st* listinfo);
ssize_t medialib_ReadChannel(chnid_t chnid,void* pos,size_t size);


#endif //LIZHI_MEDIALIB_H
