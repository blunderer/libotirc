/** \file libotirc.c
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

#ifndef LIBOTIRC_C
#define LIBOTIRC_C

#include "libotirc.h"

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

int g_debug = 1;

irc_bot_t *irc_create_bot(char *nick)
{
	irc_bot_t *bot = (irc_bot_t*)malloc(sizeof(irc_bot_t));
	memset(bot, 0, sizeof(irc_bot_t));
	if(nick)	{
		strncpy(bot->name, nick, IRC_FROM_SIZE);
		return bot;
	} 
	free(bot);
	return NULL;
}

void irc_destroy_bot(irc_bot_t *bot)
{
	irc_disconnect_bot(bot);
	while(bot->chans)	{
		irc_chan_t* chan_to_quit = bot->chans;
		bot->chans = bot->chans->next;
		free(chan_to_quit);
	}
	close(bot->soc);
	free(bot);
}

void irc_disconnect_bot(irc_bot_t* bot)
{
	int i;
	for(i = 0;(i < IRC_MAX_BOT) && (_g_bot[i] != bot); i++);

	if(_g_bot[i] == bot)	{
		_irc_bot_count--;
		_g_bot[i] = NULL;
	}
	
	if(_irc_bot_count <= 0)	{
		pthread_join(_irc_bot_service, NULL);
	}
}

int irc_connect_bot(irc_bot_t* bot, char *addr, int port)
{
	int i; 
	char user[540];
        struct sockaddr_in saServer;
        struct sockaddr_in saClient;
        struct hostent* localHost;
        char * localIP;

	bot->port = port;
	strncpy(bot->server, addr, 512);
	
	sprintf(user,"%s 8 * :%s",bot->name,bot->name);

        localHost = gethostbyname(addr);
        localIP = inet_ntoa(*(struct in_addr *)*localHost->h_addr_list);

        saClient.sin_family = AF_INET;
        saClient.sin_addr.s_addr = inet_addr(localIP);
        saClient.sin_port = htons(port);
        saServer.sin_family = AF_INET;
        saServer.sin_addr.s_addr = inet_addr(localIP);
        saServer.sin_port = htons(port);

        bot->soc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(bot->soc < 0)	{
		return -EBUSY;
	}
	
	if(_irc_bot_count <= 0)	{
		memset(_g_bot, 0, sizeof(irc_bot_t*)*IRC_MAX_BOT);
		pthread_create(&_irc_bot_service, NULL, irc_service, NULL);
	}
	for(i = 0;(i < IRC_MAX_BOT) && (_g_bot[i] != NULL); i++);
	if(i < IRC_MAX_BOT)	{
		_g_bot[i] = bot;
		_irc_bot_count++;
	} else	{
		return -ENOMEM;
	}
	
	
	connect(bot->soc, (struct sockaddr*)&saServer, sizeof(struct sockaddr));

	irc_send_command(bot, nick_cmd, bot->name);
	irc_send_command(bot, user_cmd, user);

	return 0;
}

int irc_send_command(irc_bot_t* bot, irc_commande_t cmd, char * arg)
{
	char data[IRC_DATA_SIZE];
	sprintf(data,"%s %s\n", irc_commands[cmd], arg);

	switch(cmd)	{
		case join_cmd:
			break;
		case part_cmd:
			break;
		case quit_cmd:
			break;
		default:
			break;
	}
	if(g_debug)	{
		fprintf(stdout,"==== %s ====\n",data);
	}
	
	write(bot->soc, data, strlen(data));
	return 0;
}

int irc_send_message(irc_bot_t* bot, irc_chan_t *chan, char *msg)
{
	char data[IRC_DATA_SIZE];
	sprintf(data,"PRIVMSG %s : %s\n", chan->name, msg);
	if(g_debug)	{
		fprintf(stdout,"==== %s ====\n",data);
	}
	write(bot->soc, data, strlen(data));
	return 0;
}

int irc_send_message_to(irc_bot_t* bot, irc_chan_t *chan, char * user, char *msg)
{
	char data[IRC_DATA_SIZE];
	sprintf(data,"PRIVMSG %s : %s: %s\n", chan->name, user, msg);
	if(g_debug)	{
		fprintf(stdout,"==== %s ====\n",data);
	}
	write(bot->soc, data, strlen(data));
	return 0;
}

int irc_send_private_message(irc_bot_t* bot, char * user, char *msg)
{
	char data[IRC_DATA_SIZE];
	sprintf(data,"PRIVMSG %s : %s\n", user, msg);
	if(g_debug)	{
		fprintf(stdout,"==== %s ====\n",data);
	}
	write(bot->soc, data, strlen(data));
	return 0;
}

void* irc_service(void * t)
{
	while(_irc_bot_count > 0)	{
		int i;
		fd_set rfds;
		struct timeval tv;
		int max_count = 0;
		int status = 0;
		char data[IRC_DATA_SIZE];
		char chan[IRC_CHAN_SIZE];
		char dest[IRC_DATA_SIZE];
		char from[IRC_FROM_SIZE];
		char mesg[IRC_MESG_SIZE];

		FD_ZERO(&rfds);
		for(i = 0; i < _irc_bot_count; i++)	{
			max_count = max(max_count, _g_bot[i]->soc);
			FD_SET(_g_bot[i]->soc, &rfds);
		}

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		status = select(max_count+1, &rfds, NULL, NULL, &tv);

		if(status > 0)	{
			for(i = 0; i < _irc_bot_count; i++)     {
				if(FD_ISSET(_g_bot[i]->soc, &rfds))	{
					char cgot = 0;
					int offset = 0;
					memset(data, 0, IRC_DATA_SIZE);
					
					while((offset < IRC_DATA_SIZE)&&(cgot != '\n'))	{
						recv(_g_bot[i]->soc, &cgot, 1, 0);
						data[offset++] = cgot;
					}
					
					if(g_debug)	{
						fprintf(stdout,">>>> %s <<<<\n",data);
					}

					if(strlen(data) == 0) {
						return NULL;
					}

					parse_message(data, dest, from, chan, mesg);

					if(strlen(from) == 0)	{
						if(strncasecmp(mesg, "PING", strlen("PING")) == 0)       {
							/* handle PING answer */
							irc_send_command(_g_bot[i], pong_cmd, _g_bot[i]->server);
						}
					} else	{
						irc_chan_t *this_chan = _g_bot[i]->chans;
						irc_msg_t msg;

						while((this_chan != NULL)&&(strcmp(chan, this_chan->name) != 0)) { 
							this_chan = this_chan->next; 
						};
						
						memset(&msg, 0, sizeof(irc_msg_t));
						strcpy(msg.dest, dest);
						strcpy(msg.from, from);
						strcpy(msg.mesg, mesg);
						
						if((strlen(chan) == 0)&&(strlen(dest) == 0))	{
							if(_g_bot[i]->on_cmd)	{
								_g_bot[i]->on_cmd(_g_bot[i], this_chan, &msg, _g_bot[i]->on_cmd_data);
							}
						} else if(strlen(chan) == 0)	{
							if(_g_bot[i]->on_prv)	{
								_g_bot[i]->on_prv(_g_bot[i], this_chan, &msg, _g_bot[i]->on_prv_data);
							}
						} else if(strlen(dest) == 0)	{
							if(strstr(mesg, _g_bot[i]->name))	{
								if(_g_bot[i]->on_tom)	{
									_g_bot[i]->on_tom(_g_bot[i], this_chan, &msg, _g_bot[i]->on_tom_data);
								}
							} else	{
								if(_g_bot[i]->on_msg)	{
									_g_bot[i]->on_msg(_g_bot[i], this_chan, &msg, _g_bot[i]->on_msg_data);
								}
							}
						} else { ; }
					}
				}
			}
		}
	}
	return t;
}


