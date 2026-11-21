/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_norm.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/20 00:50:00 by marvin            #+#    #+#             */
/*   Updated: 2026/08/20 00:50:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "update.h"
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

int	update_download(const char *url, const char *dest, long min)
{
	char		out[64];
	const char	*proto;
	char		*argv[10];
	struct stat	st;

	proto = "=https";
	if (ft_strncmp(url, "http://127.0.0.1", 16) == 0
		|| ft_strncmp(url, "http://localhost", 16) == 0)
		proto = "=http";
	argv[0] = "curl";
	argv[1] = "-fsSL";
	argv[2] = "--proto";
	argv[3] = (char *)proto;
	argv[4] = "--max-time";
	argv[5] = "60";
	argv[6] = "-o";
	argv[7] = (char *)dest;
	argv[8] = (char *)url;
	argv[9] = NULL;
	if (update_capture(argv, out, sizeof(out)) < 0)
		return (0);
	if (stat(dest, &st) != 0 || st.st_size < min)
		return (0);
	return (1);
}
