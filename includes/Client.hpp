/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mabou-ha <mabou-ha>                        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/28 12:43:03 by mabou-ha          #+#    #+#             */
/*   Updated: 2026/01/07 02:16:18 by mabou-ha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <set>

class Client
{
	public:
		int fd;
		std::string host;

		std::string inbuf;
		std::string outbuf;

		bool passOk;
		bool hasNick;
		bool hasUser;
		bool registered;

		std::string nick;
		std::string user;

		std::set<std::string> channels;

		Client();
		Client(int fd_, const std::string &host_);

		void queue(const std::string &data);
		bool wantsWrite() const;
};

#endif
