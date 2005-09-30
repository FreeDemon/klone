#ifndef _SERVER_BACKEND_H_
#define _SERVER_BACKEND_H_
#include <klone/queue.h>

struct server_s;
struct config_s;

/* define page list */
LIST_HEAD(backends_s, backend_s);

struct backend_s
{
    /* statically initialized values */
    const char *proto;
    int (*cb_init)(struct backend_s *);
    int (*cb_serve)(struct backend_s *, int);
    int (*cb_term)(struct backend_s *);

    /* runtime values */
    struct server_s *server;
    struct config_s *config;
    struct addr_s *addr; 
    int model;
    int ld;
    void *arg;
    LIST_ENTRY(backend_s) np;  /* next & prev pointers         */
};

typedef struct backend_s backend_t;
typedef struct backends_s backends_t; /* backend_t list        */

#define BACKEND_STATIC_INITIALIZER(proto, init, connect, term)  \
    {                                                           \
        proto,                                                  \
        init,                                                   \
        connect,                                                \
        term,                                                   \
        NULL,   /* server  */                                   \
        NULL,   /* config  */                                   \
        NULL,   /* addr    */                                   \
        0,      /* model   */                                   \
        -1,     /* ld      */                                   \
        NULL,   /* arg     */                                   \
        LIST_ENTRY_NULL                                         \
    }


extern backend_t *backend_list[];

int backend_create(const char *name, struct config_s *, backend_t **);
int backend_serve(backend_t *, int fd);
int backend_free(backend_t *);


#endif
