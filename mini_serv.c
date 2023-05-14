#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <string.h>

typedef struct s_client {
    int id;
    char msg[1024];
}   t_clients;

fd_set master, read_fds, write_fds;

void fatal(char *str)
{
    write(2, str, strlen(str));
    exit(1);
}

void sendAll(int n, int fdMax, char *str)
{
    for (int i = 0; i <= fdMax; i++) {
        if (FD_ISSET(i, &write_fds) && (i != n))
            send(i, str, strlen(str), 0);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
        fatal("wrong number of arguments\n");
    t_clients   clients[1024];
    int sockfd, client_fd, nbytes;
    socklen_t len;
    struct sockaddr_in servaddr, cli;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        fatal("Fatal error\n");
    
    bzero(&servaddr, sizeof(servaddr));
    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

    // assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1]));

    // Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
        fatal("Fatal error\n"); 

	if (listen(sockfd, 10) != 0)
        fatal("Fatal error\n");

    FD_SET(sockfd, &master);
    int fdmax = sockfd;
    int index = 0;
    char buff_read[12042];
    char buff_write[12042];

    for (;;) {
        read_fds = write_fds = master;
        if (select(fdmax + 1, &read_fds, &write_fds, NULL, NULL) == -1)
            continue ;

        // run through the existing connections looking for data to read
        for(int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == sockfd) {
	                client_fd = accept(sockfd, (struct sockaddr *)&cli, &len);
	                if (client_fd < 0)
                        continue ;
                    fdmax = client_fd > fdmax ? client_fd : fdmax;
                    clients[client_fd].id = index++;
                    FD_SET(client_fd, &master);
                    sprintf(buff_write, "server: cient %d just arrived\n", clients[client_fd].id);
                    sendAll(client_fd, fdmax, buff_write);
                    break;
                } else {
                    if ((nbytes = recv(i, buff_read, 65536, 0)) <= 0) {
                        sprintf(buff_write, "server: client %d just left\n", clients[i].id);
                        sendAll(i, fdmax, buff_write);
                        FD_CLR(i, &master);
                        close(i);
                        break ;
                    } else {
                        for (int j = 0, k = strlen(clients[i].msg); j < nbytes; j++, k++) {
                            clients[i].msg[k] = buff_read[j];
                            if (clients[i].msg[k] == '\n') {
                                clients[i].msg[k] = '\0';
                                sprintf(buff_write, "client %d: %s\n", clients[i].id, clients[i].msg);
                                sendAll(i, fdmax, buff_write);
                                bzero(&clients[i].msg, strlen(clients[i].msg));
                                k = -1;
                            }
                        }
                        break ;
                    }
                }
            }
        }
    }
    return 0;
}