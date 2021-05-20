#ifndef WEBSERV_HPP
# define WEBSERV_HPP

# include <fstream>
# include <sstream>
# include "include.hpp"
# include "Server.hpp"

class Webserv
{
	public:
		Webserv();
		Webserv(Webserv &x);
		~Webserv();

		void	loadConfiguration(std::string filename);
		void	createServer(std::vector<std::string>::iterator start, std::vector<std::string>::iterator end);
		Route	createRoute(std::vector<std::string>::iterator start, std::vector<std::string>::iterator end);
		void	run();
		void	stop();

	private:
		std::vector<Server>	vecServer;
		bool				_is_running;
};

#endif