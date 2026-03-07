#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MYPORT "3940"
#define BACKLOG 10

int main(void)
{
	struct sockaddr_storage their_addr;
	socklen_t addr_size;
	struct addrinfo hints, *res, *p;
	int sockfd, newfd;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, MYPORT, &hints, &res);
		
	setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	bind(sockfd, res->ai_addr, res->ai_addrlen)

	while(true){
		listen(sockfd, BACKLOG);
		addr_size = sizeof their_addr;
		newfd = accept(sockfd, (struct sockaddr *)&theiraddr, &addr_size);
		recv(sockfd, void *buf, 100, 0);
	}

	}

	freeaddrinfo(res);

	return 0;
}
