/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mabou-ha <mabou-ha@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/28 13:05:35 by mabou-ha          #+#    #+#             */
/*   Updated: 2026/01/12 01:44:11 by mabou-ha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include <iostream>
#include <cstdlib>

static bool isNumber(const char *s)
{
	if (!s || !*s) return false;
	for (int i = 0; s[i]; ++i)
		if (s[i] < '0' || s[i] > '9') return false;
	return true;
}

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		std::cerr << "Usage: ./ircserv <port> <password>\n";
		return 1;
	}
	if (!isNumber(argv[1]))
	{
		std::cerr << "Error: port must be numeric\n";
		return 1;
	}
	int port = std::atoi(argv[1]);
	if (port <= 0 || port > 65535)
	{
		std::cerr << "Error: invalid port\n";
		return 1;
	}
	std::string password = argv[2];
	if (password.empty())
	{
		std::cerr << "Error: password must not be empty\n";
		return 1;
	}
	try
	{
		Server srv(port, password);
		srv.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Fatal: " << e.what() << "\n";
		return 1;
	}
	return 0;
}

