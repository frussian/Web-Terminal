#define _XOPEN_SOURCE 600
#include "panther.h"
#include "edexec.h"
#define THIS_MODULE_ID 186
#include "pto_iface.h"

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/wait.h>


static void ed_buf_entry_free(struct ED_BUF*eb, struct ED_BUF_ENTRY*ee)
{
	list_remove(&eb->entries, ee);
	free(ee);
}

int ed_buf_init(struct ED_BUF*eb, int maxsize)
{
	list_clear(&eb->entries);
	eb->mem = 0;
	eb->maxmem = maxsize;
	return 0;
}

int ed_buf_term(struct ED_BUF*eb)
{
	struct ED_BUF_ENTRY*ee;
	while ((ee = list_first(&eb->entries))) ed_buf_entry_free(eb, ee);
	return 0;
}


int ed_buf_empty(struct ED_BUF*eb)
{
	return eb->mem == 0;
}

int ed_buf_full(struct ED_BUF*eb)
{
	return eb->maxmem && eb->mem == eb->maxmem;
}

int ed_buf_put(struct ED_BUF*eb, const void*data, int len)
{
	int len1 = len;
	struct ED_BUF_ENTRY*ee;
	if (eb->maxmem) {
		int rem = eb->maxmem - eb->mem;
		if (rem < len1) len1 = rem;
	}
	if (!len) return 0; // no empty entries
	ee = malloc(sizeof(*ee) + len1);
	if (!ee) return -1; // allocation error
	ee->len = len1;
	ee->ofs = 0;
	eb->mem += len1;
	memcpy(ee->data, data, len1);
	list_append(&eb->entries, ee);
	return len1;
}



static int ed_buf_get_piece(struct ED_BUF*eb, void*data, int maxlen)
{
	struct ED_BUF_ENTRY*ee;
	int rem;
	ee = list_first(&eb->entries);
	if (!ee) return 0; // buffer is empty;
	rem = ee->len - ee->ofs;
	if (rem > maxlen) rem = maxlen;
	memcpy(data, ee->data + ee->ofs, rem);
	ee->ofs += rem;
	eb->mem -= rem;
	if (ee->ofs == ee->len) ed_buf_entry_free(eb, ee);
	return rem;
}

int ed_buf_get(struct ED_BUF*eb, void*data, int maxlen)
{
	int res = 0;
	while (maxlen) {
		int r = ed_buf_get_piece(eb, data, maxlen);
		if (r < 0) return r;
		if (!r) break;
		data = ((char*)data) + r;
		res += r;
		maxlen -= r;
	}
	return res;
}

int ed_buf_preview(struct ED_BUF*eb, void**data)
{
	struct ED_BUF_ENTRY*ee;
	ee = list_first(&eb->entries);
	if (!ee) return 0; // buffer is empty;
	*data = ee->data;
	return ee->len - ee->ofs;
}

int ed_buf_remains(struct ED_BUF*eb)
{
	return eb->maxmem?eb->maxmem - eb->mem: -1;
}

int ed_buf_commit(struct ED_BUF*eb, int was_used)
{
	struct ED_BUF_ENTRY*ee;
	ee = list_first(&eb->entries);
	if (!ee) return -1; // buffer is empty;
	if (ee->ofs + was_used > ee->len) return -1;
	ee->ofs += was_used;
	eb->mem -= was_used;
	if (ee->ofs == ee->len) ed_buf_entry_free(eb, ee);
	return 0;
}





static struct ED_PROCESS_STATE*new_process(struct ED_EXEC_POOL*ep, const struct PANTHER_SEXEC_REQUEST*req, int len)
{
	struct ED_PROCESS_STATE*res;
	int i;
	
