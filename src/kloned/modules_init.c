#include <klone/debug.h>
#include <klone/session.h>
#include <klone/config.h>
#include "context.h"

/* this function will be called just after app initialization and before 
   running any "useful" code; add here your initialization function calls */
int modules_init(context_t *ctx)
{
    return 0;
err:
    return ~0;
}
