/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Message.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mabou-ha <mabou-ha@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/28 12:38:59 by mabou-ha          #+#    #+#             */
/*   Updated: 2026/01/04 17:04:31 by mabou-ha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <vector>

class Message
{
	public:
		std::string raw;
		std::string command;
		std::vector<std::string> params;
		std::string trailing;
		bool hasTrailing;

		Message();

		static Message parse(const std::string &line);

	private:
		static std::string trim(const std::string &s);
		static std::string toUpper(const std::string &s);
};

#endif
