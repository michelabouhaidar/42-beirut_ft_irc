/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mabou-ha <mabou-ha>                        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/28 13:05:27 by mabou-ha          #+#    #+#             */
/*   Updated: 2026/01/07 02:17:19 by mabou-ha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"

Client::Client()
	: fd(-1),
	host(""),
	passOk(false),
	hasNick(false),
	hasUser(false),
	registered(false) {}

Client::Client(int fd_, const std::string &host_)
	: fd(fd_), host(host_),
	  passOk(false), hasNick(false), hasUser(false), registered(false) {}

void Client::queue(const std::string &data)
{
	outbuf += data;
}

bool Client::wantsWrite() const
{
	return !outbuf.empty();
}
