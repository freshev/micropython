#ifndef __SPEECH_TRANS_H__
#define __SPEECH_TRANS_H__

#include <stdio.h>
#include "core_list.h"

typedef struct {
    char *filename;
    struct core_list_head linked_node;
} play_node_t;

/*Convert the corpus name into a file and add it to the linked list*/
void name_to_speech(const char *value, char *format, struct core_list_head *list);
/*Convert money into corpus files and add them to the linked list*/
void money_to_speech(const char *value, char *format, struct core_list_head *list);

#endif
