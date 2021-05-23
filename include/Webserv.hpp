#ifndef WEBSERV_HPP
# define WEBSERV_HPP

# include <fstream>
# include "include.hpp"
# include "Response.hpp"
# include "Server.hpp"
# include "ConnexionManager.hpp"

class Webserv
{
	public:
		Webserv();
		Webserv(Webserv &x);
		~Webserv();


		/* CORE FUNCTIONS */
		void		run(void);
		void		stop(void);

		/* CONF PARSING FUNCTIONS */
		void		loadConfiguration(std::string filename);
		void		createServer(std::vector<std::string>::iterator start, std::vector<std::string>::iterator end);
		Route		createRoute(std::vector<std::string>::iterator start, std::vector<std::string>::iterator end);


	private:
		ConnexionManager		_manager;
		bool					_is_running;
};

#endif



/* Saving the getIP() function from old class Client, might be useful

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

*/
