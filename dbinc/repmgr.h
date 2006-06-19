/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2006
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: repmgr.h,v 12.1 2006/06/19 06:41:25 mjc Exp $
 */

#ifndef _DB_REPMGR_H
#define	_DB_REPMGR_H

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef DB_WIN32
typedef SOCKET socket_t;
typedef HANDLE thread_id_t;
typedef HANDLE mgr_mutex_t;
typedef HANDLE cond_var_t;
typedef WSABUF db_iovec_t;
#else
typedef int socket_t;
typedef pthread_t thread_id_t;
typedef pthread_mutex_t mgr_mutex_t;
typedef pthread_cond_t cond_var_t;
typedef struct iovec db_iovec_t;
#endif

struct __repmgr_connection;
    typedef struct __repmgr_connection REPMGR_CONNECTION;
struct __repmgr_queue; typedef struct __repmgr_queue REPMGR_QUEUE;
struct __queued_output; typedef struct __queued_output QUEUED_OUTPUT;
struct __repmgr_retry; typedef struct __repmgr_retry REPMGR_RETRY;
struct __repmgr_runnable; typedef struct __repmgr_runnable REPMGR_RUNNABLE;
struct __repmgr_site; typedef struct __repmgr_site REPMGR_SITE;
struct __ack_waiters_table;
    typedef struct __ack_waiters_table ACK_WAITERS_TABLE;

typedef TAILQ_HEAD(__repmgr_conn_list, __repmgr_connection) CONNECTION_LIST;
typedef STAILQ_HEAD(__repmgr_out_q_head, __queued_output) OUT_Q_HEADER;
typedef TAILQ_HEAD(__repmgr_retry_q, __repmgr_retry) RETRY_Q_HEADER;

#ifdef HAVE_GETADDRINFO
typedef struct addrinfo	ADDRINFO;
#else
/*
 * Some windows platforms have getaddrinfo (Windows XP), some don't.  We don't
 * support conditional compilation in our Windows build, so we always use our
 * own getaddinfo implementation.  Rename everything so that we don't collide
 * with the system libraries.
 */
#undef	AI_PASSIVE
#define	AI_PASSIVE	0x01
#undef	AI_CANONNAME
#define	AI_CANONNAME	0x02
#undef	AI_NUMERICHOST
#define	AI_NUMERICHOST	0x04

typedef struct __addrinfo {
	int ai_flags;		/* AI_PASSIVE, AI_CANONNAME, AI_NUMERICHOST */
	int ai_family;		/* PF_xxx */
	int ai_socktype;	/* SOCK_xxx */
	int ai_protocol;	/* 0 or IPPROTO_xxx for IPv4 and IPv6 */
	size_t ai_addrlen;	/* length of ai_addr */
	char *ai_canonname;	/* canonical name for nodename */
	struct sockaddr *ai_addr;	/* binary address */
	struct __addrinfo *ai_next;	/* next structure in linked list */
} ADDRINFO;
#endif /* HAVE_GETADDRINFO */

typedef struct {
	char *host;		/* Separately allocated copy of string. */
	u_int16_t port;		/* Stored in plain old host-byte-order. */
	ADDRINFO *address_list;
	ADDRINFO *current;
} repmgr_netaddr_t;

/* Macros to proceed, as with a cursor, through the address_list: */
#define	ADDR_LIST_CURRENT(na)	((na)->current)
#define	ADDR_LIST_FIRST(na)	((na)->current = (na)->address_list)
#define	ADDR_LIST_NEXT(na)	((na)->current = (na)->current->ai_next)

#if defined(__cplusplus)
}
#endif
#endif /* !_DB_REPMGR_H_ */