	res = calloc(1, sizeof(*res));
	if (!res) return res;
	res->cmd = malloc(len - sizeof(*req) + 1);
	if (!res->cmd) { free(res); return NULL; }
	memcpy(res->cmd, req->data, len - sizeof(*req));
	res->cmd[len - sizeof(*req)] = 0;
	res->ep = ep;
	res->cmd_id = req->cmd_id;
	res->exec_flags = req->exec_flags;
	for (i = 0; i < EDEXEC_MAX_CHANS; ++i) res->pipes[i] = -1;
	
	return res;
}


static int end_process(struct ED_PROCESS_STATE*ps, int sig)
{
	int i;
	DEBUG(DEVEL, "end_process: %d, %d", ps->cmd_id, sig);
	list_remove(&ps->ep->processes, ps);
	if (!ps->finished) {
		if (sig) kill(ps->pid, sig);
	}
	for (i = 0; i < EDEXEC_MAX_CHANS; ++i) {
		if (ps->pipes[i] != -1) close(ps->pipes[i]);
		ed_buf_term(ps->bufs + i);
	}
	if (ps->cmd) free(ps->cmd);
	free(ps);
	return 0;
}

static struct ED_PROCESS_STATE*find_process(struct ED_EXEC_POOL*ep, int cmd_id)
{
	struct ED_PROCESS_STATE*ps;
	for (ps = list_first(&ep->processes); ps; ps = list_next(ps)) {
		if (ps->cmd_id == cmd_id) return ps;
	}
	return NULL;
}


static int exec_process(const char*cmd, int len)
{
	char *cmdname;
	cmdname = malloc(len + 1);
	memcpy(cmdname, cmd, len);
	cmdname[len] = 0;
	return execlp("sh", "/bin/sh", "-c", cmdname, NULL);
}

static int run_detached(struct ED_EXEC_POOL*ep, const char*cmd, int len)
{
	int r;
	DEBUG(DEVEL2, "detached cmd = %.*s", len, cmd);
	r = fork();
	if (r < 0) return r;
	if (!r) {
		close(0);
		close(1);
		close(2);
		r = fork(); // second fork to detach from a process
		if (r < 0) exit(1);
		if (!r) {
			return exec_process(cmd, len);
		}
		exit(0);
	}
	waitpid(r, NULL, 0); // do not make zombie
	return 0;
}


