#include <string.h>
#include <stdlib.h>

#include <rtthread.h>
#include <dfs_posix.h>
#include <lwip/sockets.h>
#include <lwip/netif.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#define FTP_PORT 21
#define FTP_SRV_ROOT "/"
#define FTP_MAX_CONNECTION 2
#define FTP_USER "rtt"
#define FTP_PASSWORD "demo"
#define FTP_WELCOME_MSG "220-= welcome on RT-Thread FTP server =-\r\n220 \r\n"
#define FTP_BUFFER_SIZE 1024

#define debug(fmt, ...) rt_kprintf("[%s:%s:%d] " fmt, __FILE__, __func__, __LINE__, ## __VA_ARGS__)

typedef struct {
	uint8_t ip1;
	uint8_t ip2;
	uint8_t ip3;
	uint8_t ip4;
} ip_addr_field_t;

typedef union {
	ip_addr_field_t s;
	uint32_t v;
} u_ip_addr_field_t;

typedef struct {
	uint32_t poll_in : 1;
	uint32_t poll_out : 1;
	uint32_t poll_err : 1;
} poll_mask_t;

typedef union {
	poll_mask_t s;
	uint32_t v;
} u_poll_mask_t;

typedef int (*session_data_callback_t)(void *ctx);

struct ftp_session {
	rt_bool_t is_anonymous;

	int sockfd;
	struct sockaddr_in remote;

	unsigned short pasv_port;

	int pasv_server_sockfd;
	rt_tick_t pasv_server_stamp;//rt_tick_get

	int data_sockfd;
	struct sockaddr_in data_remote;
	u_poll_mask_t data_poll_mask;
	rt_tick_t data_stamp;//rt_tick_get
	session_data_callback_t session_data_callback;

	int filefd;
	size_t offset;

	/* current directory */
	char currentdir[256];

	struct ftp_session *next;
};

static struct ftp_session *session_list = NULL;
unsigned short pasv_port = 10000;

static int ftp_process_request(struct ftp_session *session, char *buf);
static int ftp_get_filesize(char *filename);

static struct ftp_session *ftp_new_session()
{
	struct ftp_session *session;

	session = (struct ftp_session *)rt_malloc(sizeof(struct ftp_session));

	if(session == NULL) {
		return session;
	}

	session->is_anonymous = RT_TRUE;
	session->sockfd = -1;

	session->pasv_server_sockfd = -1;
	session->pasv_server_stamp = 0;

	session->data_sockfd = -1;
	session->session_data_callback = NULL;

	session->filefd = -1;
	session->offset = 0;

	session->next = session_list;
	session_list = session;

	debug("new session:%p\n", session);

	return session;
}

static void ftp_close_session(struct ftp_session *session)
{
	struct ftp_session *list;

	if(session == NULL) {
		return;
	}

	if(session->sockfd != -1) {
		closesocket(session->sockfd);
	}

	if(session->pasv_server_sockfd != -1) {
		closesocket(session->pasv_server_sockfd);
	}

	if(session->data_sockfd != -1) {
		closesocket(session->data_sockfd);
	}

	if(session->filefd != -1) {
		close(session->filefd);
	}

	if (session_list == session) {
		session_list = session_list->next;
		session->next = NULL;
	} else {
		list = session_list;

		while (list->next != session) {
			list = list->next;
		}

		list->next = session->next;
		session->next = NULL;
	}

	debug("free session:%p\n", session);
	rt_free(session);
}

static int ftp_get_filesize(char *filename)
{
	int pos;
	int end;
	int fd;

	fd = open(filename, O_RDONLY, 0);

	if (fd < 0) {
		return -1;
	}

	pos = lseek(fd, 0, SEEK_CUR);
	end = lseek(fd, 0, SEEK_END);
	lseek(fd, pos, SEEK_SET);
	close(fd);

	return end;
}

static rt_bool_t is_absolute_path(char *path)
{
#ifdef _WIN32

	if (path[0] == '\\' ||
	    (path[1] == ':' && path[2] == '\\')) {
		return RT_TRUE;
	}

#else

	if (path[0] == '/') {
		return RT_TRUE;
	}

#endif

	return RT_FALSE;
}

static int build_full_path(struct ftp_session *session, char *path, char *new_path, size_t size)
{
	if (is_absolute_path(path) == RT_TRUE) {
		strcpy(new_path, path);
	} else {
		rt_sprintf(new_path, "%s/%s", session->currentdir, path);
	}

	return 0;
}

