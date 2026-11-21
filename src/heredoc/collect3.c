/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   collect3.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/10 04:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/10 04:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "heredoc_private.h"
#include "sys.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

static const char	*hdoc_tmpdir(void)
{
	char		var[8];
	const char	*dir;

	var[0] = 'T';
	var[1] = 'M';
	var[2] = 'P';
	var[3] = 'D';
	var[4] = 'I';
	var[5] = 'R';
	var[6] = '\0';
	dir = getenv(var);
	if (!dir || !*dir)
		return (TMP_DIR);
	return (dir);
}

/* Large-body fallback: an unlinked O_RDWR temp file (anonymous-file
   semantics -- no fname kept, no teardown unlink possible or needed).
   Uses mkstemp() with O_EXCL creation in $TMPDIR (fallback to TMP_DIR)
   to prevent symlink clobber and race conditions. */
static int	hdoc_file_backing(t_shell *state, t_string *body)
{
	t_string	fname;
	const char	*tmpdir;
	int			fd;

	(void)state;
	tmpdir = hdoc_tmpdir();
	vec_init(&fname);
	vec_push_str(&fname, (char *)tmpdir);
	if (fname.len > 0 && ((char *)fname.ctx)[fname.len - 1] != '/')
		vec_push_str(&fname, "/");
	vec_push_str(&fname, "polish_hdoc_XXXXXX");
	fd = mkstemp((char *)fname.ctx);
	if (fd < 0)
		critical_error_errno_ctx((char *)fname.ctx);
	unlink((char *)fname.ctx);
	xfree(fname.ctx);
	ft_assert(write_to_file((char *)body->ctx, fd) == 0);
	lseek(fd, 0, SEEK_SET);
	return (fd);
}

/* Give the redirect at `idx` its backing fd, now that the expanded body
   size is known.  Small bodies (<= HDOC_PIPE_MAX) get a PIPE, like bash
   >= 5.1: write the body into the write end, close it, hand the read end
   to the redirect -- zero filesystem traffic, which matters when a loop
   body re-materializes its heredoc every iteration.  A fresh pipe's
   kernel buffer always holds PIPE_BUF bytes without a reader, so the
   write cannot block.  Bigger bodies fall back to the unlinked temp
   file.  The pointer into state->redirects is taken HERE, after body
   expansion, because expansion can push new entries and realloc the
   vector. */
void	hdoc_attach_backing(t_shell *state, int idx, t_string *body)
{
	t_redir	*r;
	int		p[2];

	r = &((t_redir *)state->redirects.ctx)[idx];
	if (body->len <= HDOC_PIPE_MAX)
	{
		if (pal_pipe(p))
			critical_error_errno_ctx("pipe");
		if (body->len)
			ft_assert(write_to_file((char *)body->ctx, p[1]) == 0);
		close(p[1]);
		r->fd = p[0];
		return ;
	}
	r->fd = hdoc_file_backing(state, body);
}
