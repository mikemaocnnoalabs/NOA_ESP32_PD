/*
  NOA_list.h - NOA arduino list functions
  Copyright 2012 NOA Labs
  Copyright 2022 Mike Mao
  Released under an MIT license. See LICENSE file. 
*/
#ifndef __NOA_LIST_H
#define __NOA_LIST_H

#ifdef __cplusplus
extern "C" {
#endif

#define ERRR(conf, ret)     do\
                            {\
                                if (conf)\
                                {\
                                    ret;\
                                }\
                            } while(0)

typedef struct node_t {
  struct node_t *front;
  struct node_t *next;
  uint8_t   payload[];
}Node_t;

typedef struct {
  int16_t   count;  
  uint16_t   rsv;
  Node_t  *node;
} ListHandler_t;

typedef uint8_t (*cmpFun)(void *src, void *dest);   //
typedef uint8_t (*processorFun)(void *listPaycoad, void *chagePoint);   //

void  NOA_list_init(ListHandler_t *head);
int   NOA_list_topInsert(ListHandler_t *head, void *payload);
int   NOA_list_bottomInsert(ListHandler_t *head, void *payload);
int   NOA_list_ConfInsert(ListHandler_t *head, cmpFun cmp, void *payload);
void  *NOA_list_nodeApply(uint32_t payloadSize);
void  *NOA_list_nextData(ListHandler_t *head, void *payload); // get next node data
void  *NOA_list_find(ListHandler_t *head, cmpFun cmp, void *conVal);

void  NOA_list_trans(ListHandler_t *head, processorFun fun, void *optPoint);
void  NOA_list_nodeDelete(ListHandler_t *head, void *payload);
void  NOA_list_delete(ListHandler_t *head);

#ifdef __cplusplus
}
#endif

#endif//__NOA_LIST_H
/******************************************************************************/