static void ftpd_thread_entry(void *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int sockfd, maxfd;
	struct sockaddr_in local;
	fd_set rfds;
	fd_set wfds;
	fd_set efds;
	struct ftp_session *session;
	rt_uint32_t addr_len = sizeof(struct sockaddr);

	local.sin_port = htons(FTP_PORT);
	local.sin_family = PF_INET;
	local.sin_addr.s_addr = INADDR_ANY;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if(sockfd < 0) {
		rt_kprintf("create socket failed\n");
		return;
	}

	bind(sockfd, (struct sockaddr *)&local, addr_len);
	listen(sockfd, FTP_MAX_CONNECTION);

	for(;;) {
		struct timeval tv;
		int ret;

		tv.tv_sec = 0;
		tv.tv_usec = 100 * 1000;

		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_ZERO(&efds);

		FD_SET(sockfd, &rfds);

		/* get maximum fd */
		maxfd = sockfd;

		session = session_list;

		while (session != RT_NULL) {
			if (maxfd < session->sockfd) {
				maxfd = session->sockfd;
			}

			FD_SET(session->sockfd, &rfds);

			if(session->pasv_server_sockfd != -1) {
				if (maxfd < session->pasv_server_sockfd) {
					maxfd = session->pasv_server_sockfd;
				}

				FD_SET(session->pasv_server_sockfd, &rfds);
			}

			if(session->data_sockfd != -1) {
				if (maxfd < session->data_sockfd) {
					maxfd = session->data_sockfd;
				}

				if(session->data_poll_mask.s.poll_in == 1) {
					FD_SET(session->data_sockfd, &rfds);
				}

				if(session->data_poll_mask.s.poll_out == 1) {
					FD_SET(session->data_sockfd, &wfds);
				}

				if(session->data_poll_mask.s.poll_err == 1) {
					FD_SET(session->data_sockfd, &efds);
				}
			}

			session = session->next;
		}

		ret = select(maxfd + 1, &rfds, &wfds, &efds, &tv);

		if(ret == 0) {
			continue;
		} else if(ret < 0) {
			debug("ret:%d\n", ret);
			continue;
		}

		if(FD_ISSET(sockfd, &rfds)) {
			int session_sockfd;
			struct sockaddr_in session_remote;

			debug("process session accept\n");
			session_sockfd = accept(sockfd, (struct sockaddr *)&session_remote, &addr_len);

			if(session_sockfd == -1) {
				rt_kprintf("Error on accept()\nContinuing...\n");
				continue;
			} else {
				rt_kprintf("Got connection from %s\n", inet_ntoa(session_remote.sin_addr));
				send(session_sockfd, FTP_WELCOME_MSG, strlen(FTP_WELCOME_MSG), 0);

				/* new session */
				session = ftp_new_session();

				if (session != NULL) {
					strcpy(session->currentdir, FTP_SRV_ROOT);
					session->sockfd = session_sockfd;
					session->remote = session_remote;
				}
			}
		} else {
			struct ftp_session *next;
			session = session_list;

			while (session != NULL) {
				next = session->next;

				if(FD_ISSET(session->sockfd, &rfds)) {
					int numbytes;
					numbytes = recv(session->sockfd, buffer, FTP_BUFFER_SIZE, 0);

					if(numbytes == 0 || numbytes == -1) {
						debug("Client %s disconnected\n", inet_ntoa(session->remote.sin_addr));
						//FD_CLR(session->sockfd, &rfds);
						ftp_close_session(session);
					} else {
						//debug("process session cmd\n");
						buffer[numbytes] = 0;

						if(ftp_process_request(session, buffer) == -1) {
							debug("Client %s disconnected\r\n", inet_ntoa(session->remote.sin_addr));
							ftp_close_session(session);
						}
					}
				}

				if(session->pasv_server_sockfd != -1) {
					if(FD_ISSET(session->pasv_server_sockfd, &rfds)) {
						if(session->data_sockfd == -1) {

							session->data_sockfd = accept(session->pasv_server_sockfd, (struct sockaddr *)&session->data_remote, &addr_len);

							if(session->data_sockfd == -1) {
								rt_sprintf(buffer, "425 Can't open data connection.\r\n");
								send(session->sockfd, buffer, strlen(buffer), 0);
								debug("Error on pasv accept()\nContinuing...\n");
								continue;
							} else {
								debug("Got pasv connection from %s\n", inet_ntoa(session->data_remote.sin_addr));
								session->data_stamp = rt_tick_get();

								closesocket(session->pasv_server_sockfd);
								session->pasv_server_sockfd = -1;
								debug("Leave pasv mode...\n");
							}
						} else {
							rt_sprintf(buffer, "425 Can't open data connection.\r\n");
							send(session->sockfd, buffer, strlen(buffer), 0);
							debug("session->data_sockfd exist!\n");
							debug("Error on pasv accept()\nContinuing...\n");
							continue;
						}
					} else {
						if(rt_tick_get() - session->pasv_server_stamp >= rt_tick_from_millisecond(3000)) {
							closesocket(session->pasv_server_sockfd);
							session->pasv_server_sockfd = -1;
							rt_sprintf(buffer, "425 Can't open data connection.\r\n");
							send(session->sockfd, buffer, strlen(buffer), 0);
							debug("Leave pasv mode...\n");
						}
					}
				}

				if(session->data_sockfd != -1) {
					if(session->data_poll_mask.s.poll_in == 1) {
						if(FD_ISSET(session->data_sockfd, &rfds)) {
							if(session->session_data_callback != NULL) {
								//debug("process data_sockfd\n");
								session->session_data_callback(session);
								session->data_stamp = rt_tick_get();
							}
						}
					}

					if(session->data_poll_mask.s.poll_out == 1) {
						if(FD_ISSET(session->data_sockfd, &wfds)) {
							if(session->session_data_callback != NULL) {
								//debug("process data_sockfd\n");
								session->session_data_callback(session);
								session->data_stamp = rt_tick_get();
							}
						}
					}

					if(session->data_poll_mask.s.poll_err == 1) {
						if(FD_ISSET(session->data_sockfd, &efds)) {
							debug("\n");
						}
					}

					if(rt_tick_get() - session->data_stamp >= rt_tick_from_millisecond(3000)) {
						if(session->filefd != -1) {
							close(session->filefd);
							session->filefd = -1;
						}

						closesocket(session->data_sockfd);
						session->data_sockfd = -1;
						session->session_data_callback = NULL;
						debug("data_sockfd timeout! close...\n");
					}
				}

				session = next;
			}
		}
	}

	rt_free(buffer);
}