static int sexec_exec(struct ED_EXEC_POOL*ep, const struct PANTHER_SEXEC_REQUEST*req, int len)
{
	struct ED_PROCESS_STATE*ps;
	int pipes[EDEXEC_MAX_CHANS][2], master = -1, slave = -1, r;
	const char*ptname = NULL;
	if (req->exec_flags & PFLAG_SEXEC_DETACH) return run_detached(ep, req->data, len - sizeof(*req));
	ps = find_process(ep, req->cmd_id);
	if (ps) {
		ERROR(2, "already has process with id %d", req->cmd_id);
		return -2;
	}
	ps = new_process(ep, req, len);
	if (!ps) {
		ERROR(5, "unable to allocate new process with id %d", req->cmd_id);
		return -5;
	}
	if (ps->exec_flags & PFLAG_SEXEC_NEWPTY) {
		r = master = posix_openpt(O_RDWR | O_NOCTTY);
		if (r < 0) {
			ERROR(6, "unable to allocate master PTY");
			return -6;
		}
		fcntl(master, F_SETFL, O_NONBLOCK);
		r = grantpt(master);
		if (r < 0) {
			ERROR(6, "unable to grant master PTY");
			return -6;
		}
		r = unlockpt(master);
		if (r < 0) {
			ERROR(6, "unable to unlock master PTY");
			return -6;
		}
		ptname = ptsname(master);
		DEBUG(DEVEL, "started new PTY %s with master fd = %d", ptname, master);
	} else {
		if (ps->exec_flags & PFLAG_SEXEC_STDIN) {
			pipe(pipes[EDEXEC_CHAN_STDIN]);
		}
		if (ps->exec_flags & PFLAG_SEXEC_STDOUT) {
			pipe(pipes[EDEXEC_CHAN_STDOUT]);
		}
		if (ps->exec_flags & PFLAG_SEXEC_STDERR) {
			pipe(pipes[EDEXEC_CHAN_STDERR]);
		}
	}
	DEBUG(DEVEL2, "cmd_id = %d, cmd = %.*s", ps->cmd_id, (int)(len - sizeof(*req)), req->data);
	ps->pid = fork();
	if (!ps->pid) {
		if (ps->exec_flags & PFLAG_SEXEC_NEWPTY) {
			setsid();
			if (ptname) {
				slave = open(ptname, O_RDWR); // make controlling terminal
				dup2(slave, 0);
				dup2(slave, 1);
				dup2(slave, 2);
				close(slave);
			}
			close(master);
		} else {
			if (ps->exec_flags & PFLAG_SEXEC_STDIN) {
				dup2(pipes[EDEXEC_CHAN_STDIN][0], 0);
				close(pipes[EDEXEC_CHAN_STDIN][1]);
			}
			if (ps->exec_flags & PFLAG_SEXEC_STDOUT) {
				dup2(pipes[EDEXEC_CHAN_STDOUT][1], 1);
				close(pipes[EDEXEC_CHAN_STDOUT][0]);
			}
			if (ps->exec_flags & PFLAG_SEXEC_STDERR) {
				dup2(pipes[EDEXEC_CHAN_STDERR][1], 2);
				close(pipes[EDEXEC_CHAN_STDERR][0]);
			}
		}
		exec_process(req->data, len - sizeof(*req));
	} else {
		DEBUG(DEVEL2, "cmd_id = %d, pid = %d, cmd = %.*s", ps->cmd_id, ps->pid, (int)(len - sizeof(*req)), req->data);
		if (ps->exec_flags & PFLAG_SEXEC_NEWPTY) {
			ps->pipes[EDEXEC_CHAN_PTY] = master;
			slave=open(ptname, O_RDWR | O_NOCTTY);
			ed_buf_init(ps->bufs + EDEXEC_CHAN_STDIN, 0);
			ed_buf_init(ps->bufs + EDEXEC_CHAN_STDOUT, 1024);
		} else {
			if (ps->exec_flags & PFLAG_SEXEC_STDIN) {
				close(pipes[EDEXEC_CHAN_STDIN][0]);
				ps->pipes[EDEXEC_CHAN_STDIN] = pipes[EDEXEC_CHAN_STDIN][1];
				fcntl(ps->pipes[EDEXEC_CHAN_STDIN], F_SETFL, O_NONBLOCK);
				ed_buf_init(ps->bufs + EDEXEC_CHAN_STDIN, 0);
			}
			if (ps->exec_flags & PFLAG_SEXEC_STDOUT) {
				close(pipes[EDEXEC_CHAN_STDOUT][1]);
				ps->pipes[EDEXEC_CHAN_STDOUT] = pipes[EDEXEC_CHAN_STDOUT][0];
				fcntl(ps->pipes[EDEXEC_CHAN_STDOUT], F_SETFL, O_NONBLOCK);
				ed_buf_init(ps->bufs + EDEXEC_CHAN_STDOUT, 1024);
			}
			if (ps->exec_flags & PFLAG_SEXEC_STDERR) {
				close(pipes[EDEXEC_CHAN_STDERR][1]);
				ps->pipes[EDEXEC_CHAN_STDERR] = pipes[EDEXEC_CHAN_STDERR][0];
				fcntl(ps->pipes[EDEXEC_CHAN_STDERR], F_SETFL, O_NONBLOCK);
				ed_buf_init(ps->bufs + EDEXEC_CHAN_STDERR, 1024);
			}
		}
	}
	list_append(&ep->processes, ps);
	DEBUG(DEVEL, "started new process with id %d", req->cmd_id);
	return 0;
}

