#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

fd_set read_fds, write_fds, all_fds;
int cc = 0, maxfd = 0;
int ids[65536];
char *msgs[65536];
char read_buffer[1001];
char intro_buffer[42];

void ft_error_msg(const char *str)
{
	if (str)
		write(2, str, strlen(str));
	else
		write(2, "Fatal error", 11);
	write(2, "\n", 1);
	exit(1);
}

void ft_free_all()
{
	for (int fd = 0; fd <= maxfd; fd++)
	{
		if (FD_ISSET(fd, &all_fds))
		{
			free(msgs[fd]);
			close(fd);
		}
	}
	ft_error_msg(NULL);
}

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				ft_free_all();
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		ft_free_all();
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

void ft_broadcast(int exc, char *msg)
{
	for (int fd = 0; fd <= maxfd; fd++)
	{
		if (FD_ISSET(fd, &write_fds) && exc != fd)
			send(fd, msg, strlen(msg), 0);
	}
}

void ft_send_all(int fd)
{
	char *msg;

	while (extract_message(&(msgs[fd]), &msg))
	{
		bzero(intro_buffer, 42);
		sprintf(intro_buffer, "client %d: ", ids[fd]);
		ft_broadcast(fd, intro_buffer);
		ft_broadcast(fd, msg);
		free(msg);
	}
}

void ft_add_client(int fd)
{
	int connfd;
	struct sockaddr_in cli;
	socklen_t len = sizeof(cli);
	connfd = accept(fd, (struct sockaddr *)&cli, &len);
	if (connfd >= 0)
	{
		if (connfd > maxfd)
			maxfd = connfd;
		ids[connfd] = cc++;
		msgs[connfd] = NULL;
		FD_SET(connfd, &all_fds);
		bzero(intro_buffer, 42);
		sprintf(intro_buffer, "server: client %d just arrived\n", ids[connfd]);
		ft_broadcast(connfd, intro_buffer);
	}
}

void ft_recv_msg(int fd)
{
	bzero(read_buffer, 1001);
	int bytes = recv(fd, read_buffer, sizeof(read_buffer) - 1, 0);
	if (bytes <= 0)
	{
		bzero(intro_buffer, 42);
		sprintf(intro_buffer, "server: client %d just left\n", ids[fd]);
		ft_broadcast(fd, intro_buffer);
		free(msgs[fd]);
		FD_CLR(fd, &all_fds);
		close(fd);
	}
	else
	{
		read_buffer[bytes] = '\0';
		msgs[fd] = str_join(msgs[fd], read_buffer);
		ft_send_all(fd);
	}
}

int main(int argc, char **argv) {
	int sockfd;
	struct sockaddr_in servaddr; 

	if (argc != 2)
	{
		ft_error_msg("Wrong number of arguments\n");
	}
	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1)
		ft_error_msg(NULL);


	maxfd = sockfd;
	FD_SET(sockfd, &all_fds);
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1])); 

	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		ft_error_msg(NULL);

	if (listen(sockfd, 10) != 0)
		ft_error_msg(NULL);


	while (1)
	{
		read_fds = write_fds = all_fds;

		if (select(maxfd + 1, &read_fds, &write_fds, 0, 0) < 0)
			continue;

		for (int fd = 0; fd <= maxfd; fd++)
		{
			if (!FD_ISSET(fd, &read_fds))
				continue;

			if (sockfd == fd)
			{
				ft_add_client(sockfd);
				//break?
			}
			else
			{
				ft_recv_msg(fd);
			}
		}
	}
}
