#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

typedef struct		s_client 
{
	int fd;
    	int id;
	struct s_client	*next;
}	t_client;

t_client *clients = NULL;

int sockfd, g_id = 0;
fd_set master, read_fd, write_fd;
char str[42*4096], buff[42*4096 + 42];

void	fatal() 
{
	write(2, "Fatal error\n", strlen("Fatal error\n"));
	close(sockfd);
	exit(1);
}

int get_id(int fd)
{
	for (t_client *tmp = clients; tmp; tmp = tmp->next) {
		if (tmp->fd == fd)
			return (tmp->id);
	}
	return (-1);
}

int get_max_fd() 
{
	int	max = sockfd;
	
	for (t_client *tmp = clients; tmp; tmp = tmp->next) {
		max = tmp->fd > max ? tmp->fd : max;
	}
	return (max);
}

void send_all(int not)
{
	for (t_client *tmp = clients; tmp; tmp = tmp->next) {
		if (tmp->fd != not && FD_ISSET(tmp->fd, &write_fd))
			send(tmp->fd, buff, strlen(buff), 0);
	}
}	

void add_cli()
{
	struct sockaddr_in cli;
	socklen_t len = sizeof(cli);
	int connfd;
	t_client *new;

	if ((connfd = accept(sockfd, (struct sockaddr *)&cli, &len)) < 0)
		fatal();
	if (!(new = malloc(sizeof(t_client))))
		fatal();
	FD_SET(connfd, &master);
	new->id = g_id++;
	new->fd = connfd;
	new->next = NULL;
	bzero(buff, sizeof(buff));
	sprintf(buff, "server: client %d just arrived\n", new->id);
	send_all(connfd);
	if (!clients)
		clients = new;
	else {
		t_client *tmp = clients;
		while (tmp->next)
			tmp = tmp->next;
		tmp->next = new;
	}
}

void rm_cli(int fd)
{
	t_client *to_del;

	bzero(buff, sizeof(buff));
	sprintf(buff, "server: client %d just left\n", get_id(fd));
	send_all(fd);
	if (clients && clients->fd == fd) {
		to_del = clients;
		clients = clients->next;
	} else {
		t_client *tmp = clients;

		while(tmp && tmp->next && tmp->next->fd != fd)
			tmp = tmp->next;
		to_del = tmp->next;
		tmp->next = tmp->next->next;
	}
	if (to_del)
		free(to_del);
	FD_CLR(fd, &master);
	close(fd);
}

void extract_message(int fd)
{
	char tmp[4096 * 42];
	int i = 0, j = 0;

	while (str[i])
	{
		tmp[j] = str[i];
		j++;
		if (str[i] == '\n')
		{
			bzero(buff, sizeof(buff));
			sprintf(buff, "client %d: %s", get_id(fd), tmp);
			send_all(fd);
			j = 0;
			bzero(&tmp, strlen(tmp));
		}
		i++;
	}
	bzero(&str, strlen(str));
}

int main(int ac, char **argv)
{
	if (ac != 2)
	{
		write(2, "Wrong number of arguments\n", strlen("Wrong number of arguments\n"));
		exit(1);
	}

	struct sockaddr_in servaddr;

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1]));

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		fatal();
	if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
		fatal();
	if (listen(sockfd, 0) < 0)
		fatal();

	FD_ZERO(&master);
	FD_SET(sockfd, &master);
	
	for (;;) {
		read_fd = write_fd = master;
		if (select(get_max_fd() + 1, &read_fd, &write_fd, NULL, NULL) < 0)
			continue;

		for (int fd = 0; fd <= get_max_fd(); fd++) {
			if (FD_ISSET(fd, &read_fd)) {
				if (fd == sockfd) {
					add_cli();
					break;
				} else {
					int ret_recv = 1;
					while (ret_recv == 1 && str[strlen(str) - 1] != '\n') {
						ret_recv = recv(fd, str + strlen(str), 1, 0);
						if (ret_recv <= 0)
							break ;
					}
					if (ret_recv <= 0) {
						rm_cli(fd);
						break;
					}
					extract_message(fd);
				}
			}

		}

	}
	return (0);
}