static int sexec_stdin(struct ED_EXEC_POOL*ep, const struct PANTHER_SEXEC_REQUEST*req, int len)
{
	struct ED_PROCESS_STATE*ps;
	ps = find_process(ep, req->cmd_id);
	if (!ps) {
		ERROR(2, "unable to find process with id %d", req->cmd_id);
		return -2;
	}
	if (ps->finished || !ps->pid) {
		ERROR(3, "process with id %d is already finished", req->cmd_id);
		return -3;
	}
	if (ps->pipes[EDEXEC_CHAN_STDIN] == -1) {
		ERROR(4, "process with id %d has no stdin", req->cmd_id);
		return -4;
	}
	if (req->exec_flags & PFLAG_SEXEC_DETACH) ps->close_stdin = 1;
	return ed_buf_put(ps->bufs + EDEXEC_CHAN_STDIN, req->data, len - sizeof(*req));
}

static int sexec_term(struct ED_EXEC_POOL*ep, const struct PANTHER_SEXEC_REQUEST*req, int len)
{
	int sig = SIGTERM;
	struct ED_PROCESS_STATE*ps;
	if (len - sizeof(*req) == sizeof(sig)) {
		memcpy(&sig, req->data, sizeof(sig));
	} else if (len != sizeof(*req)) {
		ERROR(1, "invalid term parameters");
		return -1;
	}
	ps = find_process(ep, req->cmd_id);
	if (!ps) {
		ERROR(2, "unable to find process with id %d", req->cmd_id);
		return -2;
	}
	if (ps->finished || !ps->pid) {
		ERROR(3, "process with id %d is already finished", req->cmd_id);
		return -3;
	}
	if (kill(ps->pid, sig) == -1) {
		STDERROR(errno, "unable to kill pid %d with signal %d (cmd_id = %d)", ps->pid, sig, req->cmd_id);
		return -4;
	}
	DEBUG(DEVEL, "sent signal %d to process %d", sig, ps->pid);
	return 0;
}


int ed_exec_init(struct ED_EXEC_POOL*ep)
{
	DEBUG(DEVEL, "initializing exec subsystem");
	list_clear(&ep->processes);
	return 0;
}

int ed_exec_term(struct ED_EXEC_POOL*ep)
{
	struct ED_PROCESS_STATE*ps;
	DEBUG(DEVEL, "terminating exec subsystem");
	while ((ps = list_first(&ep->processes))) end_process(ps, SIGHUP);
	return 0;
}

int ed_exec_put(struct ED_EXEC_POOL*ep, const struct PANTHER_SEXEC_REQUEST*req, int len)
{
	switch (req->mode) {
	case PFLAG_SEXEC_MODE_EXEC:
		return sexec_exec(ep, req, len);
	case PFLAG_SEXEC_MODE_STDIN:
		return sexec_stdin(ep, req, len);
	case PFLAG_SEXEC_MODE_TERM:
		return sexec_term(ep, req, len);
	}
	return 0;
}

int ed_exec_get(struct ED_EXEC_POOL*ep, struct PANTHER_SEXEC_RESULT*res, int maxlen)
{
	struct ED_PROCESS_STATE*ps;
	
	// check for finished processes first
	for (ps = list_first(&ep->processes); ps; ps = list_next(ps)) {
		if (ps->finished == 1 && maxlen > sizeof(*res) + sizeof(ps->status)) {
			int i;
			for (i = 0; i < EDEXEC_MAX_CHANS; ++i) {
				if (ps->pipes[i] != -1) break;
				if (i > EDEXEC_CHAN_STDIN && !ed_buf_empty(ps->bufs + i)) break;
			}
			if (i != EDEXEC_MAX_CHANS) continue;
			res->cmd_id = ps->cmd_id;
			res->stream_id = PSTREAM_SEXEC_CMDRES;
			memcpy(res->data, &ps->status, sizeof(ps->status));
			ps->finished = 2; // ready to be deleted if no pipes are ready
			return sizeof(ps->status) + sizeof(*res);
		}
	}
	// check for stderr second
	for (ps = list_first(&ep->processes); ps; ps = list_next(ps)) {
		int r;
		if (ed_buf_empty(ps->bufs + EDEXEC_CHAN_STDERR)) continue;
		r = ed_buf_get(ps->bufs + EDEXEC_CHAN_STDERR, res->data, maxlen - sizeof(*res));
		if (r < 0) return r;
		res->cmd_id = ps->cmd_id;
		res->stream_id = PSTREAM_SEXEC_STDERR;
		return r + sizeof(*res);
	}
	// check for stdout third
	for (ps = list_first(&ep->processes); ps; ps = list_next(ps)) {
		int r;
		if (ed_buf_empty(ps->bufs + EDEXEC_CHAN_STDOUT)) continue;
		r = ed_buf_get(ps->bufs + EDEXEC_CHAN_STDOUT, res->data, maxlen - sizeof(*res));
/*				{
					FILE*f = fopen("/data.txt", "ab");
					if (f) { fwrite(res->data, 1, r, f); fclose(f); }
				}*/
		if (r < 0) return r;
		res->cmd_id = ps->cmd_id;
		res->stream_id = PSTREAM_SEXEC_STDOUT;
		return r + sizeof(*res);
	}
	return 0; // no data
}

