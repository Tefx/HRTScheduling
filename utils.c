//
// Created by tefx on 8/27/15.
//

#include <gc/gc.h>
#include <string.h>
#include <stdio.h>
#include "utils.h"

list_node *wrap_list_node(void *data, list_node *pred, list_node *next) {
    list_node *node = GC_MALLOC(sizeof(list_node));
    node->prev = pred;
    node->next = next;
    node->data = data;
    return node;
}

list *new_list() {
    list *l = GC_MALLOC(sizeof(list));
    l->head = l->tail = NULL;
    return l;
}

bool is_empty(list *l) {
    return (l->head == NULL);
}

bool not_empty(list *l) {
    return (l->head != NULL);
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

list_node *find_node_r(list_node *start, bool(*compare)(const void *, const void *), const void *p) {
    list_node *tmp = start;
    while (tmp && !compare(tmp->data, p)) tmp = tmp->next;
    return tmp;
}

void insert_ordered(list *l, void *d, int (*compare)(const void *, const void *)) {
    list_node *tmp = l->head;

    if (tmp == NULL || compare(tmp->data, d) > 0) {
        push(l, d);
    } else {
        while (tmp != l->tail && compare(tmp->next->data, d) < 0)
            tmp = tmp->next;
        insert_after(l, tmp, d);
    }
}

list *copy_until_r(list *l, bool(*filter)(const void *, const void *), size_t s, const void *p) {
    list_node *tmp = l->head;
    void *copy = NULL;
    list *nl = new_list();

    while (tmp && (!filter(tmp->data, p))) {
        copy = GC_MALLOC(s);
        memcpy(copy, tmp->data, s);
        push_right(nl, copy);
        tmp = tmp->next;
    }

    return nl;
}

void insert_ordered_r(list *l, void *d, int(*compare)(const void *, const void *, const void *), const void *p) {
    list_node *tmp = l->head;

    if (tmp == NULL || compare(tmp->data, d, p) > 0) {
        push(l, d);
    } else {
        while (tmp != l->tail && compare(tmp->next->data, d, p) < 0)
            tmp = tmp->next;
        insert_after(l, tmp, d);
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

void concat_before(list *l1, list *l0) {
    if (is_empty(l1)) {
        l1->head = l0->head;
        l1->tail = l0->tail;
    } else if (not_empty(l0)) {
        l1->head->prev = l0->tail;
        l0->tail->next = l1->head;
        l1->head = l0->head;
    }
}