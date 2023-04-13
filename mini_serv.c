#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>

void fatal_error()
{
    write(2, "Fatal error\n", 13);
    exit(1);
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        write(2, "Wrong number of arguments\n", 27);
        exit(1);
    }
    fd_set master, read_fds;
    int sockfd, port, client_fd, nbytes;
    unsigned int len;
    struct sockaddr_in servaddr, cli;
    char buffer[1024];

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        write(1, "socket creation failed...\n", 27);
        exit(0);
    }
    write(1, "Socket successfully created..\n", 31);
    bzero(&servaddr, sizeof(servaddr));

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

    port = atoi(argv[1]);
    if (port == 0)
        fatal_error();
    // assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(port);

    // Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
		printf("socket bind failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully binded..\n");
	if (listen(sockfd, 10) != 0) {
		printf("cannot listen\n"); 
		exit(0); 
	}
    FD_SET(sockfd, &master);
    int fdmax = sockfd;

    for (;;) {
        read_fds = master;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            fatal_error();
        }

        // run through the existing connections looking for data to read
        for(int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == sockfd) {
                    len = sizeof(cli);
	                client_fd = accept(sockfd, (struct sockaddr *)&cli, &len);
	                if (client_fd < 0) { 
                        printf("server acccept failed...\n"); 
                        exit(0); 
                    } 
                    else {
                        printf("server acccept the client...\n");
                        FD_SET(client_fd, &master);
                        if (client_fd > fdmax)
                            fdmax = client_fd;
                    }
                } else {
                    if ((nbytes = recv(i, buffer, sizeof buffer, 0)) <= 0) {
                        close(i);
                        FD_CLR(i, &master);
                    } else {
                        for (int j = 0; j <= fdmax; j++) {
                            if (FD_ISSET(j, &master)) {
                                // except the server and ourself
                                if (j != sockfd && j != i) {
                                    printf("sending data to client %d\n", j);
                                    if (send(j, buffer, nbytes, 0) == -1) {
                                        fatal_error();
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}