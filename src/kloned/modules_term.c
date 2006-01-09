/*
 * Copyright (c) 2005, 2006 by KoanLogic s.r.l. <http://www.koanlogic.com>
 * All rights reserved.
 *
 * This file is part of KLone, and as such it is subject to the license stated
 * in the LICENSE file which you have received as part of this distribution.
 *
 * $Id: modules_term.c,v 1.7 2006/01/09 12:38:38 tat Exp $
 */

#include "klone_conf.h"
#include <u/libu.h>
#include <klone/session.h>
#include <klone/context.h>

/* this function will be called just before closing; put here your 
   destructors */
int modules_term(void)
{
    dbg_err_if (ctx == NULL);
    return 0;
err:
    return ~0;
}
