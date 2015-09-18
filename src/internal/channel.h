#ifndef libcool_internal_list_
#define libcool_internal_list_

typedef struct list List
typedef struct list_node ListNode;

ListNode *listNode_Create(void *element);
List *list_Create();
void list_Destroy((void (*)(void **)) deleteNode);
void list_Append(List *list, void *element);
void *list_GetAtIndex(List *list, size_t index);
void *list_RemoveAtIndex(List *list, void *element, size_t index);

#endif // libcool_internal_list_
