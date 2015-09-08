#include <pthread.h>

#include "list.h"

struct list_node;

struct list_node {
    void *element;
    list_node *next;
}

struct list {
    list_node *head;
    list_node *tail;
    size_t size;
    pthread_mutex_t mutex;
    void *(delete)(void **element);
};

ListNode *
listNode_Create(void *element)
{
    ListNode *result = (ListNode *) malloc(sizeof(ListNode));
    result->element = element;
    result->next = NULL;
    return result;
}

List *
list_Create((void *(void **) delete))
{
    List *result = (List *) malloc(sizeof(List));
    result->size = 0;
    result->head = result->tail = NULL;
    result->delete = delete;
    pthread_mutex_init(&result->mutex, NULL);
    return result;
}

void
list_Destroy(List **listP)
{
    List *list = (List *) *listP;

    ListNode *current = list->head;
    for (size_t i = 0; i < list->size; i++) {
        delete(&node->element);
        current = current->next;
    }

    free(list);
    *listP = NULL;
}

void
list_Append(List *list, void *element)
{
    ListNode *newNode = listNode_Create(element);

    pthread_mutex_lock(&mutex);

    if (list->tail == NULL) {
        head = tail = newNode;
    } else {
        list->tail->next = newNode;
        list->tail = newNode;
    }

    list->size++;

    pthread_mutex_unlock(&mutex);
}

void *
list_GetAtIndex(List *list, size_t index)
{
    ListNode *element = NULL;

    pthread_mutex_lock(&mutex);

    // TODO: maybe reconsider this behavior...
    index = (index % list->size);

    ListNode *current = list->head;
    size_t i = 0;
    while (i < index) {
        current = current->next;
        i++;
    }

    element = current;

    pthread_mutex_unlock(&mutex);

    return element;
}

void *
list_RemoveAtIndex(List *list, void *element, size_t index)
{
    pthread_mutex_lock(&mutex);

    if (list->head == NULL) {
        pthread_mutex_unlock(&mutex);
        return NULL;
    } else {

        index = index % list->size;
        ListNode *target = list->head;

        if (index == 0) {
            list->head = list->next;
        } else {
            size_t i = 0;
            ListNode *prev = NULL;

            while (i < index) {
                prev = target;
                target = target->next;
                i++;
            }
            prev->next = target->next;

            if (index == (list->size - 1)) {
                list->tail = prev; // move back
            }
        }

        list->size--;

        pthread_mutex_unlock(&mutex);

        return target;
    }
}