int ed_exec_hup(struct ED_EXEC_POOL*ep)
{
	struct ED_PROCESS_STATE*ps;
	DEBUG(DEVEL, "hanguping exec processes");
	for (ps = list_first(&ep->processes); ps; ps = list_next(ps)) {
		if (!ps->finished && ps->pid) {
			kill(ps->pid, SIGHUP);
		}
	}
	return 0;
}


int ed_exec_update(struct ED_EXEC_POOL*ep, int msec)
{
	struct ED_PROCESS_STATE*ps, *psnext;
	struct pollfd polls[EDEXEC_MAX_POLL];
	struct ED_PROCESS_STATE*procs[EDEXEC_MAX_POLL];
	int procchans[EDEXEC_MAX_POLL];
	int npolls = 0, r, i;
	// check for processes to remove first
	for (ps = list_first(&ep->processes); ps; ) {
		psnext = list_next(ps);
/*		DEBUG(DEVEL, "cmd_id = %d, finished = %d, pipes = %d, %d, %d, buffer = %d %d %d",
			ps->cmd_id, ps->finished, ps->pipes[0], ps->pipes[1], ps->pipes[2],
			ps->bufs[0].mem, ps->bufs[1].mem, ps->bufs[2].mem);*/
		if (ps->finished == 2) {
			int i;
			for (i = 0; i < EDEXEC_MAX_CHANS; ++i) {
				if (ps->pipes[i] != -1) break;
				if (i > EDEXEC_CHAN_STDIN && !ed_buf_empty(ps->bufs + i)) break;
			}
			if (i == EDEXEC_MAX_CHANS) { // remove descriptor
				DEBUG(DEVEL, "removing cmd %d", ps->cmd_id);
				end_process(ps, 0);
			}
		}
		ps = psnext;
	}
	// check for finished processes second
	for (ps = list_first(&ep->processes); ps; ps = list_next(ps)) {
		if (!ps->finished && ps->pid) {
			int r;
			r = waitpid(ps->pid, &ps->status, WNOHANG);
			if (!r) continue;
			if (r < 0) {
				STDERROR(errno, "unable to wait process with pid %d and cmd_id %d", ps->pid, ps->cmd_id);
				return r;
			}
			DEBUG(DEVEL, "waitpid(pid = %d, cmd_id = %d) = %d (%d)", ps->pid, ps->cmd_id, r, ps->status);
			if (ps->exec_flags & PFLAG_SEXEC_GETRES) ps->finished = 1;
			else ps->finished = 2;
			if (ps->exec_flags & PFLAG_SEXEC_NEWPTY) {
				close(ps->pipes[EDEXEC_CHAN_PTY]);
				ps->pipes[EDEXEC_CHAN_PTY] = -1;
			}
		}
	}
	// check for pipes to read/write third
	for (ps = list_first(&ep->processes); ps; ps = list_next(ps)) {
		if (ps->exec_flags & PFLAG_SEXEC_NEWPTY) {
			polls[npolls].fd = ps->pipes[EDEXEC_CHAN_PTY];
			polls[npolls].events = 0;
			if (!ed_buf_empty(ps->bufs + EDEXEC_CHAN_STDIN)) polls[npolls].events |= POLLOUT;
			if (!ed_buf_full(ps->bufs + EDEXEC_CHAN_STDOUT)) polls[npolls].events |= POLLIN;
			procs[npolls] = ps;
			procchans[npolls] = EDEXEC_CHAN_PTY;
			if (npolls < EDEXEC_MAX_POLL - 1) ++npolls;
		} else {
			if (ps->pipes[EDEXEC_CHAN_STDIN] != -1 && !ed_buf_empty(ps->bufs + EDEXEC_CHAN_STDIN)) {
				polls[npolls].fd = ps->pipes[EDEXEC_CHAN_STDIN];
				polls[npolls].events = POLLOUT;
				procs[npolls] = ps;
				procchans[npolls] = EDEXEC_CHAN_STDIN;
				if (npolls < EDEXEC_MAX_POLL - 1) ++npolls;			
			} else {
				if (ps->close_stdin && ps->pipes[EDEXEC_CHAN_STDIN] != -1) {
					close(ps->pipes[EDEXEC_CHAN_STDIN]);
					ps->pipes[EDEXEC_CHAN_STDIN] = -1;
					DEBUG(DEVEL, "closing stdin for %d by request", ps->cmd_id);
				}
			}
			if (ps->pipes[EDEXEC_CHAN_STDOUT] != -1 && !ed_buf_full(ps->bufs + EDEXEC_CHAN_STDOUT)) {
				polls[npolls].fd = ps->pipes[EDEXEC_CHAN_STDOUT];
				polls[npolls].events = POLLIN;
				procs[npolls] = ps;
				procchans[npolls] = EDEXEC_CHAN_STDOUT;
				if (npolls < EDEXEC_MAX_POLL - 1) ++npolls;			
			}
			if (ps->pipes[EDEXEC_CHAN_STDERR] != -1 && !ed_buf_full(ps->bufs + EDEXEC_CHAN_STDERR)) {
				polls[npolls].fd = ps->pipes[EDEXEC_CHAN_STDERR];
				polls[npolls].events = POLLIN;
				procs[npolls] = ps;
				procchans[npolls] = EDEXEC_CHAN_STDERR;
				if (npolls < EDEXEC_MAX_POLL - 1) ++npolls;			
			}
		}
	}
	if (!npolls) return 0; // nothing to poll
//	DEBUG(DEVEL2, "polling %d poll(s) for %d msec", npolls, msec);
	r = poll(polls, npolls, msec);
	if (r < 0) {
		STDERROR(errno, "poll error");
		return r;
	}
	if (!r) return 0;
	for (i = 0; r && i < npolls; ++i) {
		int ch;
		char buf[1024];
		int rd;
		if (!polls[i].revents) continue;
		--r;
		ps = procs[i];
		ch = procchans[i];
//		DEBUG(DEVEL, "i = %d, ch = %d, cmd_id = %d, events = %X, revents = %X", i, ch, ps->cmd_id, polls[i].events, polls[i].revents);
		if (ps->exec_flags & PFLAG_SEXEC_NEWPTY) {
			if (polls[i].revents & POLLIN) {
				int maxrd = ed_buf_remains(ps->bufs + EDEXEC_CHAN_STDOUT);
				if (maxrd == -1 || maxrd > sizeof(buf)) maxrd = sizeof(buf);
				rd = read(polls[i].fd, buf, maxrd);
//				DEBUG(DEVEL, "read(%d) = %d", ps->cmd_id, rd);
/*				{
					FILE*f = fopen("/data.txt", "ab");
					if (f) { fwrite(buf, 1, rd, f); fclose(f); }
				}*/
				if (rd > 0) ed_buf_put(ps->bufs + EDEXEC_CHAN_STDOUT, buf, rd);
			}
			if (polls[i].revents & POLLOUT) {
				void*data;
				int nw;
				rd = ed_buf_preview(ps->bufs + EDEXEC_CHAN_STDIN, &data);
//				DEBUG(DEVEL, "preview(%d) = %d", ps->cmd_id, rd);
				if (rd > 0) {
					nw = write(polls[i].fd, data, rd);
//					DEBUG(DEVEL, "write(%d) = %d", rd, nw);
					if (nw > 0) ed_buf_commit(ps->bufs + EDEXEC_CHAN_STDIN, nw);
				}
			}
			if (polls[i].revents & (POLLHUP | POLLERR)) {
				DEBUG(DEVEL, "closed PTY handle");
				close(ps->pipes[EDEXEC_CHAN_PTY]);
				ps->pipes[EDEXEC_CHAN_PTY] = -1;
			}
		} else {
			if (polls[i].revents & POLLIN) {
				int maxrd = ed_buf_remains(ps->bufs + ch);
				if (maxrd == -1 || maxrd > sizeof(buf)) maxrd = sizeof(buf);
				rd = read(polls[i].fd, buf, maxrd);
//				DEBUG(DEVEL, "read(%d) = %d", ps->cmd_id, rd);
				if (rd > 0) ed_buf_put(ps->bufs + ch, buf, rd);
			}
			if (polls[i].revents & POLLOUT) {
				void*data;
				int nw;
				rd = ed_buf_preview(ps->bufs + ch, &data);
//				DEBUG(DEVEL, "preview(%d) = %d", ps->cmd_id, rd);
				if (rd > 0) {
					nw = write(polls[i].fd, data, rd);
//					DEBUG(DEVEL, "write(%d) = %d", rd, nw);
					if (nw > 0) ed_buf_commit(ps->bufs + ch, nw);
				}
			}
			if (polls[i].revents & (POLLHUP | POLLERR)) {
				DEBUG(DEVEL, "closed channel %d", ch);
				close(ps->pipes[ch]);
				ps->pipes[ch] = -1;
			}
		}
	}
	return 0;
}

