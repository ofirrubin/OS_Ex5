#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h> // Used for threads

#define PORT "3490"  // the port users will be connecting to


void sigchld_handler(int s)
{
    (void)s; // quiet unused variable warning

    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int sock_send(const char *respond, int size, int sockfd)

{	int n = -1;                  //strlen(respond)
	if (size < 0)
		size = strlen(respond);
        if ((n= send(sockfd, respond, size, 0)) == -1)
		perror("send");
	return n;
}

void receive(int sockfd, char **buffer, int size_p1, int *input_size)
{
	if ((*input_size = recv(sockfd, buffer, size_p1-1, 0)) == -1) {
	    perror("recv");
	    return;
	}
	buffer[*input_size] = '\0';
}

// Create server and bind it
int create_server(struct sigaction *sa, char *s[INET_ADDRSTRLEN])
{
    int sockfd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    int reusable=1;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reusable, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }
    return sockfd;
}


void reap_dead_processes(struct sigaction *sa)
{
    sa->sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa->sa_mask);
    sa->sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

}
 
void server_listen(int sockfd, int backlog)
{
    if (listen(sockfd, backlog) == -1) {
        perror("listen");
        exit(1);
    }
}

void handle_forever(int sockfd, char *s[INET_ADDRSTRLEN], void *(* f)(void *))
{
    struct sockaddr_storage their_addr; // connector's address information
    
    // Handle forever
    while(1) {  // main accept() loop
        socklen_t sin_size = sizeof their_addr;
        int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        pthread_t thread_id;
	pthread_create(&thread_id, NULL, *f, (void *)&new_fd);
    }
}

void handle_forever_process(int sockfd, char *s[INET_ADDRSTRLEN], void *(* f)(void *), struct sigaction *mem_handler)
{

    struct sockaddr_storage their_addr; // connector's address information
    // Handle forever
    while(1) {  // main accept() loop
        socklen_t sin_size = sizeof their_addr;
        int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept\n");
            continue;
        }
        int pid = fork();
	if (pid == 0)
	{
		(*f)((void *)&new_fd); // Call handler
	} // Else failed or parent
       	else if (pid < 0)
       	{
       		perror("Error forking child\n");
       	}
    }
}

