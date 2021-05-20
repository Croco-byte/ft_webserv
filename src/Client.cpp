#include "Client.hpp"

Client::Client(t_socket	sock_fd, t_sockaddr_in s)
{
	sock = sock_fd;
	sin = s;
	fcntl(sock, F_SETFL, O_NONBLOCK);
}

bool	Client::waitRequest()
{
	char	buffer[2048];
	int		size;
	bool	ret;

	ret = false;
	request = "";
	while ((size = recv(sock, &buffer[0], 1, 0)) > 0)
	{
		buffer[size] = '\0';
		request.append(buffer);
		ret = true;
	}
	return (ret);
}

void	Client::closeConnection()
{
	close(sock);
}

std::string	Client::getRequest() const
{
	return (request);
}

void		Client::sendResponse(std::string request)
{
	request = request + "\0";
	send(sock, request.c_str(), request.length() + 1, 0);
}

std::string	Client::getIP() const
{
	struct sockaddr_in	*sock_ipV4 = (struct sockaddr_in*)&sin;
	struct in_addr		ipV4 = sock_ipV4->sin_addr;
	uint32_t			ip = ipV4.s_addr;
	unsigned int		tab[4];
	std::string			ip_str;

	tab[0] = (ip & 0xff000000) >> 24;
	tab[1] = (ip & 0x00ff0000) >> 16;
	tab[2] = (ip & 0x0000ff00) >> 8;
	tab[3] = (ip & 0x000000ff);
	ip_str += std::to_string(tab[3]) + "." + std::to_string(tab[2]) + "." + std::to_string(tab[1]) + "." + std::to_string(tab[0]);
	Console::info("IP CLIENT : " + ip_str);
	return (ip_str);
}