#ifdef EDEXEC_TEST


int next_id = 1;

int detached_test(struct ED_EXEC_POOL*ep)
{
	char rbuf[1024];
	struct PANTHER_SEXEC_REQUEST*req = (struct PANTHER_SEXEC_REQUEST*)rbuf;
	int len;
	req->mode = PFLAG_SEXEC_MODE_EXEC;
	req->cmd_id = next_id++;
	req->exec_flags = PFLAG_SEXEC_DETACH;
	DEBUG(DEVEL, "detached test");
	len = sizeof(*req) + sprintf(req->data, "find /usr/include");
	return ed_exec_put(ep, req, len);
}

int stdout_test(struct ED_EXEC_POOL*ep)
{
	char rbuf[1024];
	struct PANTHER_SEXEC_REQUEST*req = (struct PANTHER_SEXEC_REQUEST*)rbuf;
	int len;
	req->mode = PFLAG_SEXEC_MODE_EXEC;
	req->cmd_id = next_id++;
	req->exec_flags = PFLAG_SEXEC_STDOUT | PFLAG_SEXEC_GETRES;
	DEBUG(DEVEL, "stdout test");
	len = sizeof(*req) + sprintf(req->data, "ls /bin");
	return ed_exec_put(ep, req, len);
}

int stderr_test(struct ED_EXEC_POOL*ep)
{
	char rbuf[1024];
	struct PANTHER_SEXEC_REQUEST*req = (struct PANTHER_SEXEC_REQUEST*)rbuf;
	int len;
	req->mode = PFLAG_SEXEC_MODE_EXEC;
	req->cmd_id = next_id++;
	req->exec_flags = PFLAG_SEXEC_STDERR | PFLAG_SEXEC_GETRES;
	DEBUG(DEVEL, "stderr test");
	len = sizeof(*req) + sprintf(req->data, "xxxls /bin");
	return ed_exec_put(ep, req, len);
}


