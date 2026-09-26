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

#define PORT "3940"
#define BACKLOG 10
#define MAXDATASIZE 100


//this function moves the pointer of the string from whitespace till it reaches a non-whitespace character (left trims it).
char *ltrim(char *s)
{
    while(isspace(*s)) s++;
    return s;
}


const char *inet_ntop2(void *addr, char *buf, size_t size) {
	struct sockaddr_storage *sas = addr;
	struct sockaddr_in *sa4;
	struct sockaddr_in6 *sa6;
	void *src;
	
	switch (sas->ss_family) {
		case AF_INET:
			sa4 = addr;
			src = &(sa4->sin_addr);
			break;
		case AF_INET6:
			sa6 = addr;
			src = &(sa6->sin_addr);
			break;
		default:
			return NULL;
	}
	
	return inet_ntop(sas->ss_family, src, buf, size);
}


int get_listener_socket(void) {
	struct addrinfo hints, *ai, *p;
	int yes=1;
	int rv;
	int listener;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(NULL, PORT, &hints, &ai))) {
		fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
		exit(1);
	}

	for(p = ai; p != NULL; p = p->ai_next) {
		listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener < 0) {
			continue;
		}

		//this allows us to bind to the same port after closing the server without issues.
		if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
			close(listener);
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		exit(2);
	}
	
	freeaddrinfo(ai);

	if (listen(listener, 10) == -1) {
		perror("listen");
		exit(3);
	}

	return listener;
}

printf("waiting for incoming connections...");


void handle_new_connnection(int listener, fd_set *master, int *fdmax) {
	socklen_t addrlen;
	int newfd;
	struct sockaddr_storage remoteaddr;	
	char remoteIP[INET6_ADDRSTRLEN];

	addrlen = sizeof remoteaddr;
	newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);
	
	if (newfd == -1) {
		perror("accept");
	} else {
		FD_SET(newfd, master);
		if (newfd > *fdmax) {
			*fdmax = newfd;
		}
		printf("selectserver: new connection from %s on socket %d\n", inet_ntop2(&remoteaddr, remoteIP, sizeof remoteIP), newfd);
	}
}


void handle_client_data(int s, int listener, fd_set *master, int fdmax) {
	char buf[256];
	int nbytes;

	if ((nbytes = recv(s, buf, sizeof buf, 0)) <= 0) {
		if (nbytes == 0) {
			printf("selectserver: socket %d hung up");
		} else {
			perror("recv");
		}		
		close(s);
		FD_CLR(s, master);
	} else {
		int *client_data = nbytes;
	}
}


int main(void)
{
	fd_set master;
	fd_set read_fds;
	int fdmax;

	int listener;

	FD_SET(listener, &master);

	fdmax = listener;

	//unfinished server code
	char *header_key = "";
	char *header_value = "";
	int byte_count;
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


	//here we loop thro the results of getaddrinfo, pick a working address, and create a socket using it.

	printf("server: waiting for connections...\n");

	for (;;) {
		read_fds = master;
		if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(4);
		}

		for(int i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) {
				if (i == listener) {
					handle_new_connection(i, &master, &fdmax);
				} else {
					handle_client_data(i, listener, &master, fdmax);
				}
			}
		}
		
		//unfinished server code
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
