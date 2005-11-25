/*
 * Copyright (c) 2005 by KoanLogic s.r.l. <http://www.koanlogic.com>
 * All rights reserved.
 *
 * This file is part of KLone, and as such it is subject to the license stated
 * in the LICENSE file which you have received as part of this distribution.
 *
 * $Id: version.c,v 1.5 2005/11/25 10:25:04 tho Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <klone/version.h>
#include <u/libu.h>

/**
 *  \addtogroup u_t 
 *  \{
 *      \par
 */

/**
 * \brief   Return KLone version string (x.y.z)
 * 
 * Return KLone version string in the format x.y.z.
 *
 * \return the version string
 */
const char *klone_version(void)
{
    return KLONE_VERSION;
}

/**
 *  \}
 */
