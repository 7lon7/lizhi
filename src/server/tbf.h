//
// Created by 🍎 on 2021/5/27.
//

#ifndef LIZHI_TBF_H
#define LIZHI_TBF_H

#define TBF_MAX 1024
typedef void tbf_t;

tbf_t* tbf_init(int cps,int burst);
int tbf_fetch_token(tbf_t* tbf ,int num);
int tbf_return_token(tbf_t* tbf,int num);
int tbf_destroy(tbf_t* tbf);

#endif //LIZHI_TBF_H
