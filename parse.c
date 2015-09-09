//
// Created by tefx on 9/9/15.
//

#include <stdio.h>
#include <gc/gc.h>
#include <string.h>
#include "parse.h"
#include "cJSON/cJSON.h"

char *read_entire_file(char *filename) {
    FILE *f = fopen(filename, "r");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *string = GC_MALLOC_ATOMIC((size_t) fsize + 1);
    if (!fread(string, (size_t) fsize, 1, f)) {
        fclose(f);
        return NULL;
    }

    fclose(f);
    string[fsize] = 0;
    return string;
}

char *clone_str(char *orig) {
    char *str = GC_MALLOC_ATOMIC(sizeof(char) * (strlen(orig) + 1));
    strcpy(str, orig);
    return str;
}

char **read_argv(cJSON *json) {
    char **argv;
    task_hrts size;

    size = cJSON_GetArraySize(json);
    argv = GC_MALLOC(sizeof(char *) * (size + 1));
    argv[size] = NULL;

    for (task_hrts i = 0; i < size; i++) {
        argv[i] = clone_str(cJSON_GetArrayItem(json, i)->valuestring);
    }
    return argv;
}


task_info *parse_config(char *filename, task_hrts *num) {
    char *json_string = read_entire_file(filename);
    cJSON *root = cJSON_Parse(json_string);
    cJSON *item;
    task_info *ti;

    *num = cJSON_GetArraySize(root);
    ti = GC_MALLOC(sizeof(task_info) * (*num));
    for (task_hrts i = 0; i < *num; i++) {
        item = cJSON_GetArrayItem(root, (int) i);
        ti[i].primary_path = clone_str(cJSON_GetObjectItem(item, "primary_path")->valuestring);
        ti[i].alternate_path = clone_str(cJSON_GetObjectItem(item, "alternate_path")->valuestring);
        ti[i].primary_argv = read_argv(cJSON_GetObjectItem(item, "primary_argv"));
        ti[i].alternate_argv = read_argv(cJSON_GetObjectItem(item, "alternate_argv"));
        ti[i].period = cJSON_GetObjectItem(item, "period")->valueint;
        ti[i].primary_time = cJSON_GetObjectItem(item, "primary_time")->valueint;
        ti[i].alternate_time = cJSON_GetObjectItem(item, "alternate_time")->valueint;
    }

    return ti;
}