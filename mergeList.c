#include "stdio.h"
#include "stdlib.h"
#include "string.h"

typedef struct _node {
	int data;
	struct _node *pNext;
}node;

node *mergeList(node *pHead1, node *pHead2) {
    if (pHead1 == NULL && pHead2 == NULL) {
	    return NULL;
	}   
	node *pNode;
    node *pHead = pNode;
    node *tmpNode;
    while (pHead1 != NULL || pHead2 != NULL) {
	    if (pHead1 == NULL && pHead2 != NULL) {
			tmpNode = pHead2;
			pHead2 = pHead2->pNext;
		} else if (pHead1 != NULL && pHead2 == NULL) {
			tmpNode = pHead1;
			pHead1 = pHead1->pNext;
		} else {
			if (pHead1->data > pHead2->data) {
				tmpNode = pHead2;
				pHead2 = pHead2->pNext;
			} else {
				tmpNode = pHead1;
				pHead1 = pHead1->pNext;
			}   
		}   
	    if (pNode == NULL) {
			pHead = pNode = tmpNode;
		} else {
			pNode->pNext = tmpNode;
			pNode = tmpNode;
		}   
	}   
    return pHead;
}

node *merge(node *pHead1, node *pHead2) {
	if (pHead1 == NULL) {
		return pHead2;
	} else if (pHead2 == NULL) {
		return pHead1;
	}

	node *pHead = NULL;

	if (pHead1->data > pHead2->data) {
		pHead = pHead2;
		pHead->pNext = merge(pHead1, pHead2->pNext);
	} else {
		pHead = pHead1;
		pHead->pNext = merge(pHead1->pNext, pHead2);
	}

	return pHead;
}

int main(void) {
	node *pHead1 = (node *)malloc(sizeof(node));
	pHead1->data = 10;
	pHead1->pNext = NULL;

	node *pHead2 = (node *)malloc(sizeof(node));
	pHead2->data = 8;
	pHead2->pNext = NULL;

	node *pHead = merge(pHead1, pHead2);
	printf("%d\n", pHead->pNext->data);
	return 0;
}
