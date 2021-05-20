#ifndef CLIENT_HPP
# define CLIENT_HPP

# include "include.hpp"
# include "Response.hpp"

class Client
{
	public:
		Client(t_socket sock_fd, t_sockaddr_in sin);

		bool	waitRequest();
		void	closeConnection();
		void	sendResponse(std::string request);

		std::string	getRequest() const;
		std::string	getIP() const;

	private:
		t_socket		sock;
		t_sockaddr_in	sin;
		std::string		request;
};

#endif
