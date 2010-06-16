
#include "libotirc.h"

int running = 1;
char basepath[512];

int sbot_exec_message(irc_bot_t* bot, irc_chan_t *chan, irc_msg_t *msg, void * data)
{
	int i;
	char libcommand[1024];
	char str[1024];
	for(i = strlen(msg->mesg); (msg->mesg[i] == '\0') || (msg->mesg[i] == ' ') || (msg->mesg[i] == '\n' || (msg->mesg[i] == '\r')) || (msg->mesg[i] == '\t'); i--) msg->mesg[i] = '\0';
	sprintf(libcommand, "%s/plugins-enabled/msg.sbot %s \"%s\"", basepath, chan->name+1, msg->mesg);
	printf("%s/plugins-enabled/msg.sbot %s \"%s\"", basepath, chan->name+1, msg->mesg);
	FILE * cmd = popen(libcommand, "r");
	memset(str, 0, 1024);
	fread(str, 1, 1024, cmd);
	pclose(cmd);
	if(strlen(str)) {
		irc_send_message(bot, chan, str);
	}
	return 0;
}

int sbot_exec_command(irc_bot_t* bot, irc_chan_t *chan, irc_msg_t *msg, void *data)
{
	int i = 0;
	int ret;
	char libcommand[1024];
	char command[128];
	char str[1024];
	char * arguments;

	if(strncmp(msg->mesg, bot->name, strlen(bot->name)) == 0) {
		strcpy(command, msg->mesg+strlen(bot->name)+2);
	} else {
		strcpy(command, msg->mesg);
	}
	for(i = 0; (command[i] != '\0') && (command[i] != ' ') && (command[i] != '\n' && (command[i] != '\r')) && (command[i] != '\t'); i++);
	command[i] = '\0';
	arguments = command+i+1;

	if(data == (void*)2) {
		if(strcmp(command, "quit") == 0) {
			irc_send_command(bot, quit_cmd, arguments);
			running = 0;
			return 0;
		}
	}

	sprintf(libcommand, "%s/plugins-enabled/%s.sbot %s %s", basepath, command, msg->from, arguments);

	FILE * cmd = popen(libcommand, "r");
	memset(str, 0, 1024);
	fread(str, 1, 1024, cmd);
	ret = (pclose(cmd) >> 8) & 0xff;

	if(ret == 0) {
		if(strlen(str)) {
			if(data == (void*)1) {
				irc_send_message(bot, chan, str);
			} else {
				irc_send_private_message(bot, msg->from, str);
			}
		}
	} else if(ret == 1) {
		if(strlen(str)) {
			irc_send_private_message(bot, msg->from, str);
		}
	} else if(ret == 2) {
		irc_send_command(bot, join_cmd, str);
	} else if(ret == 3) {
		irc_send_command(bot, part_cmd, str);
	} 
	return 0;
}

int sbot_command(irc_bot_t* bot, irc_chan_t *chan, irc_msg_t *msg, void *data)
{
	sbot_exec_command(bot, chan, msg, data);
	sbot_exec_message(bot, chan, msg, data);
	return 0;
}

int main(int argc, const char ** argv)
{
	int i;

	char * name = (char *)argv[0];
	for(i = 0; argv[0][i] != '\0'; i++) {
		if(argv[0][i] == '/') {
			name = (char*)argv[0] + i + 1;
		}
	}
	
	memset(basepath, 0, 512);
	getcwd(basepath, 512);
	strcat(basepath, "/");
	strncat(basepath, argv[0], strlen(argv[0])-strlen(name));
	
	irc_bot_t* sbot = irc_create_bot(name);

	if(argc > 2) {
		irc_connect_bot(sbot, (char*)argv[1], atoi((char*)argv[2]));
	} else {
		printf("syntax: ./[bot name] [server] [port] [chan1] [chan2] ...\n");
		exit(0);
	}
	
	usleep(1000000);

	for(i = 3; i < argc; i++) {
		char channame[128];
		sprintf(channame, "#%s", argv[i]);
		irc_join(sbot, channame);
	}

	irc_call_on_chan_message(sbot, sbot_exec_message, (void*)1);
	irc_call_on_chan_message_to_me(sbot, sbot_command, (void*)1);
	irc_call_on_chan_message_private(sbot, sbot_exec_command, (void*)2);

	while(running) {
		usleep(100000);
	}

	irc_disconnect_bot(sbot);
	irc_destroy_bot(sbot);
	return 0;
}