static int str_begin_with(char *src, char *match)
{
	while (*match) {
		/* check source */
		if (*src == 0) {
			return -1;
		}

		if (*match != *src) {
			return -1;
		}

		match ++;
		src ++;
	}

	return 0;
}


static int list_callback(void *ctx)
{
	struct ftp_session *session = (struct ftp_session *)ctx;
	DIR *dirp;
	struct dirent *entry;
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

#ifdef _WIN32
	struct _stat s;
#else
	struct stat s;
#endif

	if(buffer == NULL) {
		debug("no memory\n");

		rt_sprintf(buffer, "500 Internal Error\r\n");
		send(session->sockfd, buffer, strlen(buffer), 0);

		ftp_close_session(session);

		return ret;
	}

	dirp = opendir(session->currentdir);

	if (dirp == NULL) {
		debug("opendir %s\n", strerror(errno));

		rt_sprintf(buffer, "500 Internal Error\r\n");
		send(session->sockfd, buffer, strlen(buffer), 0);

		ftp_close_session(session);

		goto exit;
	}

	while (1) {
		entry = readdir(dirp);

		if (entry == NULL) {
			break;
		}

		rt_sprintf(buffer, "%s/%s", session->currentdir, entry->d_name);
#ifdef _WIN32

		if (_stat(buffer, &s) == 0)
#else
		if (stat(buffer, &s) == 0)
#endif
		{
			int length = 0;

			if (s.st_mode & S_IFDIR) {
				length = rt_sprintf(buffer, "drw-r--r-- 1 admin admin %d Jan 1 2000 %s\r\n", 0, entry->d_name);
			} else {
				length = rt_sprintf(buffer, "-rw-r--r-- 1 admin admin %d Jan 1 2000 %s\r\n", s.st_size, entry->d_name);
			}

			send(session->data_sockfd, buffer, length, 0);
		} else {
			rt_kprintf("Get directory entry error\n");
			break;
		}
	}

	closedir(dirp);

	ret = 0;

	closesocket(session->data_sockfd);
	session->data_sockfd = -1;

	rt_sprintf(buffer, "226 Transfert Complete.\r\n");
	send(session->sockfd, buffer, strlen(buffer), 0);

exit:
	rt_free(buffer);

	return ret;
}

static int simple_list_callback(void *ctx)
{
	struct ftp_session *session = (struct ftp_session *)ctx;
	DIR *dirp;
	struct dirent *entry;
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		rt_sprintf(buffer, "500 Internal Error\r\n");
		send(session->sockfd, buffer, strlen(buffer), 0);

		ftp_close_session(session);

		return ret;
	}

	dirp = opendir(session->currentdir);

	if (dirp == NULL) {
		debug("opendir %s\n", strerror(errno));

		rt_sprintf(buffer, "500 Internal Error\r\n");
		send(session->sockfd, buffer, strlen(buffer), 0);

		ftp_close_session(session);

		goto exit;
	}

	while (1) {
		entry = readdir(dirp);

		if (entry == NULL) {
			break;
		}

		rt_sprintf(buffer, "%s\r\n", entry->d_name);
		send(session->data_sockfd, buffer, strlen(buffer), 0);
	}

	closedir(dirp);

	closesocket(session->data_sockfd);
	session->data_sockfd = -1;

	rt_sprintf(buffer, "226 Transfert Complete.\r\n");
	send(session->sockfd, buffer, strlen(buffer), 0);

