/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mabou-ha <mabou-ha>                        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/28 13:05:25 by mabou-ha          #+#    #+#             */
/*   Updated: 2026/01/07 02:24:59 by mabou-ha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Channel.hpp"

Channel::Channel()
	: name(""), topic(""),
	modeInviteOnly(false), modeTopicOpOnly(false),
	modeKey(false), key(""),
	modeLimit(false), userLimit(0) {}

Channel::Channel(const std::string &name_)
	: name(name_), topic(""),
	modeInviteOnly(false), modeTopicOpOnly(false),
	modeKey(false), key(""),
	modeLimit(false), userLimit(0) {}

bool Channel::isMember(int fd) const
{
	return members.find(fd) != members.end();
}

bool Channel::isOp(int fd) const
{
	return operators.find(fd) != operators.end();
}

void Channel::addMember(int fd)
{
	members.insert(fd);
}

void Channel::removeMember(int fd)
{
	members.erase(fd);
	operators.erase(fd);
	invited.erase(fd);
}

bool Channel::isFull() const
{
	if (!modeLimit) return false;
	return (int)members.size() >= userLimit;
}
