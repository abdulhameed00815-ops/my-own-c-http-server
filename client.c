#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libs/socket.h"

int main()
{
	int bytes;
	char buff[100];
	Socket *s = init_socket(AF_INET, SOCK_STREAM);
	s->connect(s, "127.0.0.1", 12345);

	while (1) {
		memset(buff, 0, sizeof(buff));
		
		printf(">>");

		if (fgets(buff, sizeof(buff), stdin) == NULL) {
			perror("fgets error");
			break;
		}

		s->send(s, buff);
		s->recv(s, buff, sizeof(buff));
	}

	socket_close(s);
}
