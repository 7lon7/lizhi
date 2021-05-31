//
// Created by 🍎 on 2021/5/27.
//

#ifndef LIZHI_PROTO_H
#define LIZHI_PROTO_H


#include "site_type.h"

#define DEFAULT_MGROUP          "224.2.2.2"
#define DEFAULT_RCVPORT         "11111"
#define CHNNR                   100
#define LISTCHNID               0
#define MINCHNID                1
#define MAXCHNID                (MINCHNID+CHNNR-1)
#define MAX_CHN_MSG             (65536-20-8)
#define MAX_DATA                (MAX_CHN_MSG-sizeof(chnid_t))
#define MAX_LIST_MSG            (65536-20-8)
#define MAX_INFO                (MAX_LIST_MSG-sizeof(chnid_t))

struct msg_channel_st
{
    chnid_t chnid;
    uint8_t data[1];
}__attribute__((packed));


struct msg_channel_info_st
{
    chnid_t chnid;
    int len;
    uint8_t info[1];
}__attribute__((packed));
/*
 * channel信息
 * chnid    频道号         1
 * len      信息长度        20
 * info     频道信息        "pop music"
*/




struct msg_list_st
{
    chnid_t chnid;
    struct msg_channel_info_st chninfo[1];
}__attribute__((packed));


/*
 * 频道信息列表
 *
*/


#endif //LIZHI_PROTO_H
