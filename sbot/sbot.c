/*
 * sbot
 *
 * sbot is a simple bot made using libotirc.
 * This bot is fully extensible since it match function in plugins to 
 * IRC commands following. Each command will trigger a call to 
 * plugins-enabled/<command>.sbot
 * All the command plugin must source "sbot_common.sh" and be able to:
 * read arguments as 
 *  - $CHAN_NAME:	channel name
 *  - $MESSAGE:		message
 *  - $USER:		the message autor
 * write to stdout the answer to send (max 1024 char)
 * return the exit code following those rules depending on required action
 *  - $ANSWER_AUTO: 	answer to the user (private or in channel depending on the input message
 *  - $ANSWER_PRIVATE: 	answer in a private message (whatever the input message is)
 *  - $ANSWER_JOIN: 	join the chan name given in stdout
 *  - $ANSWER_PART: 	part the chan name given in stdout
 *
 * It will also call the special command "msg.sbot" for each received message
 * and output stdout as answer on the same chan
 *
 * It will also call the special command "cron.sbot" each CRON_PERIOD milliseconds
 * 
 */

#include "libotirc.h"
#include "sbot_common.sh"

int running = 1;
char basepath[512];

#define CRON_PERIOD 10000

/* 
 * exec an action and process output
 */
static void exec_action(irc_bot_t *bot, char * command, irc_chan_t * chan, char * from, void * data)
{
	int ret = 0;
	char str[1024];

	FILE * cmd = popen(command, "r");
	memset(str, 0, 1024);
	fread(str, 1, 1024, cmd);
	ret = (pclose(cmd) >> 8) & 0xff;

	/* data = 0 if this is a round exec command: ie exec command on something sent by the bot himself */
	if(data != (void*)0) {
		/* process command output: do we have to answer something or take any action on the irc server */
		if(ret == ANSWER_AUTO) {
			if(strlen(str)) {
				if(data == (void*)1) {
					char libcommand[1024];
					irc_send_message(bot, chan, str);
					sprintf(libcommand, "%s/plugins-enabled/msg.sbot %s %s \"%s\"", basepath, chan->name+1, bot->name, str);
					exec_action(bot, libcommand, chan, str, (void*)0);
				} else {
					if(from) {
						irc_send_private_message(bot, from, str);
					}
				}
			}
		} else if(ret == ANSWER_PRIVATE) {
			if(strlen(str) && from) {
				irc_send_private_message(bot, from, str);
			}
		} else if(ret == ANSWER_JOIN) {
			irc_send_command(bot, join_cmd, str);
		} else if(ret == ANSWER_PART) {
			irc_send_command(bot, part_cmd, str);
		} 
	}
	return;
}

/* 
 * on message callback
 * function called on message received
 * will check for something to answer on the chan
 */
int sbot_exec_message(irc_bot_t* bot, irc_chan_t *chan, irc_msg_t *msg, void * data)
{
	int i;
	char libcommand[1024];
	
	/* remove trailing white char */
	for(i = strlen(msg->mesg); (msg->mesg[i] == '\0') || (msg->mesg[i] == ' ') || (msg->mesg[i] == '\n' || (msg->mesg[i] == '\r')) || (msg->mesg[i] == '\t'); i--) msg->mesg[i] = '\0';

	/* generate command to execute for this msg */
	sprintf(libcommand, "%s/plugins-enabled/msg.sbot %s %s \"%s\"", basepath, chan->name+1, msg->from, msg->mesg);

	/* exec the command */
	exec_action(bot, libcommand, chan, msg->from, data);

	return 0;
}

/* 
 * on command callback
 * function called on command received
 * will check for action to execute
 */
