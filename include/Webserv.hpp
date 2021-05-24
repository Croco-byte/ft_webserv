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
		/* TYPEDEFS */
		typedef typename std::vector<std::string>::iterator		ConfIterator;

		/* CONSTRUCTORS | DESTRUCTOR */
		Webserv();
		Webserv(Webserv &x);
		~Webserv();


		/* CORE FUNCTIONS */
		void								run(void);
		void								stop(void);

		/* CONF PARSING FUNCTIONS */
		void								parseConfiguration(std::string filename);
		Route								createRoute(ConfIterator start, ConfIterator end);


	private:
		ConnexionManager		_manager;
		bool					_is_running;

		/* PRIVATE HELPERS : UTILITY IDENTIFIERS */
		bool								isServBlockStart(std::string const & line)				const;
		bool								isRouteBlockStart(std::string const & line)				const;
		bool								isValidDirective(std::string const & directive)			const;
	
		/* PRIVATE HELPERS : SERVER CONF PARSING*/
		bool								handleConfLine(std::string const & line, ServerConfiguration & conf);
		void								parseServerConfLine(std::string & line, ServerConfiguration & conf);
		void								createServers(std::vector<std::string> & lines);
		void								createServer(ConfIterator start, ConfIterator end);

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
