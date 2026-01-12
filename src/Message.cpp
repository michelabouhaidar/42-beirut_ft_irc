/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Message.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mabou-ha <mabou-ha@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/28 13:05:32 by mabou-ha          #+#    #+#             */
/*   Updated: 2026/01/12 01:54:23 by mabou-ha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Message.hpp"
#include <cctype>
#include <sstream>

Message::Message() : hasTrailing(false) {}

std::string Message::trim(const std::string &s)
{
	std::string::size_type b = 0;
	while (b < s.size() && (s[b] == ' ' || s[b] == '\t'))
		b++;
	std::string::size_type e = s.size();
	while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t'))
		e--;
	return s.substr(b, e - b);
}

std::string Message::toUpper(const std::string &s)
{
	std::string out = s;
	for (std::string::size_type i = 0; i < out.size(); i++)
		out[i] = (char)std::toupper((unsigned char)out[i]);
	return out;
}

Message Message::parse(const std::string &line)
{
	Message m;
	m.raw = line;
	std::string s = trim(line);
	if (s.empty())
		return m;
	if (s[0] == ':')
	{
		std::string::size_type sp = s.find(' ');
		if (sp == std::string::npos)
			return m;
		s = trim(s.substr(sp + 1));
		if (s.empty())
			return m;
	}
	std::string::size_type trailPos = s.find(" :");
	if (trailPos != std::string::npos)
	{
		m.hasTrailing = true;
		m.trailing = s.substr(trailPos + 2);
		s = trim(s.substr(0, trailPos));
	}
	std::istringstream iss(s);
	std::string cmd;
	if (!(iss >> cmd))
		return m;
	m.command = toUpper(cmd);
	std::string p;
	while (iss >> p)
		m.params.push_back(p);
	return m;
}