PRIVATE_API char * get_user(char * data, char * user_from)
{
	char *enduser;
	strncpy(user_from, data + 1, 127);
	enduser = strstr(user_from,"!");
	if(enduser == NULL)	{
		strcpy(user_from, "");
		return data;
	}
	enduser[0] = '\0';
	return enduser+1;
}

PRIVATE_API char * get_target(char * data, char * user_to, char * chan)
{
	char *target;
	char *endtarget;
	
	target = strstr(data, irc_commands[privmsg_cmd]);
	
	if(target == NULL)	{
		strcpy(chan, "");
		strcpy(user_to, "");
		return data;
	}
	
	target += strlen(irc_commands[privmsg_cmd])+1;
	if(target[0] == '#')	{
		/* chan */
		strcpy(user_to, "");
		strncpy(chan, target, 127);
		endtarget = strstr(chan," :");
		endtarget++;
		endtarget[0] = '\0';
	} else {
		strcpy(chan, "");
		strncpy(user_to, target, 127);
		endtarget = strstr(user_to," :");
		endtarget++;
		endtarget[0] = '\0';
	}
	return endtarget+1;
}

PRIVATE_API int parse_message(char * data, char * to, char * from, char * chan, char * msg)	{
	int i;

	/* get sender */
	data = get_user(data, from);
	i = strlen(from)-1;
	while((from[i] == ' ')||(from[i] == '\t')) { from[i--] = '\0'; }

	/* get dest */
	data = get_target(data, to, chan);
	i = strlen(chan)-1;
	while((chan[i] == ' ')||(chan[i] == '\t')) { chan[i--] = '\0'; }
	i = strlen(to)-1;
	while((to[i] == ' ')||(to[i] == '\t')) { to[i--] = '\0'; }
	
	/* get message */
	strcpy(msg, data);
	i = strlen(msg)-1;
	while((msg[i] == ' ')||(msg[i] == '\t')) { msg[i--] = '\0'; }

	return 0;
}

