/*
 * Copyright (c) 2005 by KoanLogic s.r.l. <http://www.koanlogic.com>
 * All rights reserved.
 *
 * This file is part of KLone, and as such it is subject to the license stated
 * in the LICENSE file which you have received as part of this distribution.
 *
 * $Id: modules_init.c,v 1.6 2005/11/23 17:27:01 tho Exp $
 */

#include "klone_conf.h"
#include <u/libu.h>
#include <klone/session.h>
#include <klone/context.h>

/* this function will be called just after app initialization and before 
   running any "useful" code; add here your initialization function calls */
int modules_init(void)
{
    dbg_err_if (ctx == NULL);

    return 0;
err:
    return ~0;
}
