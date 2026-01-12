/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Handlers.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mabou-ha <mabou-ha@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/28 13:05:30 by mabou-ha          #+#    #+#             */
/*   Updated: 2026/01/12 01:59:29 by mabou-ha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Handlers.hpp"
#include <sstream>
#include <cstdlib>

std::string itos(int x)
{
	std::ostringstream oss;
	oss << x;
	return oss.str();
}

static std::string prefixOf(Server &srv, int fd)
{
	Client *c = srv.getClient(fd);
	if (!c) 
		return ":unknown";
	std::string n = c->hasNick ? c->nick : "*";
	std::string u = c->hasUser ? c->user : "user";
	return ":" + n + "!" + u + "@" + c->host;
}

static bool ensureRegistered(Server &srv, int fd)
{
	Client *c = srv.getClient(fd);
	if (!c)
		return false;
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
	if (!c)
		return;
	if (!c->registered && c->passOk && c->hasNick && c->hasUser)
	{
		c->registered = true;
		srv.sendNumeric(fd, 1, c->nick, "Welcome to the IRC server " + c->nick);
		srv.sendNumeric(fd, 2, c->nick, "Your host is " + srv.getServerName());
		srv.sendNumeric(fd, 3, c->nick, "This server was created for the project");
		srv.sendNumeric(fd, 4, c->nick, srv.getServerName() +" 1.0 iw itkol");
		srv.sendNumeric(fd, 5, c->nick, "CHANTYPES=# PREFIX=(o)@ CHANMODES=itkol NETWORK=" + srv.getServerName());
		srv.sendNumeric(fd, 375, c->nick, "- " + srv.getServerName() + " Message of the Day -");
		srv.sendNumeric(fd, 372, c->nick, "Welcome. This is a minimal IRC server.");
		srv.sendNumeric(fd, 376, c->nick, "End of /MOTD command.");
	}
}

// static void sendNames(Server &srv, int fd, const std::string &chan)
// {
// 	Client *c = srv.getClient(fd);
// 	Channel *ch = srv.getChannel(chan);
// 	if (!c || !ch)
// 		return;
// 	std::string names;
// 	for (std::set<int>::iterator it = ch->members.begin(); it != ch->members.end(); ++it)
// 	{
// 		Client *m = srv.getClient(*it);
// 		if (!m || !m->hasNick)
// 			continue;
// 		std::string one = m->nick;
// 		if (ch->isOp(*it))
// 			one = "@" + one;
// 		if (!names.empty()) names += " ";
// 		names += one;
// 	}
// 	srv.sendNumeric(fd, 353, c->nick, "= " + chan + " :" + names);
// 	srv.sendNumeric(fd, 366, c->nick, chan + " :End of /NAMES list");
// }

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
		if (ch->isOp(*it)) one = "@" + one;

		if (!names.empty()) names += " ";
		names += one;
	}

	std::vector<std::string> p353;
	p353.push_back(c->nick);
	p353.push_back("=");
	p353.push_back(chan);
	srv.sendNumeric(fd, 353, p353, names);

	std::vector<std::string> p366;
	p366.push_back(c->nick);
	p366.push_back(chan);
	srv.sendNumeric(fd, 366, p366, "End of /NAMES list");
}

static void cmdPASS(Server &srv, int fd, const Message &msg)
{
	Client *c = srv.getClient(fd);
	if (!c)
		return;
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
	if (!c)
		return;
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
	if (!c)
		return;
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

static std::string upper(const std::string &s)
{
	std::string o = s;
	for (size_t i = 0; i < o.size(); ++i)
		o[i] = (char)std::toupper((unsigned char)o[i]);
	return o;
}

static void cmdCAP(Server &srv, int fd, const Message &msg)
{
	Client *c = srv.getClient(fd);
	std::string nick = (c && c->hasNick) ? c->nick : "*";

	if (msg.params.empty())
		return;

	std::string sub = upper(msg.params[0]);

	if (sub == "LS")
	{
		srv.sendToClient(fd, ":" + srv.getServerName() + " CAP " + nick + " LS :multi-prefix\r\n");
		return;
	}

	if (sub == "REQ")
	{
		std::string req = msg.hasTrailing ? msg.trailing : (msg.params.size() >= 2 ? msg.params[1] : "");
		if (req.find("multi-prefix") != std::string::npos)
			srv.sendToClient(fd, ":" + srv.getServerName() + " CAP " + nick + " ACK :multi-prefix\r\n");
		else
			srv.sendToClient(fd, ":" + srv.getServerName() + " CAP " + nick + " NAK :" + req + "\r\n");
		return;
	}

	if (sub == "END")
		return;
}
static void cmdJOIN(Server &srv, int fd, const Message &msg)
{
	if (!ensureRegistered(srv, fd))
		return;
	Client *c = srv.getClient(fd);
	if (!c)
		return;
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
	std::vector<std::string> p;
	p.push_back(c->nick);
	p.push_back(chname);

	if (ch->topic.empty())
		srv.sendNumeric(fd, 331, p, "No topic is set");
	else
		srv.sendNumeric(fd, 332, p, ch->topic);
	sendNames(srv, fd, chname);
}

static void cmdPART(Server &srv, int fd, const Message &msg)
{
	if (!ensureRegistered(srv, fd))
		return;
	Client *c = srv.getClient(fd);
	if (!c)
		return;
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
	if (!ensureRegistered(srv, fd))
		return;
	Client *c = srv.getClient(fd);
	if (!c)
		return;
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
	std::vector<std::string> p;
	p.push_back(c->nick);
	p.push_back(nick);
	p.push_back(chname);
	srv.sendNumeric(fd, 341, p, "");
	std::string line = prefixOf(srv, fd) + " INVITE " + nick + " :" + chname + "\r\n";
	srv.sendToClient(tfd, line);
}

static void cmdKICK(Server &srv, int fd, const Message &msg)
{
	if (!ensureRegistered(srv, fd))
		return;
	Client *c = srv.getClient(fd);
	if (!c)
		return;
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
		std::vector<std::string> p;
		p.push_back(c->nick);
		p.push_back(chname);
		srv.sendNumeric(fd, 403, p, "No such channel");
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

static bool isModeToken(const std::string &tok)
{
	if (tok.empty())
		return false;
	if (tok[0] != '+' && tok[0] != '-')
		return false;
	for (size_t i = 0; i < tok.size(); ++i)
	{
		char c = tok[i];
		if (c != '+' && c != '-' &&
			c != 'i' && c != 't' && c != 'k' && c != 'l' && c != 'o')
			return false;
	}
	return true;
}

static void cmdMODE(Server &srv, int fd, const Message &msg)
{
	if (!ensureRegistered(srv, fd))
		return;
	Client *c = srv.getClient(fd);
	if (!c)
		return;
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
		std::vector<std::string> p;
		p.push_back(c->nick);
		p.push_back(target);
		srv.sendNumeric(fd, 403, p, "No such channel");
		return;
	}
	if (msg.params.size() == 1)
	{
		std::string modes = "+";
		std::vector<std::string> p;

		if (ch->modeInviteOnly) modes += "i";
		if (ch->modeTopicOpOnly) modes += "t";
		if (ch->modeKey) modes += "k";
		if (ch->modeLimit) modes += "l";
		p.push_back(c->nick);
		p.push_back(target);
		p.push_back(modes);
		if (ch->modeKey)
			p.push_back(ch->key);
		if (ch->modeLimit)
			p.push_back(itos(ch->userLimit));

		srv.sendNumeric(fd, 324, p, "");
		return;
	}
	if (!ch->isMember(fd))
	{
		std::vector<std::string> p;
		p.push_back(c->nick);
		p.push_back(target);
		srv.sendNumeric(fd, 442, p, "You're not on that channel");
		return;
	}
	if (!ch->isOp(fd))
	{
		std::vector<std::string> p;
		p.push_back(c->nick);
		p.push_back(target);
		srv.sendNumeric(fd, 482, p, "You're not channel operator");
		return;
	}
	std::string modeStr;
	size_t m = 1;
	while (m < msg.params.size() && isModeToken(msg.params[m]))
	{
		modeStr += msg.params[m];
		++m;
	}
	size_t argi = m;
	if (modeStr.empty())
	{
		srv.sendNumeric(fd, 501, c->nick, "Unknown MODE flag");
		return;
	}
	bool adding = true;
	std::string appliedModes;
	bool haveSign = false;
	bool lastSign = true;
	std::string announceParams;
	for (size_t i = 0; i < modeStr.size(); ++i)
	{
		char chm = modeStr[i];
		if (chm == '+') { adding = true;  continue; }
		if (chm == '-') { adding = false; continue; }

		if (!haveSign || adding != lastSign)
		{
			appliedModes += (adding ? "+" : "-");
			lastSign = adding;
			haveSign = true;
		}
		if (chm == 'i')
		{
			ch->modeInviteOnly = adding;
			appliedModes += "i";
		}
		else if (chm == 't')
		{
			ch->modeTopicOpOnly = adding;
			appliedModes += "t";
		}
		else if (chm == 'k')
		{
			if (adding)
			{
				if (msg.params.size() <= argi)
				{
					srv.sendNumeric(fd, 461, c->nick, "MODE :Not enough parameters");
					return;
				}
				ch->modeKey = true;
				ch->key = msg.params[argi++];
				appliedModes += "k";
				announceParams += " " + ch->key;
			}
			else
			{
				ch->modeKey = false;
				ch->key = "";
				appliedModes += "k";
			}
		}
		else if (chm == 'l')
		{
			if (adding)
			{
				if (msg.params.size() <= argi)
				{
					srv.sendNumeric(fd, 461, c->nick, "MODE :Not enough parameters");
					return;
				}
				ch->modeLimit = true;
				ch->userLimit = std::atoi(msg.params[argi].c_str());
				appliedModes += "l";
				announceParams += " " + msg.params[argi];
				++argi;
			}
			else
			{
				ch->modeLimit = false;
				ch->userLimit = 0;
				appliedModes += "l";
			}
		}
		else if (chm == 'o')
		{
			if (msg.params.size() <= argi)
			{
				srv.sendNumeric(fd, 461, c->nick, "MODE :Not enough parameters");
				return;
			}
			std::string nick = msg.params[argi++];
			int tfd = srv.fdByNick(nick);
			if (tfd < 0)
			{
				std::vector<std::string> p;
				p.push_back(c->nick);
				p.push_back(nick);
				srv.sendNumeric(fd, 401, p, "No such nick");
				return;
			}
			if (!ch->isMember(tfd))
			{
				std::vector<std::string> p;
				p.push_back(c->nick);
				p.push_back(nick);
				p.push_back(target);
				srv.sendNumeric(fd, 441, p, "They aren't on that channel");
				return;
			}
			if (adding)
				ch->operators.insert(tfd);
			else
				ch->operators.erase(tfd);
			appliedModes += "o";
			announceParams += " " + nick;
		}
		else
		{
			srv.sendNumeric(fd, 501, c->nick, "Unknown MODE flag");
			return;
		}
	}
	if (appliedModes.empty())
		return;
	std::string line = prefixOf(srv, fd) + " MODE " + target + " " + appliedModes + announceParams + "\r\n";
	srv.broadcastToChannel(target, -1, line);
}

static void cmdPRIVMSG(Server &srv, int fd, const Message &msg)
{
	if (!ensureRegistered(srv, fd))
		return;
	Client *c = srv.getClient(fd);
	if (!c)
		return;
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

// static void cmdQUIT(Server &srv, int fd, const Message &msg)
// {
// 	(void)msg;
// 	srv.disconnectClient(fd, "Client quit");
// }

static void cmdQUIT(Server &srv, int fd, const Message &msg)
{
	Client *c = srv.getClient(fd);
	std::string reason = msg.hasTrailing ? msg.trailing : "Client quit";

	if (c)
	{
		std::string line = prefixOf(srv, fd) + " QUIT :" + reason + "\r\n";
		for (std::set<std::string>::iterator it = c->channels.begin(); it != c->channels.end(); ++it)
			srv.broadcastToChannel(*it, fd, line);
	}

	srv.disconnectClient(fd, reason);
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
