/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Handlers.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mabou-ha <mabou-ha>                        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/28 13:05:30 by mabou-ha          #+#    #+#             */
/*   Updated: 2026/01/07 02:36:49 by mabou-ha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Handlers.hpp"
#include <sstream>
#include <cstdlib>

static std::string prefixOf(Server &srv, int fd)
{
	Client *c = srv.getClient(fd);
	if (!c) return ":unknown";
	std::string n = c->hasNick ? c->nick : "*";
	std::string u = c->hasUser ? c->user : "user";
	return ":" + n + "!" + u + "@" + c->host;
}

static bool ensureRegistered(Server &srv, int fd)
{
	Client *c = srv.getClient(fd);
	if (!c) return false;
	if (!c->registered)
	{
		srv.sendNumeric(fd, 451, "*", "You have not registered");
		return false;
	}
	return true;
}

static void tryWelcome(Server &srv, int fd)
{
	Client *c = srv.getClient(fd);
	if (!c) return;

	if (!c->registered && c->passOk && c->hasNick && c->hasUser)
	{
		c->registered = true;
		srv.sendNumeric(fd, 1, c->nick, "Welcome to the IRC server " + c->nick);
		srv.sendNumeric(fd, 2, c->nick, "Your host is " + srv.getServerName());
		srv.sendNumeric(fd, 3, c->nick, "This server was created for the project");
	}
}

static void sendNames(Server &srv, int fd, const std::string &chan)
{
	Client *c = srv.getClient(fd);
	Channel *ch = srv.getChannel(chan);
	if (!c || !ch) return;

	std::string names;
	for (std::set<int>::iterator it = ch->members.begin(); it != ch->members.end(); ++it)
	{
		Client *m = srv.getClient(*it);
		if (!m || !m->hasNick) continue;

		std::string one = m->nick;
		if (ch->isOp(*it))
			one = "@" + one;

		if (!names.empty()) names += " ";
		names += one;
	}

	// 353 RPL_NAMREPLY, 366 RPL_ENDOFNAMES
	srv.sendNumeric(fd, 353, c->nick, "= " + chan + " :" + names);
	srv.sendNumeric(fd, 366, c->nick, chan + " :End of /NAMES list");
}

static void cmdPASS(Server &srv, int fd, const Message &msg)
{
	Client *c = srv.getClient(fd);
	if (!c) return;

	if (msg.params.size() < 1)
	{
		srv.sendNumeric(fd, 461, "*", "PASS :Not enough parameters");
		return;
	}
	if (c->registered)
	{
		srv.sendNumeric(fd, 462, c->nick, "You may not reregister");
		return;
	}

	if (msg.params[0] == srv.getPassword())
		c->passOk = true;
	else
	{
		srv.sendNumeric(fd, 464, "*", "Password incorrect");
		srv.disconnectClient(fd, "Bad password");
	}
}

static void cmdNICK(Server &srv, int fd, const Message &msg)
{
	Client *c = srv.getClient(fd);
	if (!c) return;

	if (msg.params.size() < 1)
	{
		srv.sendNumeric(fd, 431, "*", "No nickname given");
		return;
	}

	std::string newNick = msg.params[0];

	if (newNick.empty() || newNick.find(' ') != std::string::npos)
	{
		srv.sendNumeric(fd, 432, "*", "Erroneous nickname");
		return;
	}

	if (srv.nickExists(newNick) && (!c->hasNick || c->nick != newNick))
	{
		srv.sendNumeric(fd, 433, "*", "Nickname is already in use");
		return;
	}

	if (c->hasNick && c->nick != newNick)
	{
		std::string line = prefixOf(srv, fd) + " NICK :" + newNick + "\r\n";
		for (std::set<std::string>::iterator it = c->channels.begin(); it != c->channels.end(); ++it)
			srv.broadcastToChannel(*it, -1, line);
	}

	c->nick = newNick;
	c->hasNick = true;

	tryWelcome(srv, fd);
}

static void cmdUSER(Server &srv, int fd, const Message &msg)
{
	Client *c = srv.getClient(fd);
	if (!c) return;

	if (c->registered)
	{
		srv.sendNumeric(fd, 462, c->nick, "You may not reregister");
		return;
	}

	if (msg.params.size() < 1)
	{
		srv.sendNumeric(fd, 461, "*", "USER :Not enough parameters");
		return;
	}

	c->user = msg.params[0];
	c->hasUser = true;

	tryWelcome(srv, fd);
}

static void cmdPING(Server &srv, int fd, const Message &msg)
{
	std::string token;
	if (!msg.params.empty())
		token = msg.params[0];
	else if (msg.hasTrailing)
		token = msg.trailing;
	else
		token = "0";

	srv.sendToClient(fd, ":" + srv.getServerName() + " PONG " + srv.getServerName() + " :" + token + "\r\n");
}

static void cmdCAP(Server &srv, int fd, const Message &msg)
{
	(void)srv; (void)fd; (void)msg;
}

static void cmdJOIN(Server &srv, int fd, const Message &msg)
{
	if (!ensureRegistered(srv, fd)) return;

	Client *c = srv.getClient(fd);
	if (!c) return;

	if (msg.params.size() < 1)
	{
		srv.sendNumeric(fd, 461, c->nick, "JOIN :Not enough parameters");
		return;
	}

	std::string chname = msg.params[0];
	if (chname.empty() || chname[0] != '#')
	{
		srv.sendNumeric(fd, 479, c->nick, "Illegal channel name");
		return;
	}

	std::string providedKey;
	if (msg.params.size() >= 2)
		providedKey = msg.params[1];

	Channel *ch = srv.getOrCreateChannel(chname);

	if (ch->isMember(fd))
		return;

	if (ch->modeInviteOnly && ch->invited.find(fd) == ch->invited.end())
	{
		srv.sendNumeric(fd, 473, c->nick, chname + " :Cannot join channel (+i)");
		return;
	}
	if (ch->modeKey && ch->key != providedKey)
	{
		srv.sendNumeric(fd, 475, c->nick, chname + " :Cannot join channel (+k)");
		return;
	}
	if (ch->isFull())
	{
		srv.sendNumeric(fd, 471, c->nick, chname + " :Cannot join channel (+l)");
		return;
	}

	ch->addMember(fd);
	ch->invited.erase(fd);
	c->channels.insert(chname);

	if (ch->operators.empty())
		ch->operators.insert(fd);

	std::string joinLine = prefixOf(srv, fd) + " JOIN :" + chname + "\r\n";
	srv.broadcastToChannel(chname, -1, joinLine);

	if (ch->topic.empty())
		srv.sendNumeric(fd, 331, c->nick, chname + " :No topic is set");
	else
		srv.sendNumeric(fd, 332, c->nick, chname + " :" + ch->topic);

	sendNames(srv, fd, chname);
}

static void cmdPART(Server &srv, int fd, const Message &msg)
{
	if (!ensureRegistered(srv, fd)) return;

	Client *c = srv.getClient(fd);
	if (!c) return;

	if (msg.params.size() < 1)
	{
		srv.sendNumeric(fd, 461, c->nick, "PART :Not enough parameters");
		return;
	}

	std::string chname = msg.params[0];
	Channel *ch = srv.getChannel(chname);
	if (!ch)
	{
		srv.sendNumeric(fd, 403, c->nick, chname + " :No such channel");
		return;
	}
	if (!ch->isMember(fd))
	{
		srv.sendNumeric(fd, 442, c->nick, chname + " :You're not on that channel");
		return;
	}

	std::string reason = msg.hasTrailing ? msg.trailing : "";
	std::string line = prefixOf(srv, fd) + " PART " + chname;
	if (!reason.empty()) line += " :" + reason;
	line += "\r\n";

	srv.broadcastToChannel(chname, -1, line);

	ch->removeMember(fd);
	c->channels.erase(chname);

	if (ch->members.empty())
		srv.eraseChannel(chname);
}

static void cmdTOPIC(Server &srv, int fd, const Message &msg)
{
	if (!ensureRegistered(srv, fd)) return;

	Client *c = srv.getClient(fd);
	if (!c) return;

	if (msg.params.size() < 1)
	{
		srv.sendNumeric(fd, 461, c->nick, "TOPIC :Not enough parameters");
		return;
	}

	std::string chname = msg.params[0];
	Channel *ch = srv.getChannel(chname);
	if (!ch)
	{
		srv.sendNumeric(fd, 403, c->nick, chname + " :No such channel");
		return;
	}
	if (!ch->isMember(fd))
	{
		srv.sendNumeric(fd, 442, c->nick, chname + " :You're not on that channel");
		return;
	}
	if (!msg.hasTrailing)
	{
		if (ch->topic.empty())
			srv.sendNumeric(fd, 331, c->nick, chname + " :No topic is set");
		else
			srv.sendNumeric(fd, 332, c->nick, chname + " :" + ch->topic);
		return;
	}
	if (ch->modeTopicOpOnly && !ch->isOp(fd))
	{
		srv.sendNumeric(fd, 482, c->nick, chname + " :You're not channel operator");
		return;
	}

	ch->topic = msg.trailing;
	std::string line = prefixOf(srv, fd) + " TOPIC " + chname + " :" + ch->topic + "\r\n";
	srv.broadcastToChannel(chname, -1, line);
}

static void cmdINVITE(Server &srv, int fd, const Message &msg)
{
	if (!ensureRegistered(srv, fd)) return;

	Client *c = srv.getClient(fd);
	if (!c) return;

	if (msg.params.size() < 2)
	{
		srv.sendNumeric(fd, 461, c->nick, "INVITE :Not enough parameters");
		return;
	}

	std::string nick = msg.params[0];
	std::string chname = msg.params[1];

	int tfd = srv.fdByNick(nick);
	if (tfd < 0)
	{
		srv.sendNumeric(fd, 401, c->nick, nick + " :No such nick");
		return;
	}

	Channel *ch = srv.getChannel(chname);
	if (!ch)
	{
		srv.sendNumeric(fd, 403, c->nick, chname + " :No such channel");
		return;
	}
	if (!ch->isMember(fd))
	{
		srv.sendNumeric(fd, 442, c->nick, chname + " :You're not on that channel");
		return;
	}
	if (ch->modeInviteOnly && !ch->isOp(fd))
	{
		srv.sendNumeric(fd, 482, c->nick, chname + " :You're not channel operator");
		return;
	}

	if (ch->isMember(tfd))
	{
		srv.sendNumeric(fd, 443, c->nick, nick + " " + chname + " :is already on channel");
		return;
	}

	ch->invited.insert(tfd);

	srv.sendNumeric(fd, 341, c->nick, nick + " " + chname);

	std::string line = prefixOf(srv, fd) + " INVITE " + nick + " :" + chname + "\r\n";
	srv.sendToClient(tfd, line);
}

static void cmdKICK(Server &srv, int fd, const Message &msg)
{
	if (!ensureRegistered(srv, fd)) return;

	Client *c = srv.getClient(fd);
	if (!c) return;

	if (msg.params.size() < 2)
	{
		srv.sendNumeric(fd, 461, c->nick, "KICK :Not enough parameters");
		return;
	}

	std::string chname = msg.params[0];
	std::string nick = msg.params[1];
	std::string reason = msg.hasTrailing ? msg.trailing : "Kicked";

	Channel *ch = srv.getChannel(chname);
	if (!ch)
	{
		srv.sendNumeric(fd, 403, c->nick, chname + " :No such channel");
		return;
	}
	if (!ch->isMember(fd))
	{
		srv.sendNumeric(fd, 442, c->nick, chname + " :You're not on that channel");
		return;
	}
	if (!ch->isOp(fd))
	{
		srv.sendNumeric(fd, 482, c->nick, chname + " :You're not channel operator");
		return;
	}

	int tfd = srv.fdByNick(nick);
	if (tfd < 0)
	{
		srv.sendNumeric(fd, 401, c->nick, nick + " :No such nick");
		return;
	}
	if (!ch->isMember(tfd))
	{
		srv.sendNumeric(fd, 441, c->nick, nick + " " + chname + " :They aren't on that channel");
		return;
	}

	std::string line = prefixOf(srv, fd) + " KICK " + chname + " " + nick + " :" + reason + "\r\n";
	srv.broadcastToChannel(chname, -1, line);

	Client *tc = srv.getClient(tfd);
	if (tc) tc->channels.erase(chname);

	ch->removeMember(tfd);

	if (ch->members.empty())
		srv.eraseChannel(chname);
}

static void cmdMODE(Server &srv, int fd, const Message &msg)
{
	if (!ensureRegistered(srv, fd)) return;

	Client *c = srv.getClient(fd);
	if (!c) return;

	if (msg.params.size() < 1)
	{
		srv.sendNumeric(fd, 461, c->nick, "MODE :Not enough parameters");
		return;
	}

	std::string target = msg.params[0];
	if (target.empty() || target[0] != '#')
	{
		srv.sendNumeric(fd, 501, c->nick, "Unknown MODE flag");
		return;
	}

	Channel *ch = srv.getChannel(target);
	if (!ch)
	{
		srv.sendNumeric(fd, 403, c->nick, target + " :No such channel");
		return;
	}

	if (msg.params.size() == 1)
	{
		std::string modes = "+";
		if (ch->modeInviteOnly) modes += "i";
		if (ch->modeTopicOpOnly) modes += "t";
		if (ch->modeKey) modes += "k";
		if (ch->modeLimit) modes += "l";
		srv.sendNumeric(fd, 324, c->nick, target + " " + modes);
		return;
	}

	if (!ch->isMember(fd))
	{
		srv.sendNumeric(fd, 442, c->nick, target + " :You're not on that channel");
		return;
	}
	if (!ch->isOp(fd))
	{
		srv.sendNumeric(fd, 482, c->nick, target + " :You're not channel operator");
		return;
	}

	std::string modeStr = msg.params[1];
	bool adding = true;
	size_t argi = 2;

	std::string announceParams;

	for (size_t i = 0; i < modeStr.size(); ++i)
	{
		char chm = modeStr[i];
		if (chm == '+') { adding = true; continue; }
		if (chm == '-') { adding = false; continue; }

		if (chm == 'i') ch->modeInviteOnly = adding;
		else if (chm == 't') ch->modeTopicOpOnly = adding;
		else if (chm == 'k')
		{
			if (adding)
			{
				if (msg.params.size() <= argi) { srv.sendNumeric(fd, 461, c->nick, "MODE :Not enough parameters"); return; }
				ch->modeKey = true;
				ch->key = msg.params[argi++];
				announceParams += " " + ch->key;
			}
			else
			{
				ch->modeKey = false;
				ch->key = "";
			}
		}
		else if (chm == 'l')
		{
			if (adding)
			{
				if (msg.params.size() <= argi) { srv.sendNumeric(fd, 461, c->nick, "MODE :Not enough parameters"); return; }
				ch->modeLimit = true;
				ch->userLimit = std::atoi(msg.params[argi].c_str());
				announceParams += " " + msg.params[argi];
				argi++;
			}
			else
			{
				ch->modeLimit = false;
				ch->userLimit = 0;
			}
		}
		else if (chm == 'o')
		{
			if (msg.params.size() <= argi) { srv.sendNumeric(fd, 461, c->nick, "MODE :Not enough parameters"); return; }
			std::string nick = msg.params[argi++];
			int tfd = srv.fdByNick(nick);
			if (tfd < 0) { srv.sendNumeric(fd, 401, c->nick, nick + " :No such nick"); return; }
			if (!ch->isMember(tfd)) { srv.sendNumeric(fd, 441, c->nick, nick + " " + target + " :They aren't on that channel"); return; }

			if (adding) ch->operators.insert(tfd);
			else ch->operators.erase(tfd);

			announceParams += " " + nick;
		}
		else
		{
			srv.sendNumeric(fd, 501, c->nick, "Unknown MODE flag");
			return;
		}
	}

	std::string line = prefixOf(srv, fd) + " MODE " + target + " " + modeStr + announceParams + "\r\n";
	srv.broadcastToChannel(target, -1, line);
}

static void cmdPRIVMSG(Server &srv, int fd, const Message &msg)
{
	if (!ensureRegistered(srv, fd)) return;

	Client *c = srv.getClient(fd);
	if (!c) return;

	if (msg.params.size() < 1 || !msg.hasTrailing)
	{
		srv.sendNumeric(fd, 461, c->nick, "PRIVMSG :Not enough parameters");
		return;
	}

	std::string target = msg.params[0];
	std::string text = msg.trailing;

	std::string line = prefixOf(srv, fd) + " PRIVMSG " + target + " :" + text + "\r\n";

	if (!target.empty() && target[0] == '#')
	{
		Channel *ch = srv.getChannel(target);
		if (!ch)
		{
			srv.sendNumeric(fd, 403, c->nick, target + " :No such channel");
			return;
		}
		if (!ch->isMember(fd))
		{
			srv.sendNumeric(fd, 404, c->nick, target + " :Cannot send to channel");
			return;
		}
		srv.broadcastToChannel(target, fd, line);
	}
	else
	{
		int tfd = srv.fdByNick(target);
		if (tfd < 0)
		{
			srv.sendNumeric(fd, 401, c->nick, target + " :No such nick");
			return;
		}
		srv.sendToClient(tfd, line);
	}
}

static void cmdQUIT(Server &srv, int fd, const Message &msg)
{
	(void)msg;
	srv.disconnectClient(fd, "Client quit");
}

void handleCommand(Server &srv, int fd, const Message &msg)
{
	if (msg.command == "CAP")
		cmdCAP(srv, fd, msg);
	else if (msg.command == "PASS")
		cmdPASS(srv, fd, msg);
	else if (msg.command == "NICK")
		cmdNICK(srv, fd, msg);
	else if (msg.command == "USER")
		cmdUSER(srv, fd, msg);
	else if (msg.command == "PING")
		cmdPING(srv, fd, msg);
	else if (msg.command == "JOIN")
		cmdJOIN(srv, fd, msg);
	else if (msg.command == "PART")
		cmdPART(srv, fd, msg);
	else if (msg.command == "TOPIC")
		cmdTOPIC(srv, fd, msg);
	else if (msg.command == "INVITE")
		cmdINVITE(srv, fd, msg);
	else if (msg.command == "KICK")
		cmdKICK(srv, fd, msg);
	else if (msg.command == "MODE")
		cmdMODE(srv, fd, msg);
	else if (msg.command == "PRIVMSG")
		cmdPRIVMSG(srv, fd, msg);
	else if (msg.command == "QUIT")
		cmdQUIT(srv, fd, msg);
	else
	{
		Client *c = srv.getClient(fd);
		std::string target = (c && c->hasNick) ? c->nick : "*";
		srv.sendNumeric(fd, 421, target, msg.command + " :Unknown command");
	}
}
