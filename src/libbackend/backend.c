#include <unistd.h>
#include <klone/debug.h>
#include <klone/utils.h>
#include <klone/backend.h>
#include <klone/server.h>
#include <klone/config.h>
#include "conf.h"

extern backend_t be_http;

#ifdef HAVE_OPENSSL
extern backend_t be_https;
#endif

backend_t *backend_list[] = { 
    &be_http, 
    #ifdef HAVE_OPENSSL
    &be_https, 
    #endif
    0 };


static int backend_set_model(backend_t *be, const char *v)
{
    if(!strcasecmp(v, "fork"))
        be->model = SERVER_MODEL_FORK;
    else if(!strcasecmp(v, "iterative"))
        be->model = SERVER_MODEL_ITERATIVE;
    else
        warn_err("unknown server model [%s]", v);
    return 0;
err:
    return ~0;
}

int backend_create(const char *proto, config_t *config, backend_t **pbe)
{
    backend_t *be = NULL, **pp;
    const char *v;

    be = u_calloc(sizeof(backend_t));
    dbg_err_if(be == NULL);

    if((v = config_get_subkey_value(config, "model")) != NULL)
        dbg_err_if(backend_set_model(be, v));

    /* look for the requested backend */
    for(pp = backend_list; *pp != NULL; ++pp)
    {
        if(strcasecmp((*pp)->proto, proto) == 0)
        {   /* found */
            memcpy(be, *pp, sizeof(backend_t));
            be->config = config;
            if(be->cb_init)
                dbg_err_if(be->cb_init(be));
            *pbe = be;
            return 0;
        }
    }

    warn_err("backend type \"%s\" not found", proto);

    return 0;
err:
    if(be)
        u_free(be);
    return ~0;
}

int backend_serve(backend_t *be, int fd)
{
    dbg_err_if(be->cb_serve == NULL);

    be->cb_serve(be, fd);

    return 0;
err:
    return ~0;
}

int backend_free(backend_t *be)
{
    if(be)
    {
        if(be->cb_term)
            be->cb_term(be);
        u_free(be);
    }
    return 0;
}

