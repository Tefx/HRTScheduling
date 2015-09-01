//
// Created by tefx on 8/27/15.
//

#include <malloc.h>
#include <string.h>
#include "utils.h"

list_node *wrap_list_node(void *data, list_node *pred, list_node *next) {
    list_node *node = malloc(sizeof(list_node));
    node->prev = pred;
    node->next = next;
    node->data = data;
    return node;
}

void deep_free_list_node(list_node *node) {
    if (node->data) free(node->data);
    free(node);
}


list *new_list() {
    list *l = malloc(sizeof(list));
    l->head = l->tail = NULL;
    return l;
}

void free_list(list *l) {
    list_node *tmp;

    while (not_empty(l)) {
        tmp = l->head;
        l->head = tmp->next;
        free(tmp);
    }
}

void deep_free_list(list *l) {
    list_node *tmp;

    while (not_empty(l)) {
        tmp = l->head;
        l->head = tmp->next;
        deep_free_list_node(tmp);
    }
}

bool is_empty(list *l) {
    return (l->head == NULL);
}

bool not_empty(list *l) {
    return (l->head);
}

void push(list *l, void *d) {
    if (is_empty(l))
        l->tail = l->head = wrap_list_node(d, NULL, NULL);
    else {
        l->head = wrap_list_node(d, NULL, l->head);
        l->head->next->prev = l->head;
    }
}

void *pop(list *l) {
    list_node *ret = l->head;
    void *d = NULL;

    if (ret) {
        d = ret->data;
        l->head = ret->next;
        free(ret);

        if (l->head)
            l->head->prev = NULL;
        else
            l->tail = NULL;
    }
    return d;
}

void *first(list *l) {
    return (l->head) ? l->head->data : NULL;
}

void push_right(list *l, void *d) {
    if (is_empty(l))
        l->head = l->tail = wrap_list_node(d, NULL, NULL);
    else {
        l->tail->next = wrap_list_node(d, l->tail, NULL);
        l->tail = l->tail->next;
    }
}

void *pop_right(list *l) {
    list_node *ret = l->tail;
    void *d = NULL;

    if (ret) {
        d = ret->data;
        l->tail = ret->prev;
        free(ret);

        if (l->tail)
            l->tail->next = NULL;
        else
            l->head = NULL;
    }
    return d;
}

void insert_after(list *l, list_node *p, void *data) {
    list_node *node;

    if (p == NULL)
        push(l, data);
    else if (p == l->tail)
        push_right(l, data);
    else {
        node = wrap_list_node(data, p, p->next);
        p->next->prev = node;
        p->next = node;
    }
}

void *delete_node(list *l, list_node *node) {
    void *data = node->data;

    if (node->prev == NULL)
        l->head = node->next;
    else
        node->prev->next = node->next;

    if (node->next == NULL)
        l->tail = node->prev;
    else
        node->next->prev = node->prev;

    return data;
}

list_node *find_node(list_node *start, bool(*compare)(const void *, const void *), const void *p) {
    list_node *tmp = start;
    while (tmp && !compare(tmp->data, p)) tmp = tmp->next;
    return tmp;
}

void insert_ordered(list *l, void *d, int (*compare)(const void *, const void *)) {
    list_node *p = l->head;

    if (p == NULL || compare(p->data, d) > 0) {
        push(l, d);
    } else {
        while (p != l->tail && compare(p->next->data, d) < 0)
            p = p->next;
        insert_after(l, p, d);
    }
}

void print_list(list *l, void(*print)(void *)) {
    list_node *tmp = l->head;

    while (tmp) {
        print(tmp->data);
        printf("=>");
        tmp = tmp->next;
    }
    printf("nil\n");
}

list *copy_while(list *l, bool(*filter)(const void *, const void *), const void *p) {
    list_node *tmp = l->head;
    list *nl = new_list();

    while (tmp && filter(tmp->data, p)) {
        push_right(nl, tmp->data);
        tmp = tmp->next;
    }

    return nl;
}

list *deep_copy_while(list *l, bool(*filter)(const void *, const void *), size_t s, const void *p) {
    list_node *tmp = l->head;
    void *copy = NULL;
    list *nl = new_list();

    while (tmp && filter(tmp->data, p)) {
        copy = malloc(s);
        memcpy(copy, tmp->data, s);
        push_right(nl, copy);
        tmp = tmp->next;
    }

    return nl;
}

void concat_before(list *l1, list *l0) {
    if (is_empty(l1)) {
        l1->head = l0->head;
        l1->tail = l0->tail;
    } else if (not_empty(l0)) {
        l1->head->prev = l0->tail;
        l0->tail->next = l1->head;
        l1->head = l0->head;
    }
    free(l0);
}