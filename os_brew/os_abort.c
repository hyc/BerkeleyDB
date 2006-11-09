/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005,2006 Oracle.  All rights reserved.
 *
 * $Id: os_abort.c,v 1.5 2006/11/01 00:53:40 bostic Exp $
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __os_abort --
 */
void
__os_abort()
{
	AEEApplet *app;

	app = (AEEApplet *)GETAPPINSTANCE();
	ISHELL_CloseApplet(app->m_pIShell, FALSE);

	/* NOTREACHED */
}
