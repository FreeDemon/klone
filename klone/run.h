/*
 * Copyright (c) 2005 by KoanLogic s.r.l. <http://www.koanlogic.com>
 * All rights reserved.
 *
 * This file is part of KLone, and as such it is subject to the license stated
 * in the LICENSE file which you have received as part of this distribution.
 *
 * $Id: run.h,v 1.3 2005/11/23 17:27:01 tho Exp $
 */

#ifndef _KLONE_RUN_H_
#define _KLONE_RUN_H_

#include <klone/request.h>
#include <klone/response.h>

/* run a dynamic object page */
int run_page(const char *, request_t*, response_t*);

#endif
