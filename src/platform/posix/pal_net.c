/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pal_net.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/21 21:05:00 by marvin            #+#    #+#             */
/*   Updated: 2026/11/21 21:05:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "pal.h"
#include "libft.h"
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

/* Resolve and connect to host:serv.
   proto: PAL_NET_TCP (SOCK_STREAM) or PAL_NET_UDP (SOCK_DGRAM).
   Tries each resolved address in turn and returns the first connected
   descriptor, matching bash /dev/tcp and /dev/udp behavior. */
int	pal_net_connect(const char *host, const char *serv, int proto)
{
	struct addrinfo		hints;
	struct addrinfo		*list;
	struct addrinfo		*ai;
	int					socktype;
	int					fd;

	if (proto == PAL_NET_UDP)
		socktype = SOCK_DGRAM;
	else
		socktype = SOCK_STREAM;
	ft_memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = socktype;
	if (getaddrinfo(host, serv, &hints, &list) != 0)
		return (errno = EINVAL, -1);
	ai = list;
	while (ai)
	{
		fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (fd >= 0 && connect(fd, ai->ai_addr, ai->ai_addrlen) == 0)
			return (freeaddrinfo(list), fd);
		if (fd >= 0)
			close(fd);
		ai = ai->ai_next;
	}
	return (freeaddrinfo(list), -1);
}
