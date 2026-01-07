/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mabou-ha <mabou-ha>                        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/28 12:52:19 by mabou-ha          #+#    #+#             */
/*   Updated: 2026/01/07 02:43:08 by mabou-ha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <map>
#include <vector>
#include <poll.h>

#include "Client.hpp"
#include "Message.hpp"
#include "Channel.hpp"

class Server
{
	public:
		Server(int port, const std::string &password);
		~Server();

		void run();

		void sendToClient(int fd, const std::string &msg);
		void sendNumeric(int fd, int code, const std::string &target, const std::string &text);
		void broadcastToChannel(const std::string &chan, int exceptFd, const std::string &msg);

		bool nickExists(const std::string &nick) const;
		int fdByNick(const std::string &nick) const;

		Client *getClient(int fd);
		Channel *getChannel(const std::string &name);

		Channel *getOrCreateChannel(const std::string &name);
		void eraseChannel(const std::string &name);

		const std::string& getPassword() const;
		const std::string& getServerName() const;

		void disconnectClient(int fd, const std::string &reason);

	private:
		int port_;
		std::string password_;

		int listenFd_;
		bool running_;

		std::string serverName_;
		std::map<int, Client> clients_;
		std::map<std::string, Channel> channels_;

		void setupListenSocket();
		void setNonBlocking(int fd);
		void buildPollFds(std::vector<struct pollfd> &pfds);

		void handleAccept();
		void handleClientRead(int fd);
		void handleClientWrite(int fd);

		void processLine(int fd, const std::string &line);
};

#endif
