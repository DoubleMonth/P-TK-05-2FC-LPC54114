#include <printk.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>
#include <utils.h>
#include "klist.h"
#include "object.h"

static LIST_HEAD(object_list);

void object_attach(struct object *o, const char *name)
{
    o->name = name;

//    taskENTER_CRITICAL();
    list_add_tail(&o->entry, &object_list);
//    taskEXIT_CRITICAL();
}

void object_detach(struct object *o)
{
//    taskENTER_CRITICAL();
    list_del(&o->entry);
//    taskEXIT_CRITICAL();
}

object_t object_find(const char *name)
{
    struct list_head *iter;

    list_for_each(iter, &object_list)
    {
        struct object *o = container_of(iter, struct object, entry);
        if (!strcmp(o->name, name))
            return o;
    }
    return NULL;
}
