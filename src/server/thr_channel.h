//
// Created by 🍎 on 2021/5/27.
//

#ifndef LIZHI_THR_CHANNEL_H
#define LIZHI_THR_CHANNEL_H

#include "medialib.h"

int thr_channel_create(struct medialib_listinfo_st* );
int thr_channel_destroy(struct medialib_listinfo_st*);
int thr_channel_destroy_all();

#endif //LIZHI_THR_CHANNEL_H