exit:

	rt_free(buffer);
	return 0;
}

static int retr_callback(void *ctx)
{
	struct ftp_session *session = (struct ftp_session *)ctx;
	int  numbytes;
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		rt_sprintf(buffer, "500 Internal Error\r\n");
		send(session->sockfd, buffer, strlen(buffer), 0);

		ftp_close_session(session);
		return ret;
	}

	numbytes = read(session->filefd, buffer, FTP_BUFFER_SIZE);

	if(numbytes > 0) {
		ret = send(session->data_sockfd, buffer, numbytes, 0);

		if(ret <= 0) {
			debug("abort retr!\n");

			close(session->filefd);
			session->filefd = -1;
			closesocket(session->data_sockfd);
			session->data_sockfd = -1;
		} else {
			ret = 0;
		}
	} else if(numbytes == 0) {
		debug("\n");
		rt_sprintf(buffer, "226 Finished.\r\n");
		send(session->sockfd, buffer, strlen(buffer), 0);
		ret = 0;

		close(session->filefd);
		session->filefd = -1;
		closesocket(session->data_sockfd);
		session->data_sockfd = -1;
	} else if(numbytes == -1) {
		debug("abort retr\n");

		close(session->filefd);
		session->filefd = -1;
		closesocket(session->data_sockfd);
		session->data_sockfd = -1;
	}

	rt_free(buffer);

	return ret;
}

static int stor_callback(void *ctx)
{
	struct ftp_session *session = (struct ftp_session *)ctx;
	int  numbytes;
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		rt_sprintf(buffer, "500 Internal Error\r\n");
		send(session->sockfd, buffer, strlen(buffer), 0);

		ftp_close_session(session);
		return ret;
	}

	if((numbytes = recv(session->data_sockfd, buffer, FTP_BUFFER_SIZE, 0)) > 0) {
		ret = write(session->filefd, buffer, numbytes);

		if(ret > 0) {
			ret = 0;
		} else {
			debug("abort stor!\n");

			close(session->filefd);
			session->filefd = -1;
			closesocket(session->data_sockfd);
			session->data_sockfd = -1;
		}
	} else if(numbytes == 0) {
		rt_sprintf(buffer, "226 Finished.\r\n");
		send(session->sockfd, buffer, strlen(buffer), 0);
		ret = 0;

		close(session->filefd);
		session->filefd = -1;
		closesocket(session->data_sockfd);
		session->data_sockfd = -1;
	} else if(numbytes == -1) {
		debug("abort stor!\n");

		close(session->filefd);
		session->filefd = -1;
		closesocket(session->data_sockfd);
		session->data_sockfd = -1;
	}

	rt_free(buffer);

	return ret;
}

static int do_user(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	debug("%s sent login \"%s\"\n", inet_ntoa(session->remote.sin_addr), parameter);

	if(buffer == NULL) {
		return ret;
	}

	// login correct
	if(strcmp(parameter, "anonymous") == 0) {
		session->is_anonymous = RT_TRUE;
		rt_sprintf(buffer, "331 Anonymous login OK send e-mail address for password.\r\n", parameter);
		send(session->sockfd, buffer, strlen(buffer), 0);
		ret = 0;
	} else if (strcmp(parameter, FTP_USER) == 0) {
		session->is_anonymous = RT_FALSE;
		rt_sprintf(buffer, "331 Password required for %s\r\n", parameter);
		send(session->sockfd, buffer, strlen(buffer), 0);
		ret = 0;
	} else {
		// incorrect login
		rt_sprintf(buffer, "530 Login incorrect. Bye.\r\n");
		send(session->sockfd, buffer, strlen(buffer), 0);
	}

	rt_free(buffer);
	return ret;
}

static int do_pass(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	rt_kprintf("%s sent password \"%s\"\n", inet_ntoa(session->remote.sin_addr), parameter);

	if(buffer == NULL) {
		return ret;
	}

	if (strcmp(parameter, FTP_PASSWORD) == 0 || session->is_anonymous == RT_TRUE) {
		// password correct
		rt_sprintf(buffer, "230 User logged in\r\n");
		send(session->sockfd, buffer, strlen(buffer), 0);
		ret = 0;
	} else {
		// incorrect password
		rt_sprintf(buffer, "530 Login or Password incorrect. Bye!\r\n");
		send(session->sockfd, buffer, strlen(buffer), 0);
	}

	rt_free(buffer);

	return ret;
}

