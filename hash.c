#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"

Buckets *initBuckets() {
	Buckets *bucket = (Buckets *)malloc(sizeof(Buckets));
	bucket->buckets = buckets;
	bucket->size = MAX_SIZE;
	bucket->element_num = 0;

	return bucket;
}

Node *initNode(void *key, void *data) {

	Node *node = (Node *)malloc(sizeof(Node));
	node->key = key;
	node->data = data;
	node->next = NULL;

	return node;
}

int getHashKey(void *key) {
	int hashKey = 0, len = strlen((char *)key), i = 0;
	char *k = (char *)key;
	for (i=0; i<len; ++i) {
		hashKey += k[i];
	}
	return hashKey % MAX_SIZE;
}

int inNode(Buckets *B, void *key, void *data) {
	if (key == NULL || data == NULL) {
		return -1;
	}
	int hashKey = getHashKey(key);
	Node *node = initNode(key, data);
	if (buckets[hashKey]) {
		node->next = buckets[hashKey];	
	}
	buckets[hashKey] = node;
	B->element_num++;

	return 0;
}

Node *getNode(void *key) {
	if (key == NULL) {
		return NULL;
	}
	int hashKey = getHashKey(key);
	while (buckets[hashKey] != NULL) {
		if (!strcmp((char *)(buckets[hashKey]->key), (char *)key)) {
			return buckets[hashKey];
		}
	}
	return NULL;
}

int delNode(Buckets *B, void *key) {
	if (key == NULL) {
		return -1;
	}
	int hashKey = getHashKey(key);

	while (buckets[hashKey] != NULL) {
		if (!strcmp((char *)(buckets[hashKey]->key), (char *)key)) {
			Node *tmp = buckets[hashKey];
			buckets[hashKey] = buckets[hashKey]->next;
			free(tmp);
			tmp = NULL;
			B->element_num--;
			return 0;
		}
	}
	return -1;
}

void freeNode(Buckets *bucket) {
	int i = 0;
	Node **node = bucket->buckets;
	for (i=0; i<bucket->size; ++i) {
		Node *tmpNode = node[i];
		while (tmpNode != NULL) {
			Node *tmp = tmpNode;
			tmpNode = tmpNode->next;
			free(tmp);
			tmp = NULL;
		}	
	}
	free(bucket);
	bucket = NULL;
}

int main(void) {
	Buckets *B = initBuckets();
	inNode(B, "solariens", "this is a demo");
	inNode(B, "solariens", "this is a demo");
	delNode(B, "solariens");
	printf("%d\n", B->element_num);
	freeNode(B);
	return 0;
}
