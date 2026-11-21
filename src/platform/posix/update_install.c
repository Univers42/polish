/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   update_install.c                                   :+:      :+:    :+:   */
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

static int	first_field(const char *path, char *out, size_t n)
{
	int		fd;
	ssize_t	r;
	size_t	i;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return (0);
	r = read(fd, out, n - 1);
	close(fd);
	if (r <= 0)
		return (0);
	out[r] = '\0';
	i = 0;
	while (out[i] && out[i] != ' ' && out[i] != '\t' && out[i] != '\n')
		i++;
	out[i] = '\0';
	return (i == 64);
}

static int	sha256_of(const char *path, char *out, size_t n)
{
	char		buf[256];
	char		*argv[3];
	size_t		i;

	argv[0] = "sha256sum";
	argv[1] = (char *)path;
	argv[2] = NULL;
	if (update_capture(argv, buf, sizeof(buf)) <= 0)
		return (0);
	i = 0;
	while (buf[i] && buf[i] != ' ' && i + 1 < n)
	{
		out[i] = buf[i];
		i++;
	}
	out[i] = '\0';
	return (i == 64);
}

int	update_verify_sha(const char *tag, const char *asset, const char *file)
{
	char	url[1024];
	char	sumfile[1024];
	char	want[128];
	char	got[128];

	ft_strlcpy(sumfile, file, sizeof(sumfile));
	ft_strlcat(sumfile, ".sha256", sizeof(sumfile));
	ft_strlcpy(url, asset, sizeof(url));
	ft_strlcat(url, ".sha256", sizeof(url));
	if (!update_asset_url(tag, url, url + 512, sizeof(url) - 512))
		return (-1);
	if (!update_download(url + 512, sumfile, 32))
		return (unlink(sumfile), -1);
	if (!first_field(sumfile, want, sizeof(want)))
		return (unlink(sumfile), -1);
	unlink(sumfile);
	if (!sha256_of(file, got, sizeof(got)))
		return (0);
	return (ft_strcmp(want, got) == 0);
}
