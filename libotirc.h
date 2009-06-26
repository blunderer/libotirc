/** \file libotirc.h
 *
 * \author blunderer <blunderer@blunderer.org>
 * \date 28 Jun 2009
 *
 * Copyright (C) 2009 blunderer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef LIBOTIRC_H
#define LIBOTIRC_H

#include <time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <pthread.h>

#ifndef max
#define max(a,b)	((a > b)?a:b)
#endif

#define PRIVATE_API	static
#define PUBLIC_API
#define TODO

#define IRC_MAX_BOT	16
#define IRC_FROM_SIZE	256
#define IRC_DEST_SIZE	256
#define IRC_CHAN_SIZE	256
#define IRC_MESG_SIZE	2048
#define IRC_DATA_SIZE	(IRC_MESG_SIZE+IRC_FROM_SIZE+IRC_DEST_SIZE+IRC_CHAN_SIZE)

typedef struct _irc_chan_t irc_chan_t;
typedef struct _irc_bot_t irc_bot_t;
typedef struct _irc_msg_t irc_msg_t;

typedef int (*irc_bot_callback)(irc_bot_t* bot, irc_chan_t *chan, irc_msg_t *msg, void *data);

struct _irc_msg_t {
	char dest[IRC_DEST_SIZE];
	char from[IRC_FROM_SIZE];
	char mesg[IRC_MESG_SIZE];
};

struct _irc_chan_t {
	char name[IRC_CHAN_SIZE];
	struct _irc_chan_t* next;
};

struct _irc_bot_t {
	int soc;
	int port;
	char name[IRC_FROM_SIZE];
	char server[512];
	irc_chan_t *chans;
	irc_bot_callback on_msg;
	irc_bot_callback on_cmd;
	irc_bot_callback on_prv;
	irc_bot_callback on_tom;
	void* on_msg_data;
	void* on_cmd_data;
	void* on_prv_data;
	void* on_tom_data;
};


typedef enum {
	ping_cmd = 0,
	join_cmd,
	part_cmd,
	quit_cmd,
	privmsg_cmd,
	nick_cmd,
	user_cmd,
	pong_cmd
} irc_commande_t;

PRIVATE_API char* irc_commands[10] = {
	"PING",
	"JOIN",
	"PART",
	"QUIT",
	"PRIVMSG",
	"NICK",
	"USER",
	"PONG"
};

PRIVATE_API int _irc_bot_count = 0;
PRIVATE_API irc_bot_t* _g_bot[IRC_MAX_BOT];
PRIVATE_API pthread_t _irc_bot_service;

PRIVATE_API char * get_target(char * data, char * user_to, char * chan);
PRIVATE_API char * get_user(char * data, char * user_from);
PRIVATE_API int parse_message(char * data, char * to, char * from, char * chan, char * msg);
PRIVATE_API void* irc_service(void * t);

PUBLIC_API irc_bot_t* irc_create_bot(char *nick);
PUBLIC_API void irc_destroy_bot(irc_bot_t* bot);

PUBLIC_API int irc_connect_bot(irc_bot_t* bot, char *addr, int port);
PUBLIC_API void irc_disconnect_bot(irc_bot_t* bot);

PUBLIC_API irc_chan_t* irc_join(irc_bot_t* bot, char * name);
PUBLIC_API void irc_part(irc_bot_t* bot, irc_chan_t* chan);

PUBLIC_API int irc_send_command(irc_bot_t* bot, irc_commande_t cmd, char * arg);
PUBLIC_API int irc_send_message(irc_bot_t* bot, irc_chan_t *chan, char *msg);
PUBLIC_API int irc_send_message_to(irc_bot_t* bot, irc_chan_t *chan, char * user, char *msg);
PUBLIC_API int irc_send_private_message(irc_bot_t* bot, char * user, char *msg);

PUBLIC_API void irc_call_on_command(irc_bot_t* bot, irc_bot_callback cb, void *data);
PUBLIC_API void irc_call_on_chan_message(irc_bot_t* bot, irc_bot_callback cb, void *data);
PUBLIC_API void irc_call_on_chan_message_to_me(irc_bot_t* bot, irc_bot_callback cb, void *data);
PUBLIC_API void irc_call_on_chan_message_private(irc_bot_t* bot, irc_bot_callback cb, void *data);




#endif /* LIBOTIRC_H */

