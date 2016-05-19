#ifndef HASH_H
#define HASH_H

#define MAX_SIZE 5

typedef struct _node {
	void *key;
	void *data;
	struct _node *next;
}Node;

typedef struct _buckets {
	Node **buckets;
	int size;
	int element_num;
}Buckets;

Node *buckets[MAX_SIZE];

Buckets *initBuckets();
Node *initNode(void *key, void *data);
int getHashKey(void *key);
int inNode(Buckets *B, void *key, void *data);
Node *getNode(void *key);
int delNode(Buckets *B, void *key)
void freeNode(Buckets *bucket)

#endif
