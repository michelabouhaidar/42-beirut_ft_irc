/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   IrcMessage.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mabou-ha <mabou-ha>                        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/28 12:38:59 by mabou-ha          #+#    #+#             */
/*   Updated: 2026/01/02 01:41:28 by mabou-ha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef IRCMESSAGE_HPP
#define IRCMESSAGE_HPP

#include <string>
#include <vector>

class IrcMessage
{
    public:
        std::string raw;
        std::string command;
        std::vector<std::string> params;
        std::string trailing;
        bool hasTrailing;

        IrcMessage();

        static IrcMessage parse(const std::string &line);

    private:
        static std::string trim(const std::string &s);
        static std::string toUpper(const std::string &s);
};

#endif