int stdin_test(struct ED_EXEC_POOL*ep)
{
	char rbuf[1024];
	struct PANTHER_SEXEC_REQUEST*req = (struct PANTHER_SEXEC_REQUEST*)rbuf;
	int len;
	req->mode = PFLAG_SEXEC_MODE_EXEC;
	req->cmd_id = 11;
	req->exec_flags = PFLAG_SEXEC_STDIN | PFLAG_SEXEC_STDOUT | PFLAG_SEXEC_STDERR | PFLAG_SEXEC_GETRES;
	DEBUG(DEVEL, "stdin test");
	len = sizeof(*req) + sprintf(req->data, "tr a b");
	ed_exec_put(ep, req, len);

	req->mode = PFLAG_SEXEC_MODE_STDIN;
	req->cmd_id = 11;
	req->exec_flags = 0;
	len = sizeof(*req) + sprintf(req->data, "data x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\ndata x\n");
	return ed_exec_put(ep, req, len);
}


int close_test(struct ED_EXEC_POOL*ep)
{
	char rbuf[1024];
	struct PANTHER_SEXEC_REQUEST*req = (struct PANTHER_SEXEC_REQUEST*)rbuf;
	int len;
	req->mode = PFLAG_SEXEC_MODE_STDIN;
	req->cmd_id = 11;
	req->exec_flags = PFLAG_SEXEC_DETACH;
	DEBUG(DEVEL, "close test");
	len = sizeof(*req);
	return ed_exec_put(ep, req, len);
}

