//
// Created by tefx on 8/27/15.
//

#ifndef HRTSCHEDULING_UTILS_H
#define HRTSCHEDULING_UTILS_H

#include <stdbool.h>

typedef struct list_node_struct {
    void *data;
    struct list_node_struct *prev;
    struct list_node_struct *next;
} list_node;

#define head_of(x) ((x)->head)
#define tail_of(x) ((x)->tail)
#define data_of(x) ((x)->data)

typedef struct {
    list_node *head;
    list_node *tail;
} list;

list *new_list();

void free_list(list *);

void deep_free_list(list *);

bool is_empty(list *);

bool not_empty(list *);

void push(list *, void *);

void *pop(list *);

void push_right(list *, void *);

void *pop_right(list *);

void *first(list *);

void insert_ordered(list *, void *, int(*)(const void *, const void *));

void insert_after(list *, list_node *, void *);

void *delete_node(list *, list_node *);

list_node *find_node(list_node *, bool(*)(const void *, const void *), const void *);

list *copy_while(list *, bool(*)(const void *, const void *), const void *);

list *deep_copy_while(list *, bool(*)(const void *, const void *), size_t, const void *);

void concat_before(list *, list *);

void print_list(list *, void(*)(void *));


#define COMPARE(x, y, t, field) (((((t*)(x))->field)>(((t*)(y))->field))?1:((((t*)(x))->field==((t*)(y))->field)?0:-1))

#endif //HRTSCHEDULING_UTILS_H
