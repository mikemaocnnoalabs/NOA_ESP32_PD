/*
  NOA_list.cpp - NOA arduino list functions
  Copyright 2012 NOA Labs
  Copyright 2022 Mike Mao
  Released under an MIT license. See LICENSE file.
*/
#include "NOA_public.h"
#include "NOA_list.h"

#define LIST_APPLY_FLAG     (0x55aa33cc)

static Node_t *nodeCheck(uint8_t *pLoad);
static Node_t *nodeFind(ListHandler_t *head, uint8_t *pLoad);

/*******************************************************************************
Function:   list_init
Feature:    init list struct
Parateters: head: point for list head
return:     null
*******************************************************************************/
void NOA_list_init(ListHandler_t *head) {
  head->count = 0;
  head->node = NULL;
}

/*******************************************************************************
Function:   list_nodeApply
Feature:    apply node
Parateters: payloadSize: node size
return:     list head node point
*******************************************************************************/
void *NOA_list_nodeApply(uint32_t payloadSize) {
  Node_t *node = NULL;
  node = (Node_t *)memory_apply(payloadSize + sizeof(Node_t));
  ERRR(node == NULL, return NULL);

  /*apply node size success*/
  node->front = (Node_t *)(LIST_APPLY_FLAG ^ (int)node->payload);

  return node->payload;
}

/*******************************************************************************
Function:   list_nodeDelete
Feature:    delete node
Parateters: node: node list point
            payload: node to delete
return:     null
*******************************************************************************/
void NOA_list_nodeDelete(ListHandler_t *head, void *payload) {
  Node_t *node = nodeCheck((uint8_t *)payload);
  if (node != NULL) {
    memory_release(node);
    node = NULL;
    return;
  }

  ERRR((head == NULL || head->node == NULL), return);
  node = nodeFind(head, (uint8_t *)payload);
  if (node != NULL) {
    if (head->node == node) {
      if (node->next == head->node) { // last node
        head->node = NULL;
      } else {
        head->node = node->next;
      }
    }
    node->front->next = node->next;
    node->next->front = node->front;
    if (node != NULL) {
      memory_release(node);
      node = NULL;
    }
    head->count--;
  }
}

/*******************************************************************************
Function:   list_delete
Feature:    delete list
Parateters: head: desk list head point
return:     null
*******************************************************************************/
void NOA_list_delete(ListHandler_t *head) {
  Node_t *node = NULL;
  Node_t *tail = head->node;

  ERRR((head == NULL || tail == NULL), return);
  do {
    node = tail;
    tail = tail->next;
    if (node != NULL) {
      memory_release(node);
      node = NULL;
    }
  } while (tail != head->node && tail != NULL);

  NOA_list_init(head);
}

/*******************************************************************************
Function:   list_topInsert
Feature:    insert node to list from top
Parateters: head: desk list head point
            payload: node
return:     insert result: 0 for success or fail
*******************************************************************************/
int NOA_list_topInsert(ListHandler_t *head, void *payload) {
  int i = NOA_list_bottomInsert(head, payload);

  if (i == 0) {
    head->node = head->node->front;
  }

  return i;
}

/*******************************************************************************
Function:   list_bottomInsert
Feature:    insert node to list from bottom
Parateters: head: desk list head point
            payload: node
return:     insert result: 0 for success or fail
*******************************************************************************/
int NOA_list_bottomInsert(ListHandler_t *head, void *payload) {
  if (head == NULL) {
    return 1;
  }

  Node_t *node = nodeCheck((uint8_t *)payload);
  if (node == NULL) {
    node = nodeFind(head, (uint8_t *)payload);
    if (node == NULL) {
      return 2;
    } else {
      node->front->next = node->next;
      node->next->front = node->front;
      head->count--;
    }
  }

  if (head->node == NULL) {
    head->node = node;
    node->front = node;
    node->next = node;
  } else {
    Node_t *tail = head->node;

    node->front = tail->front;
    node->next = head->node;
    tail->front->next = node;
    tail->front = node;
  }
  head->count++;

  return 0;
}

