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
#include <assert.h>

#define PORT "9034"
#define BACKLOG 10


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
			src = &(sa6->sin6_addr);
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

	if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
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


void handle_new_connection(int listener, fd_set *master, int *fdmax) {
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


char** str_split(char* a_str, const char a_delim) {
	char** result = 0;
	size_t count = 0;
	char* tmp = a_str;
	char* last_comma = 0;
	char delim[2];
	delim[0] = a_delim;
	delim[1] = 0;

	while (*tmp) {
		if (a_delim == *tmp) {
			count++;
			last_comma = tmp;
		}
		tmp++;
	}
	
	count += last_comma < (a_str + strlen(a_str) - 1);
	count++;
	
	result = malloc(sizeof(char*) * count);
	
	if (result) {
		size_t idx = 0;
		char* token = strtok(a_str, delim);
		
		while (token) {
			assert(idx < count);
			*(result + idx++) = strdup(token);
			token = strtok(0, delim);
		}
		assert(idx == count -1);
		*(result +idx) = 0;
	}
	return result;

}


void parse_client_data(char buf[]) {
	char first_line[256];
	int i;
	for(i = 0; buf[i] != '\n'; i++) {
		first_line[i] = buf[i];
	} 
	first_line[i] = '\0';

	printf("first line: %s\n", first_line);	
	char** tokens;
	
	tokens = str_split(first_line, '/');

	if (tokens) {
		for(int i = 0; *(tokens + i); i++) {
			printf("token number(%d): %s\n", i, tokens[i]);
			free(*(tokens + i));
		}
		printf("\n");
		free(tokens);
	}
}


void handle_client_data(int s, int listener, fd_set *master, int fdmax) {
	char buf[256];
	int nbytes;

	if ((nbytes = recv(s, buf, sizeof buf, 0)) <= 0) {
		if (nbytes == 0) {
			printf("selectserver: socket %d hung up", s);
		} else {
			perror("recv");
		}		
		close(s);
		FD_CLR(s, master);
	} else {
		parse_client_data(buf);
		return;
	}
}


int main(void)
{
	fd_set master;
	fd_set read_fds;
	int fdmax;

	int listener;
	listener = get_listener_socket();

	FD_SET(listener, &master);

	fdmax = listener;


	printf("listening on port: %s\n", PORT);
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
		
	}

	return 0;
}