static int do_list(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	rt_sprintf(buffer, "150 Opening Binary mode connection for file list.\r\n");
	send(session->sockfd, buffer, strlen(buffer), 0);
	ret = 0;

	debug("start list_callback\n");
	session->data_poll_mask.v = 0;
	session->data_poll_mask.s.poll_out = 1;
	session->session_data_callback = list_callback;

	rt_free(buffer);

	return ret;
}

static int do_nlst(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	rt_sprintf(buffer, "150 Opening Binary mode connection for file list.\r\n");
	send(session->sockfd, buffer, strlen(buffer), 0);
	ret = 0;

	debug("start simple_list_callback\n");
	session->data_poll_mask.v = 0;
	session->data_poll_mask.s.poll_out = 1;
	session->session_data_callback = simple_list_callback;

	rt_free(buffer);

	return ret;
}

static int do_pwd(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	rt_sprintf(buffer, "257 \"%s\" is current directory.\r\n", session->currentdir);
	send(session->sockfd, buffer, strlen(buffer), 0);
	ret = 0;

	rt_free(buffer);

	return ret;
}

static int do_type(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	if(strcmp(parameter, "I") == 0) {
		rt_sprintf(buffer, "200 Type set to binary.\r\n");
		send(session->sockfd, buffer, strlen(buffer), 0);
	} else {
		rt_sprintf(buffer, "200 Type set to ascii.\r\n");
		send(session->sockfd, buffer, strlen(buffer), 0);
	}

	ret = 0;

	rt_free(buffer);

	return ret;
}

