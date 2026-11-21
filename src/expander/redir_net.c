/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   redir_net.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:27:47 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:27:47 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* bash's /dev/tcp and /dev/udp virtual redirection targets (issue #16).

   These paths do not exist on any filesystem -- `ls /dev/tcp` fails on a
   stock Linux box -- so a shell that just calls open(2) reports "No such
   file or directory", which is what hellish did.  bash intercepts the path
   before it ever reaches the filesystem and hands back a connected socket
   instead.  ksh does the same; dash does not, so this is an extension, not
   a POSIX requirement.

   The parse is bash's, measured rather than assumed: after the nine-byte
   prefix the host runs to the NEXT slash and the service is everything
   after it, slashes included.  That is why `/dev/tcp/1.2.3.4/80/x` reports
   a bad service name rather than a bad path, and why `/dev/tcp/1.2.3.4`
   with no service at all is not a network path -- it falls through to the
   normal file open and gets "No such file or directory", exactly as bash
   leaves it.

   Platform networking details (socket, connect, getaddrinfo) are abstracted
   behind the PAL layer pal_net_connect(). */

#include "expander_private.h"
#include "pal.h"

/* PAL_NET_TCP for /dev/tcp/, PAL_NET_UDP for /dev/udp/, -1 for anything
   else.  Only the prefix is judged here; whether the rest is well formed
   is net_redir_open's problem. */
static int	net_proto(const char *fname)
{
	if (ft_strncmp(fname, "/dev/tcp/", 9) == 0)
		return (PAL_NET_TCP);
	if (ft_strncmp(fname, "/dev/udp/", 9) == 0)
		return (PAL_NET_UDP);
	return (-1);
}

/* Copy the host out of "host/service" into buf and answer with a pointer
   to the service.  NULL means "no slash after the host", i.e. not a
   network path at all -- the caller must then let the normal file open
   have it, because that is the case bash reports as a missing file. */
static const char	*net_split(const char *rest, char *buf, size_t n)
{
	size_t	len;

	len = 0;
	while (rest[len] && rest[len] != '/')
		len++;
	if (rest[len] != '/' || len >= n)
		return (NULL);
	ft_memcpy(buf, rest, len);
	buf[len] = '\0';
	return (rest + len + 1);
}

/* -1 = not a network path, leave it to the file open; 0 = it was one and
   it failed (errno is set, and the caller prints strerror for it); 1 = a
   connected socket is in ret->fd.

   The fd shuffle at the end is the same one open_file_redir needs and for
   the same reason: socket(2) hands back the lowest free descriptor, which
   can be the very fd this redirect targets, and the apply step would then
   be a no-op that the teardown promptly closes. */
int	net_redir_open(char *fname, t_redir *ret)
{
	char			host[256];
	const char		*serv;
	int				proto;

	proto = net_proto(fname);
	if (proto < 0)
		return (-1);
	serv = net_split(fname + 9, host, sizeof(host));
	if (!serv)
		return (-1);
	ret->fd = pal_net_connect(host, serv, proto);
	if (ret->fd < 0)
		return (0);
	if (!redir_park_fd(ret))
		return (0);
	ret->should_delete = false;
	return (1);
}