PUBLIC_API void irc_call_on_command(irc_bot_t* bot, irc_bot_callback cb, void *data)
{
	bot->on_cmd = cb;
	bot->on_cmd_data = data;
}

PUBLIC_API void irc_call_on_chan_message(irc_bot_t* bot, irc_bot_callback cb, void *data)
{
	bot->on_msg = cb;
	bot->on_msg_data = data;
}

PUBLIC_API void irc_call_on_chan_message_to_me(irc_bot_t* bot, irc_bot_callback cb, void *data)
{
	bot->on_tom = cb;
	bot->on_tom_data = data;
}

PUBLIC_API void irc_call_on_chan_message_private(irc_bot_t* bot, irc_bot_callback cb, void *data)
{
	bot->on_prv = cb;
	bot->on_prv_data = data;
}

PUBLIC_API irc_chan_t* irc_join(irc_bot_t* bot, char * name)
{
	irc_chan_t *ptr = bot->chans;

	if(bot->chans == NULL)	{
		bot->chans = (irc_chan_t*)malloc(sizeof(irc_chan_t));
		ptr = bot->chans;
	} else	{
		while(ptr && ptr->next) 	{
			ptr = ptr->next;
		}
		ptr->next = (irc_chan_t*)malloc(sizeof(irc_chan_t));
		ptr = ptr->next;
	}
	memset(ptr, 0, sizeof(irc_chan_t));
	strcpy(ptr->name, name);
	irc_send_command(bot, join_cmd, name);
	return ptr;
}

PUBLIC_API void irc_part(irc_bot_t* bot, irc_chan_t* chan)
{
	irc_chan_t *ptr = bot->chans;

	if(bot->chans == chan)	{
		bot->chans = bot->chans->next;
	} else	{
		while(ptr && ptr->next)         {
			if(ptr->next == chan)	{
				ptr->next = chan->next;	
				break;
			}
			ptr = ptr->next;
		}
	}

	irc_send_command(bot, part_cmd, chan->name);
	free(chan);
}

#endif /* LIBOTIRC_C */

