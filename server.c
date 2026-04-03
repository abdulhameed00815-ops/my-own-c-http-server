#include <stdio.h>
#include <ctype.h>
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


//this function moves the pointer of the string from whitespace till it reaches a non-whitespace character (left trims it).
char *ltrim(char *s)
{
    while(isspace(*s)) s++;
    return s;
}


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
	char *header_key = "";
	char *header_value = "";
	struct sockaddr_storage their_addr;
	socklen_t sin_size;
	struct addrinfo hints, *servinfo, *p;
	struct sigaction sa;
	int yes=1;
	int i = 0;
	char s[INET6_ADDRSTRLEN];
	char *request_line_parts[10];
	int rv, byte_count;
	int sockfd, newfd;
	char buf[MAXDATASIZE];
	char lines[5][1024];
	char headers[7][1024];
	char line[1024];
	char *request_method = "";
	char *request_path = "";
	char *request_protocol = "";
	char header_keys[7][1024];
	char header_values[7][1024];
	char body_lines[7][1024];

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
			int n;
			int total = 0;
			//so the buf + total part tells the function from where to start writing the recieved data in memory.
			n = recv(newfd, buf + total, MAXDATASIZE-1, 0);
			total += n;

                        char *headers_end = strstr(buf, "\r\n\r\n");

			while (headers_end == NULL) {

				n = recv(newfd, buf + total, MAXDATASIZE-1, 0);
				headers_end = strstr(buf, "\r\n\r\n");
				total += n;
			}
			printf("headers recieved successfuly\n");
			printf("%s\n\n", buf);

                        if (headers_end) {
				int header_len = headers_end - buf + 4;

				char *cl = strstr(buf, "Content-Length:");
				if (cl) {
					int content_length;
					sscanf(cl, "Content-Length: %d", &content_length);

					while (total < header_len + content_length) {
						n = recv(newfd, buf + total, MAXDATASIZE - total - 1, 0);
						if (n <= 0) break;
						total += n;
					}
				}
			}

			char new_buf[] = {0};

			sscanf(buf, "%[^\n]", new_buf);	

			printf("new_buf: %s", new_buf);

			int length = sizeof(lines) / sizeof(lines[0]);
			printf("%d\n", length);

			for (int i = 0; i < 5; i++) {
				printf("%s\n", lines[i]);
			}
			i = 0;

			char *request_line = lines[0];

			char *myPtr = strtok(request_line, " ");

			while (myPtr != NULL) {
				request_line_parts[i] = myPtr;
				myPtr = strtok(NULL, " ");
				i++;
			}
			i = 0;

			request_method = request_line_parts[0];

			request_path = request_line_parts[1];

			request_protocol = request_line_parts[2];

			//parses header lines.
			for (i = 0; i < 6; i++) {
				if (!(lines[i] == request_line)) {
				       strcpy(headers[i], lines[i]);
				       printf("header%d: %s\n", i, headers[i]);
				}

				if (strcmp(lines[i], "\n") == 0) {
					break;
				}
			}


			//actually parses headers.
			for (i = 1; i < 5; i++) {
				char *token = "";
				token = strtok(headers[i], ":");
				strcpy(header_keys[i-1], token);
				//this second token is the header value.
				token = strtok(NULL, "\0");
				token = ltrim(token);
				strcpy(header_values[i-1], token);
			}

			int j = 0;
			int body_line_count = 0;
			int line_is_header;
			for (i = 0; i < 8; i++) {
				if (i != 0) {
					printf("line%d: %s", i, lines[i]);
					for (j = 0; j < 5; j++) {
 						line_is_header = strcmp(lines[i], headers[j]);
						printf("%d", line_is_header);
						if (line_is_header != 0) {
							printf("body_line%d: %s", i, lines[i]);
							strcpy(body_lines[body_line_count], lines[i]);
							body_line_count++;
						}
					}
				}
			}

			for (i = 0; i < 5; i++) {
				printf("body line%d: %s", i, body_lines[i]);
			} 

				
			if (send(newfd, "hello, bird", 11, 0) == -1) {
				perror("send");
			}
			close(newfd);
			exit(0);
		}
		close(newfd);
	}

	return 0;
}
