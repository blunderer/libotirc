########################
# SBOT: simple IRC bot #
######################## 

# bot
To launch a bot : rename the binary as the choosen name of the bot.
launch ./<botname> <server> <port> <chan1> <chan2> <chan3> ...

# builtin command
'quit' in a privmsg will tell the bot to quit

# plugins:
when talking to the bot, it will uses the first word as a command followed by arguments
each command is processed by calling the executable <command name>.sbot <username> <argument1> <argument2> ...

the result of the command will depend on the return value:
- 0: write to user on the chan or using private message depending on source message
- 1: write to user using private message
- 2: write a command JOIN followed by chan name 
- 3: write a command PART followed by chan name

Upon each new message on a chan, the sbot.msg executable will be called:
<botname>.msg <channame> <message>

this can be usefull for channel log, or channel supervision (react to some specific words)...

plugin can use any language (shell / python / C / ...).

the stdout string (maximum 1024 char) will be used as argument for command or message to user.

