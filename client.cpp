#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
 
#include <stdio.h>
#include <stdlib.h>
 
 
 
int main(int ac, char **av)
{
	int erreur = 0;

	SOCKET sock;
	SOCKADDR_IN sin;

	if(!erreur)
	{
		sock = socket(AF_INET, SOCK_STREAM, 0);
 
		sin.sin_addr.s_addr = inet_addr("127.0.0.1");
		sin.sin_family = AF_INET;
		sin.sin_port = htons(atoi(av[1]));

		if(connect(sock, (SOCKADDR*)&sin, sizeof(sin)) != SOCKET_ERROR)
			printf("Connexion à %s sur le port %d\n", inet_ntoa(sin.sin_addr), htons(sin.sin_port));
		else
			printf("Impossible de se connecter\n");

		while (1)
		{
			std::string str;
			std::cin >> str;
			send(sock, str.c_str(), str.size(), 0);
			if (str == "exit")
			{
				close(sock);
				break ;
			}
		}

		closesocket(sock);
	}

	return EXIT_SUCCESS;
}
