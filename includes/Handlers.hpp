/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Handlers.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mabou-ha <mabou-ha@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/28 12:54:52 by mabou-ha          #+#    #+#             */
/*   Updated: 2026/01/12 01:59:42 by mabou-ha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HANDLERS_HPP
#define HANDLERS_HPP

#include "Server.hpp"
#include "Message.hpp"

void handleCommand(Server &srv, int fd, const Message &msg);

std::string itos(int x);

#endif