int term_test(struct ED_EXEC_POOL*ep)
{
	char rbuf[1024];
	struct PANTHER_SEXEC_REQUEST*req = (struct PANTHER_SEXEC_REQUEST*)rbuf;
	int len;
	req->mode = PFLAG_SEXEC_MODE_TERM;
	req->cmd_id = 11;
	req->exec_flags = 0;
	DEBUG(DEVEL, "term test");
	len = sizeof(*req);
	return ed_exec_put(ep, req, len);
}

int pty_test(struct ED_EXEC_POOL*ep)
{
	char rbuf[1024];
	struct PANTHER_SEXEC_REQUEST*req = (struct PANTHER_SEXEC_REQUEST*)rbuf;
	int len;
	req->mode = PFLAG_SEXEC_MODE_EXEC;
	req->cmd_id = next_id++;
	req->exec_flags = PFLAG_SEXEC_NEWPTY | PFLAG_SEXEC_GETRES;
	DEBUG(DEVEL, "pty test");
	len = sizeof(*req) + sprintf(req->data, "ls /bin");
	return ed_exec_put(ep, req, len);
}

int main(int argc, const char*argv[])
{
	struct ED_EXEC_POOL ep;
	char rbuf[256];
	struct PANTHER_SEXEC_RESULT*res = (struct PANTHER_SEXEC_RESULT*)rbuf;
	
	pto_init(NULL, PTO_LEVEL_ALL);
	ed_exec_init(&ep);
	
	for (;;) {
		int r;
		r = ed_exec_get(&ep, res, sizeof(rbuf));
		if (r) {
			DEBUG_DATA(DEVEL, res->data, r - sizeof(*res), "got result(%d): cmd_id = %d, stream_id = %d ", r, res->cmd_id, res->stream_id);
		} else if (r < 0) abort();
		ed_exec_update(&ep, 100);
		r = rand() % 10;
		switch (r) {
		case 0: detached_test(&ep); break;
//		case 1: stdout_test(&ep); break;
//		case 2: pty_test(&ep); break;
//		case 3: stderr_test(&ep); break;
//		case 4: stdin_test(&ep); break;
//		case 5: close_test(&ep); break;
//		case 6: term_test(&ep); break;
		}
	}
	ed_exec_term(&ep);
}

#endif // EDEXEC_TEST
