/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mabou-ha <mabou-ha>                        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/28 12:47:36 by mabou-ha          #+#    #+#             */
/*   Updated: 2025/12/28 12:51:56 by mabou-ha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>

class Channel
{
	public:
		std::string name;
		std::string topic;
		std::set<int> members;
		std::set<int> operators;
		std::set<int> invited;
		bool modeInviteOnly;
		bool modeTopicOpOnly;
		bool modeKey;
		std::string key;
		bool modeLimit;
		int userLimit;

		Channel();
		Channel(const std::string &name_);
		
		bool isMember(int fd)const;
		bool isOp(int fd)const;
		void addMember(int fd);
		void removeMemeber(int fd);
};

#endif