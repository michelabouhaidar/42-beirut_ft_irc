/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mabou-ha <mabou-ha@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/28 13:05:37 by mabou-ha          #+#    #+#             */
/*   Updated: 2026/01/08 20:11:10 by mabou-ha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "Handlers.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <cerrno>
#include <csignal>

#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

static std::string itos(int x)
{
	std::ostringstream oss;
	oss << x;
	return oss.str();
}

Server::Server(int port, const std::string &password)
	: port_(port), password_(password), listenFd_(-1), running_(true), serverName_("ircserv")
{
	std::signal(SIGPIPE, SIG_IGN);
	setupListenSocket();
}

Server::~Server()
{
	if (listenFd_ != -1)
		close(listenFd_);

	for (std::map<int, Client>::iterator it = clients_.begin(); it != clients_.end(); ++it)
		close(it->first);
}

const std::string& Server::getPassword() const { return password_; }
const std::string& Server::getServerName() const { return serverName_; }

void Server::setNonBlocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0) 
		throw std::runtime_error("fcntl(F_GETFL) failed");
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
		throw std::runtime_error("fcntl(F_SETFL, O_NONBLOCK) failed");
}

void Server::setupListenSocket()
{
	listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
	if (listenFd_ < 0) 
		throw std::runtime_error("socket() failed");

	int yes = 1;
	if (setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
		throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");

	sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons((unsigned short)port_);

	if (bind(listenFd_, (sockaddr*)&addr, sizeof(addr)) < 0)
		throw std::runtime_error("bind() failed");

	if (listen(listenFd_, 128) < 0)
		throw std::runtime_error("listen() failed");

	setNonBlocking(listenFd_);
}

void Server::buildPollFds(std::vector<struct pollfd> &pfds)
{
	pfds.clear();

	pollfd p;
	p.fd = listenFd_;
	p.events = POLLIN;
	p.revents = 0;
	pfds.push_back(p);

	for (std::map<int, Client>::iterator it = clients_.begin(); it != clients_.end(); ++it)
	{
		pollfd c;
		c.fd = it->first;
		c.events = POLLIN;
		if (it->second.wantsWrite())
			c.events |= POLLOUT;
		c.revents = 0;
		pfds.push_back(c);
	}
}

void Server::run()
{
	while (running_)
	{
		std::vector<pollfd> pfds;
		buildPollFds(pfds);

		int ret = poll(&pfds[0], pfds.size(), 1000);
		if (ret < 0)
		{
			if (errno == EINTR) continue;
			continue;
		}

		for (std::vector<pollfd>::iterator it = pfds.begin(); it != pfds.end(); ++it)
		{
			if (it->fd == listenFd_)
			{
				if (it->revents & POLLIN)
					handleAccept();
			}
			else
			{
				int fd = it->fd;

				if (it->revents & (POLLHUP | POLLERR | POLLNVAL))
				{
					disconnectClient(fd, "Connection closed");
					continue;
				}
				if (it->revents & POLLIN)
					handleClientRead(fd);
				if (it->revents & POLLOUT)
					handleClientWrite(fd);
			}
		}
	}
}

void Server::handleAccept()
{
	while (true)
	{
		sockaddr_in addr;
		socklen_t len = sizeof(addr);
		int cfd = accept(listenFd_, (sockaddr*)&addr, &len);
		if (cfd < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			break;
		}

		setNonBlocking(cfd);

		std::string host = inet_ntoa(addr.sin_addr);
		clients_.insert(std::make_pair(cfd, Client(cfd, host)));
	}
}

Client* Server::getClient(int fd)
{
	std::map<int, Client>::iterator it = clients_.find(fd);
	if (it == clients_.end())
		return 0;
	return &it->second;
}

Channel* Server::getChannel(const std::string &name)
{
	std::map<std::string, Channel>::iterator it = channels_.find(name);
	if (it == channels_.end())
		return 0;
	return &it->second;
}

Channel* Server::getOrCreateChannel(const std::string &name)
{
	Channel *ch = getChannel(name);
	if (ch) 
		return ch;

	channels_.insert(std::make_pair(name, Channel(name)));
	return getChannel(name);
}

void Server::eraseChannel(const std::string &name)
{
	channels_.erase(name);
}

bool Server::nickExists(const std::string &nick) const
{
	for (std::map<int, Client>::const_iterator it = clients_.begin(); it != clients_.end(); ++it)
	{
		if (it->second.hasNick && it->second.nick == nick)
			return true;
	}
	return false;
}

int Server::fdByNick(const std::string &nick) const
{
	for (std::map<int, Client>::const_iterator it = clients_.begin(); it != clients_.end(); ++it)
	{
		if (it->second.hasNick && it->second.nick == nick)
			return it->first;
	}
	return -1;
}

void Server::handleClientRead(int fd)
{
	Client *c = getClient(fd);
	if (!c) return;

	char buf[4096];

	while (true)
	{
		ssize_t n = recv(fd, buf, sizeof(buf), 0);
		if (n == 0)
		{
			disconnectClient(fd, "Client quit");
			return;
		}
		if (n < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			disconnectClient(fd, "Read error");
			return;
		}

		c->inbuf.append(buf, buf + n);

		while (true)
		{
			std::string::size_type pos = c->inbuf.find('\n');
			if (pos == std::string::npos)
				break;
		
			std::string line = c->inbuf.substr(0, pos);
			c->inbuf.erase(0, pos + 1);
			if (!line.empty() && line[line.size() - 1] == '\r')
				line.erase(line.size() - 1);
		
			if (!line.empty())
				processLine(fd, line);
		}

	}
}

void Server::handleClientWrite(int fd)
{
	Client *c = getClient(fd);
	if (!c) return;

	while (!c->outbuf.empty())
	{
		ssize_t n = send(fd, c->outbuf.c_str(), c->outbuf.size(), 0);
		if (n > 0)
		{
			c->outbuf.erase(0, (std::string::size_type)n);
			continue;
		}
		if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
			return;

		disconnectClient(fd, "Write error");
		return;
	}
}

void Server::processLine(int fd, const std::string &line)
{
	Message msg = Message::parse(line);
	if (msg.command.empty())
		return;

	handleCommand(*this, fd, msg);
}

void Server::sendToClient(int fd, const std::string &msg)
{
	Client *c = getClient(fd);
	if (!c)
		return;
	c->queue(msg);
}

void Server::sendNumeric(int fd, int code, const std::string &target, const std::string &text)
{
	std::string c = itos(code);
	while (c.size() < 3) c = "0" + c;

	std::string line = ":" + serverName_ + " " + c + " " + target + " :" + text + "\r\n";
	sendToClient(fd, line);
}

void Server::broadcastToChannel(const std::string &chan, int exceptFd, const std::string &msg)
{
	Channel *ch = getChannel(chan);
	if (!ch) return;

	for (std::set<int>::iterator it = ch->members.begin(); it != ch->members.end(); ++it)
	{
		if (*it == exceptFd) continue;
		sendToClient(*it, msg);
	}
}

void Server::disconnectClient(int fd, const std::string &reason)
{
	Client *c = getClient(fd);

	if (c)
	{
		for (std::set<std::string>::iterator it = c->channels.begin(); it != c->channels.end(); ++it)
		{
			Channel *ch = getChannel(*it);
			if (ch)
			{
				ch->removeMember(fd);
				if (ch->members.empty())
					eraseChannel(ch->name);
			}
		}
		c->channels.clear();
	}
	std::string err = "ERROR :" + reason + "\r\n";
	::send(fd, err.c_str(), err.size(), 0);

	close(fd);
	clients_.erase(fd);
}