static int do_pasv(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int dig1 = 0;
	int dig2 = 0;
	int optval = 1;
	struct sockaddr_in local;
	u_ip_addr_field_t u_ip_addr_field;
	int ret = -1;
	rt_uint32_t addr_len = sizeof(struct sockaddr_in);

	if(buffer == NULL) {
		return ret;
	}

	ret = 0;

	if(session->pasv_server_sockfd != -1) {
		debug("session->pasv_server_sockfd exist!\n");

		rt_sprintf(buffer, "425 Can't open data connection.\r\n");
		send(session->sockfd, buffer, strlen(buffer), 0);
		ret = 0;
		goto exit;
	}

	if(session->data_sockfd != -1) {
		debug("session->data_sockfd exist!\n");

		rt_sprintf(buffer, "425 Can't open data connection.\r\n");
		send(session->sockfd, buffer, strlen(buffer), 0);
		ret = 0;
		goto exit;
	}

	session->pasv_port = pasv_port++;

	local.sin_family = PF_INET;
	local.sin_port = htons(session->pasv_port);
	local.sin_addr.s_addr = INADDR_ANY;

	dig1 = (int)(session->pasv_port / 256);
	dig2 = session->pasv_port % 256;
	u_ip_addr_field.v = netif_default->ip_addr.addr;

	if((session->pasv_server_sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		debug("socket %s\n", strerror(errno));

		rt_sprintf(buffer, "425 Can't open data connection.\r\n");
		send(session->sockfd, buffer, strlen(buffer), 0);
		goto exit;
	}

	if(setsockopt(session->pasv_server_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
		debug("setsockopt %s\n", strerror(errno));

		rt_sprintf(buffer, "425 Can't open data connection.\r\n");
		send(session->sockfd, buffer, strlen(buffer), 0);

		closesocket(session->pasv_server_sockfd);
		session->pasv_server_sockfd = -1;
		goto exit;
	}

	if(bind(session->pasv_server_sockfd, (struct sockaddr *)&local, addr_len) == -1) {
		debug("bind %s\n", strerror(errno));

		rt_sprintf(buffer, "425 Can't open data connection.\r\n");
		send(session->sockfd, buffer, strlen(buffer), 0);

		closesocket(session->pasv_server_sockfd);
		session->pasv_server_sockfd = -1;
		goto exit;
	}

	if(listen(session->pasv_server_sockfd, 1) == -1) {
		debug("listen %s\n", strerror(errno));

		rt_sprintf(buffer, "425 Can't open data connection.\r\n");
		send(session->sockfd, buffer, strlen(buffer), 0);

		closesocket(session->pasv_server_sockfd);
		session->pasv_server_sockfd = -1;
		goto exit;
	}

	session->pasv_server_stamp = rt_tick_get();

	rt_kprintf("Listening %d seconds @ port %d\n", 3, session->pasv_port);

	rt_sprintf(buffer, "227 Entering passive mode (%d,%d,%d,%d,%d,%d)\r\n", u_ip_addr_field.s.ip1, u_ip_addr_field.s.ip2, u_ip_addr_field.s.ip3, u_ip_addr_field.s.ip4, dig1, dig2);
	send(session->sockfd, buffer, strlen(buffer), 0);

exit:

	rt_free(buffer);

	return ret;
}

static int do_retr(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;
	int file_size;

	if(buffer == NULL) {
		return ret;
	}

	build_full_path(session, parameter, buffer, 256);
	strcpy(parameter, buffer);

	file_size = ftp_get_filesize(parameter);

	if (file_size == -1) {
		rt_sprintf(buffer, "550 \"%s\" : not a regular file\r\n", parameter);
		send(session->sockfd, buffer, strlen(buffer), 0);

		goto exit;
	}

	session->filefd = open(parameter, O_RDONLY, 0);

	if (session->filefd < 0) {
		rt_sprintf(buffer, "550 \"%s\" : not a regular file\r\n", parameter);
		send(session->sockfd, buffer, strlen(buffer), 0);

		goto exit;
	}

	if(session->offset > 0 && session->offset < file_size) {
		lseek(session->filefd, session->offset, SEEK_SET);
		rt_sprintf(buffer, "150 Opening binary mode data connection for partial \"%s\" (%d/%d bytes).\r\n", parameter, file_size - session->offset, file_size);
	} else {
		rt_sprintf(buffer, "150 Opening binary mode data connection for \"%s\" (%d bytes).\r\n", parameter, file_size);
	}

	send(session->sockfd, buffer, strlen(buffer), 0);

	ret = 0;

	debug("start retr_callback\n");
	session->data_poll_mask.v = 0;
	session->data_poll_mask.s.poll_out = 1;
	session->session_data_callback = retr_callback;

exit:
	rt_free(buffer);

	return ret;
}

static int do_stor(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	if(session->is_anonymous == RT_TRUE) {
		rt_sprintf(buffer, "550 Permission denied.\r\n");
		send(session->sockfd, buffer, strlen(buffer), 0);

		debug("abort stor!\n");
		goto exit;
	}

	build_full_path(session, parameter, buffer, FTP_BUFFER_SIZE);

	strcpy(parameter, buffer);

	session->filefd = open(parameter, O_WRONLY | O_CREAT | O_TRUNC, 0);

	if(session->filefd < 0) {
		rt_sprintf(buffer, "550 Cannot open \"%s\" for writing.\r\n", parameter);
		send(session->sockfd, buffer, strlen(buffer), 0);

		session->filefd = -1;
		goto exit;
	}

	rt_sprintf(buffer, "150 Opening binary mode data connection for \"%s\".\r\n", parameter);
	send(session->sockfd, buffer, strlen(buffer), 0);

	debug("start stor_callback\n");

	session->data_poll_mask.v = 0;
	session->data_poll_mask.s.poll_in = 1;
	session->session_data_callback = stor_callback;

	ret = 0;

exit:
	rt_free(buffer);

	return ret;
}

static int do_size(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;
	int file_size;

	if(buffer == NULL) {
		return ret;
	}

	build_full_path(session, parameter, buffer, FTP_BUFFER_SIZE);

	strcpy(parameter, buffer);

	file_size = ftp_get_filesize(parameter);

	if( file_size == -1) {
		rt_sprintf(buffer, "550 \"%s\" : not a regular file\r\n", parameter);
		send(session->sockfd, buffer, strlen(buffer), 0);
	} else {
		rt_sprintf(buffer, "213 %d\r\n", file_size);
		send(session->sockfd, buffer, strlen(buffer), 0);
	}

	ret = 0;

	rt_free(buffer);

	return ret;
}

static int do_mdtm(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	rt_sprintf(buffer, "550 \"/\" : not a regular file\r\n");
	send(session->sockfd, buffer, strlen(buffer), 0);
	ret = 0;
	rt_free(buffer);

	return ret;
}

static int do_syst(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	rt_sprintf(buffer, "215 %s\r\n", "RT-Thread RTOS");
	send(session->sockfd, buffer, strlen(buffer), 0);
	ret = 0;
	rt_free(buffer);

	return ret;
}

static int do_cwd(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	build_full_path(session, parameter, buffer, 256);
	strcpy(parameter, buffer);

	rt_sprintf(buffer, "250 Changed to directory \"%s\"\r\n", parameter);
	send(session->sockfd, buffer, strlen(buffer), 0);
	strcpy(session->currentdir, parameter);
	rt_kprintf("Changed to directory %s\n", parameter);
	ret = 0;
	rt_free(buffer);

	return ret;
}

static int do_cdup(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	rt_sprintf(buffer, "%s/%s", session->currentdir, "..");
	strcpy(parameter, buffer);

	rt_sprintf(buffer, "250 Changed to directory \"%s\"\r\n", parameter);
	send(session->sockfd, buffer, strlen(buffer), 0);
	strcpy(session->currentdir, parameter);
	rt_kprintf("Changed to directory %s\n", parameter);

	ret = 0;
	rt_free(buffer);

	return ret;
}

static int do_port(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;
	int i;
	int portcom[6];
	char tmpip[100];
	rt_uint32_t addr_len = sizeof(struct sockaddr_in);

	if(buffer == NULL) {
		return ret;
	}

	if(session->data_sockfd != -1) {
		debug("session->data_sockfd exist!\n");

		rt_sprintf(buffer, "425 Can't open data connection.\r\n");
		send(session->sockfd, buffer, strlen(buffer), 0);
		ret = 0;
		goto exit;
	}

	i = 0;
	portcom[i++] = atoi(strtok(parameter, ".,;()"));

	for(; i < 6; i++) {
		portcom[i] = atoi(strtok(0, ".,;()"));
	}

	rt_sprintf(tmpip, "%d.%d.%d.%d", portcom[0], portcom[1], portcom[2], portcom[3]);

	if((session->data_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		debug("socket %s\n", strerror(errno));
		rt_sprintf(buffer, "425 Can't open data connection.\r\n");
		send(session->sockfd, buffer, strlen(buffer), 0);

		goto exit;
	}

	session->data_remote.sin_addr.s_addr = inet_addr(tmpip);
	session->data_remote.sin_port = htons(portcom[4] * 256 + portcom[5]);
	session->data_remote.sin_family = PF_INET;

	if(connect(session->data_sockfd, (struct sockaddr *)&session->data_remote, addr_len) == -1) {
		// is it only local address?try using gloal ip addr
		session->data_remote.sin_addr = session->remote.sin_addr;

		if(connect(session->data_sockfd, (struct sockaddr *)&session->data_remote, addr_len) == -1) {
			debug("connect %s\n", strerror(errno));
			rt_sprintf(buffer, "425 Can't open data connection.\r\n");
			send(session->sockfd, buffer, strlen(buffer), 0);

			closesocket(session->data_sockfd);
			session->data_sockfd = -1;

			ret = 0;
			goto exit;
		}
	}

	session->pasv_port = portcom[4] * 256 + portcom[5];
	rt_kprintf("Connected to Data(PORT) %s @ %d\n", tmpip, portcom[4] * 256 + portcom[5]);
	rt_sprintf(buffer, "200 Port Command Successful.\r\n");
	send(session->sockfd, buffer, strlen(buffer), 0);

	ret = 0;
exit:
	rt_free(buffer);

	return ret;
}

static int do_rest(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	if(atoi(parameter) >= 0) {
		session->offset = atoi(parameter);
		rt_sprintf(buffer, "350 Send RETR or STOR to start transfert.\r\n");
		send(session->sockfd, buffer, strlen(buffer), 0);
	}

	ret = 0;

	rt_free(buffer);

	return ret;
}

static int do_mkd(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	if (session->is_anonymous == RT_TRUE) {
		rt_sprintf(buffer, "550 Permission denied.\r\n");
		send(session->sockfd, buffer, strlen(buffer), 0);
		ret = 0;

		goto exit;
	}

	build_full_path(session, parameter, buffer, 256);

	strcpy(parameter, buffer);

	if(mkdir(parameter, 0) == -1) {
		rt_sprintf(buffer, "550 File \"%s\" exists.\r\n", parameter);
		send(session->sockfd, buffer, strlen(buffer), 0);
	} else {
		rt_sprintf(buffer, "257 directory \"%s\" successfully created.\r\n", parameter);
		send(session->sockfd, buffer, strlen(buffer), 0);
	}

	ret = 0;

exit:
	rt_free(buffer);

	return ret;
}

static int do_dele(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	if (session->is_anonymous == RT_TRUE) {
		rt_sprintf(buffer, "550 Permission denied.\r\n");
		send(session->sockfd, buffer, strlen(buffer), 0);

		ret = 0;
		goto exit;
	}

	build_full_path(session, parameter, buffer, 256);
	strcpy(parameter, buffer);

	if(unlink(parameter) == 0) {
		rt_sprintf(buffer, "250 Successfully deleted file \"%s\".\r\n", parameter);
	} else {
		rt_sprintf(buffer, "550 Not such file or directory: %s.\r\n", parameter);
	}

	send(session->sockfd, buffer, strlen(buffer), 0);

	ret = 0;

exit:
	rt_free(buffer);

	return ret;
}

static int do_rmd(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	if (session->is_anonymous == RT_TRUE) {
		rt_sprintf(buffer, "550 Permission denied.\r\n");
		send(session->sockfd, buffer, strlen(buffer), 0);
		ret = 0;
		goto exit;
	}

	build_full_path(session, parameter, buffer, 256);
	strcpy(parameter, buffer);

	if(unlink(parameter) == -1) {
		rt_sprintf(buffer, "550 Directory \"%s\" doesn't exist.\r\n", parameter);
		send(session->sockfd, buffer, strlen(buffer), 0);
	} else {
		rt_sprintf(buffer, "257 directory \"%s\" successfully deleted.\r\n", parameter);
		send(session->sockfd, buffer, strlen(buffer), 0);
	}

	ret = 0;

exit:
	rt_free(buffer);

	return ret;
}

static int do_quit(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	rt_sprintf(buffer, "221 Bye!\r\n");
	send(session->sockfd, buffer, strlen(buffer), 0);

	rt_free(buffer);

	return ret;
}

static int do_un_imp(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	rt_sprintf(buffer, "502 Not Implemented.\r\n");
	send(session->sockfd, buffer, strlen(buffer), 0);
	ret = 0;

	rt_free(buffer);

	return ret;
}

static int ftp_process_request(struct ftp_session *session, char *buf)
{
	//int  numbytes;
	char *parameter_ptr, *ptr;
	int ret;

	/* remove \r\n */
	ptr = buf;

	while (*ptr) {
		if (*ptr == '\r' || *ptr == '\n') {
			*ptr = 0;
		}

		ptr ++;
	}

	/* get request parameter */
	parameter_ptr = strchr(buf, ' ');

	if(parameter_ptr != NULL) {
		parameter_ptr++;
	}

	// debug:
	rt_kprintf("%s requested: \"%s\"\n", inet_ntoa(session->remote.sin_addr), buf);

	//
	//-----------------------
	if(str_begin_with(buf, "USER") == 0) {
		ret = do_user(session, parameter_ptr);
	} else if(str_begin_with(buf, "PASS") == 0) {
		ret = do_pass(session, parameter_ptr);
	} else if(str_begin_with(buf, "LIST") == 0  ) {
		ret = do_list(session, parameter_ptr);
	} else if(str_begin_with(buf, "NLST") == 0 ) {
		ret = do_nlst(session, parameter_ptr);
	} else if(str_begin_with(buf, "PWD") == 0 || str_begin_with(buf, "XPWD") == 0) {
		ret = do_pwd(session, parameter_ptr);
	} else if(str_begin_with(buf, "TYPE") == 0) {
		ret = do_type(session, parameter_ptr);
	} else if(str_begin_with(buf, "PASV") == 0) {
		ret = do_pasv(session, parameter_ptr);
	} else if (str_begin_with(buf, "RETR") == 0) {
		ret = do_retr(session, parameter_ptr);
	} else if (str_begin_with(buf, "STOR") == 0) {
		ret = do_stor(session, parameter_ptr);
	} else if(str_begin_with(buf, "SIZE") == 0) {
		ret = do_size(session, parameter_ptr);
	} else if(str_begin_with(buf, "MDTM") == 0) {
		ret = do_mdtm(session, parameter_ptr);
	} else if(str_begin_with(buf, "SYST") == 0) {
		ret = do_syst(session, parameter_ptr);
	} else if(str_begin_with(buf, "CWD") == 0) {
		ret = do_cwd(session, parameter_ptr);
	} else if(str_begin_with(buf, "CDUP") == 0) {
		ret = do_cdup(session, parameter_ptr);
	} else if(str_begin_with(buf, "PORT") == 0) {
		ret = do_port(session, parameter_ptr);
	} else if(str_begin_with(buf, "REST") == 0) {
		ret = do_rest(session, parameter_ptr);
	} else if(str_begin_with(buf, "MKD") == 0) {
		ret = do_mkd(session, parameter_ptr);
	} else if(str_begin_with(buf, "DELE") == 0) {
		ret = do_dele(session, parameter_ptr);
	} else if(str_begin_with(buf, "RMD") == 0) {
		ret = do_rmd(session, parameter_ptr);
	} else if(str_begin_with(buf, "QUIT") == 0) {
		ret = do_quit(session, parameter_ptr);
	} else {
		ret = do_un_imp(session, parameter_ptr);
	}

	return ret;
}

void ftpd_start()
{
	rt_thread_t tid;

	tid = rt_thread_create("ftpd", ftpd_thread_entry, RT_NULL, 4096, 30, 5);

	if (tid != RT_NULL) {
		rt_thread_startup(tid);
	}
}

#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(ftpd_start, start ftp server)
MSH_CMD_EXPORT(ftpd_start, start ftp server);
#endif
