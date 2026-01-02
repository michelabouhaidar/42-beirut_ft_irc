/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   IrcMessage.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mabou-ha <mabou-ha>                        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/28 13:05:32 by mabou-ha          #+#    #+#             */
/*   Updated: 2026/01/02 01:41:37 by mabou-ha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "IrcMessage.hpp"
#include <cctype>
#include <sstream>

IrcMessage::IrcMessage() : hasTrailing(false) {}

std::string IrcMessage::trim(const std::string &s)
{
    std::string::size_type b = 0;
    while (b < s.size() && (s[b] == ' ' || s[b] == '\t'))
        b++;
    std::string::size_type e = s.size();
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t'))
        e--;
    return s.substr(b, e - b);
}

std::string IrcMessage::toUpper(const std::string &s)
{
    std::string out = s;
    for (std::string::size_type i = 0; i < out.size(); i++)
        out[i] = (char)std::toupper((unsigned char)out[i]);
    return out;
}

IrcMessage IrcMessage::parse(const std::string &line)
{
    IrcMessage m;
    m.raw = line;

    std::string work = trim(line);
    if (work.empty())
        return m;

    // Strip optional prefix
    if (work[0] == ':')
    {
        std::string::size_type sp = work.find(' ');
        if (sp == std::string::npos)
            return m;
        work = trim(work.substr(sp + 1));
        if (work.empty())
            return m;
    }

    // Split trailing: everything after " :"
    std::string::size_type trailPos = work.find(" :");
    if (trailPos != std::string::npos)
    {
        m.hasTrailing = true;
        m.trailing = work.substr(trailPos + 2);
        work = trim(work.substr(0, trailPos));
    }

    std::istringstream iss(work);
    std::string cmd;
    if (!(iss >> cmd))
        return m;

    m.command = toUpper(cmd);

    std::string p;
    while (iss >> p)
        m.params.push_back(p);

    return m;
}