int sbot_exec_command(irc_bot_t* bot, irc_chan_t *chan, irc_msg_t *msg, void *data)
{
	int i = 0;
	char * arguments;
	char str[1024];
	char command[128];
	char libcommand[1024];

	/* export the message to a command */
	if(strncmp(msg->mesg, bot->name, strlen(bot->name)) == 0) {
		strcpy(command, msg->mesg+strlen(bot->name)+2);
	} else {
		strcpy(command, msg->mesg);
	}

	/* transform command into one real command + arguments */
	for(i = 0; (command[i] != '\0') && (command[i] != ' ') && (command[i] != '\n' && (command[i] != '\r')) && (command[i] != '\t'); i++);
	command[i] = '\0';
	arguments = command+i+1;

	/* if data == 2 means this is a private message, else this is a message to in a chan */
	if(data == (void*)2) {
		/* handle command quit as a built in command */
		if(strcmp(command, "quit") == 0) {
			irc_send_command(bot, quit_cmd, arguments);
			running = 0;
			return 0;
		}
	}

	/* generate command to execute */
	if(chan) {
		sprintf(libcommand, "%s/plugins-enabled/%s.sbot %s %s %s", basepath, command, chan->name+1, msg->from, arguments);
	} else {
		sprintf(libcommand, "%s/plugins-enabled/%s.sbot PRIVATE %s %s", basepath, command, msg->from, arguments);
	}

	/* execute command */
	exec_action(bot, libcommand, chan, msg->from, data);

	return 0;
}

/* 
 * wrapper: a message to me can be a command and/or a simple message
 */
int sbot_command(irc_bot_t* bot, irc_chan_t *chan, irc_msg_t *msg, void *data)
{
	sbot_exec_command(bot, chan, msg, data);
	sbot_exec_message(bot, chan, msg, data);
	return 0;
}

/* 
 * sbot main
 * Main function that init the bot
 */
int main(int argc, const char ** argv)
{
	int i;
	int cnt = 0;
	char libcommand[1024];

	char * name = (char *)argv[0];
	for(i = 0; argv[0][i] != '\0'; i++) {
		if(argv[0][i] == '/') {
			name = (char*)argv[0] + i + 1;
		}
	}
	
	memset(basepath, 0, 512);

	if(argv[0][0] == '/') {
		strncat(basepath, argv[0], strlen(argv[0])-strlen(name));
	} else {
		getcwd(basepath, 512);
		strcat(basepath, "/");
		strncat(basepath, argv[0], strlen(argv[0])-strlen(name));
	}
	sprintf(libcommand, "%s/plugins-enabled/cron.sbot", basepath);

	irc_bot_t* sbot = irc_create_bot(name);

	if(argc > 2) {
		if(irc_connect_bot(sbot, (char*)argv[1], atoi((char*)argv[2])) < 0) {
			printf("cannot connect bot to server\n");
			exit(1);
		}
	} else {
		printf("syntax: ./[bot name] [server] [port] [chan1] [chan2] ...\n");
		exit(0);
	}
	
	/* wait for connection ok */
	usleep(1000000);

	/* join every chan given from command line */
	for(i = 3; i < argc; i++) {
		char channame[128];
		sprintf(channame, "#%s", argv[i]);
		irc_join(sbot, channame);
	}

	/* configure the bot callbacks. arg = 1 or 2 to change behaviour when command is sent from private message */
	irc_call_on_chan_message(sbot, sbot_exec_message, (void*)1);
	irc_call_on_chan_message_to_me(sbot, sbot_command, (void*)1);
	irc_call_on_chan_message_private(sbot, sbot_exec_command, (void*)2);

	/* main loop: do nothing */
	while(running) {
		cnt++;
		if((cnt % CRON_PERIOD) == 1) {
			/* exec the command */
			irc_chan_t * chan = sbot->chans;
			while(chan) {
				exec_action(sbot, libcommand, chan, NULL, (void*)1);
				chan = chan->next;
			}
		}
		usleep(1000);
	}

	/* disconnect the bot */
	irc_disconnect_bot(sbot);
	irc_destroy_bot(sbot);

	return 0;
}

