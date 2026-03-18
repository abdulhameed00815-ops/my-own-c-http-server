#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define MYPORT "3940"
#define BACKLOG 10
#define MAXDATASIZE 100

//clean dead children processes.
void sigchld_handler(int s)
{
	(void)s;

	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
	struct sockaddr_storage their_addr;
	socklen_t sin_size;
	struct addrinfo hints, *servinfo, *p;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv, byte_count;
	int sockfd, newfd;
	char buf[MAXDATASIZE];
	char lines[5][1024];
	FILE *fp;
	char s1[1024];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	//the getaddrinfo function makes the os give us some ready to use socket address configurations for our server based on our hints.
	if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo))) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	//here we loop thro the results of getaddrinfo, pick a working address, and create a socket using it.
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		//this allows us to bind to the same port after closing the server without issues.
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
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
	
	freeaddrinfo(servinfo);

	if (p == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	if (sigaction(SIGCHLD, &sa, NULL) == -1){
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {
		sin_size = sizeof their_addr;
		newfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

		if (newfd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) {
			close(sockfd);
			if ((byte_count = recv(newfd, buf, MAXDATASIZE-1, 0)) == -1) {
				perror("recv");
			}
			printf("original recieved message: %s\n", buf);

			FILE *wf = fopen("client_msg.txt", "w");

			fputs(buf, wf);
			FILE *rf = fopen("client_msg.txt", "r");
			while (fgets(s1, sizeof s1, rf) != NULL) {
				printf("%s", s);
			}
			int length = sizeof(lines) / sizeof(lines[0]);
			printf("%d\n", length);
			//modified the function to send the buffered recieved data instead of just a string.
			if (send(newfd, buf, sizeof buf, 0) == -1) {
				perror("send");
			}
			close(newfd);
			exit(0);
		}
		close(newfd);
	}

	return 0;
}