/*******************************************************************************
Function:   list_ConfInsert
Feature:    compare a node in list
Parateters: head: desk list head point
            cmp: compare function
            payload: node
return:     insert result: 0 for success or fail
*******************************************************************************/
int NOA_list_ConfInsert(ListHandler_t *head, cmpFun cmp, void *payload) {
  if (head == NULL) {
    return 1;
  }

  Node_t *node = nodeCheck((uint8_t *)payload);
  if (node == NULL) {
    node = nodeFind(head, (uint8_t *)payload);
    if (node == NULL) {
      return 2;
    } else {
      node->front->next = node->next;
      node->next->front = node->front;
      head->count--;
    }
  }

  if (head->node == NULL) {
    head->node = node;
    node->front = node;
    node->next = node;
  } else {
    Node_t *tail = head->node;
    do {
      if (0 == cmp(tail->payload, payload)) {
        if (tail == head->node) { // check from head node
          head->node = node;
        }
        break;
      }
      tail = tail->next;
    } while (tail != head->node);

    node->front = tail->front;
    node->next = tail;
    tail->front->next = node;
    tail->front = node;
  }
  head->count++;

  return 0;
}

/*******************************************************************************
Function:   list_nextData
Feature:    get next node point of current node
Parateters: head: desk list head point
            payload: current node
return:     point of next node or null
*******************************************************************************/
void *NOA_list_nextData(ListHandler_t *head, void *payload) {
  Node_t *node = nodeFind(head, (uint8_t *)payload);

  if (node == NULL) {
    if (head != NULL && head->node != NULL) {
      return head->node->payload;
    }
  } else if (node->next != head->node) {
    return node->next->payload;
  }

  return NULL;
}

/*******************************************************************************
Function:   list_find
Feature:    find node in list with some value and compare function
Parateters: head: desk list head point
            cmp: compare function
            conVal: value
return:     point of next node or null
*******************************************************************************/
void *NOA_list_find(ListHandler_t *head, cmpFun cmp, void *conVal) {
  Node_t *tail = head->node;

  ERRR((head == NULL || tail == NULL), return NULL);
  do {
    if (0 == cmp(tail->payload, conVal)) {
      return tail->payload;
    }
    tail = tail->next;
  } while (tail != head->node);

  return NULL;
}

/*******************************************************************************
Function:   list_trans
Feature:    trans list
Parateters: head: desk list head point
            fun: process function
            optPoint: option point
return:     null
*******************************************************************************/
void NOA_list_trans(ListHandler_t *head, processorFun fun, void *optPoint) {
  Node_t *tail = head->node;
  void *payload = NULL;

  ERRR((head == NULL || tail == NULL || fun == NULL), return);
  do {
    payload = tail->payload;
    tail = tail->next;
    if (0 == fun(payload, optPoint)) {
      break;
    }
  } while (tail != head->node && head->node != NULL);
}

/*******************************************************************************
Function:   nodeCheck
Feature:    check node is save for list or not
Parateters: pLoad: value point
return:     point to node or null
*******************************************************************************/
static Node_t *nodeCheck(uint8_t *pLoad) {
  Node_t *node = (Node_t *)(((uint8_t *)pLoad) - sizeof(Node_t));

  if (pLoad != NULL && (LIST_APPLY_FLAG ^ (int)node->front) == (int)pLoad) {
    return node;
  }

  return NULL;
}

/*******************************************************************************
Function:   nodeFind
Feature:    find node in list
Parateters: head: desk list head point
            pLoad: node point
return:     point to node in list or null
*******************************************************************************/
static Node_t *nodeFind(ListHandler_t *head, uint8_t *pLoad) {
  ERRR((head == NULL || head->node == NULL || pLoad == NULL), return NULL);

  Node_t *node = (Node_t *)(((uint8_t *)pLoad) - sizeof(Node_t));
  Node_t *tail = head->node;
  do {
    if (node == tail) {
      return node;
    }
    tail = tail->next;
  } while (tail != head->node && tail != NULL);

  return NULL;
}
/*****************************Copyright NOA******************************/

