#include "Queue.h"
#include <malloc.h>
#include "log.h"
/**
 * 初始化队列
 */
Queue* queue_init(int size, void* (*queue_free_func)()){
	Queue *queue = (Queue*)malloc(sizeof(Queue));
	queue->size = size;

	queue->next_to_write = 0;
	queue->next_to_read = 0;

	queue->tab = (void **)malloc(sizeof(*queue->tab) * size);

	//给二级指针的每个元素开辟空间
	for(int i = 0; i < size; i++) {
		queue->tab[i] = queue_free_func();
	}
	return queue;
}

/**
 * 销毁队列
 */
void queue_free(Queue* queue){
	for(int i = 0; i < queue->size; i++) {
		free(queue->tab[i]);
	}
	free(queue->tab);

	free(queue);
}

/**
 * 获取下一个索引位置
 */
int queue_get_next(Queue *queue, int current){
	return (current + 1) % queue->size;
}

/**
 * 队列压人元素  返回要写入位置的指针
 */
void* queue_push(Queue *queue) {
	int current = queue->next_to_write;
	queue->next_to_write = queue_get_next(queue,current);
	return queue->tab[current];
}


/**
 * 弹出元素
 */
void* queue_pop(Queue *queue) {
	int current = queue->next_to_read;
	queue->next_to_read = queue_get_next(queue,current);
	return queue->tab[current];
}



