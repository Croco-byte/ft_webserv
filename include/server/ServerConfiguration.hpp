#ifndef SERVER_CONFIGURATION_HPP
# define SERVER_CONFIGURATION_HPP

# include "utils/include.hpp"
# include "server/Route.hpp"

class	ServerConfiguration
{
	public:
		ServerConfiguration();
		ServerConfiguration(const ServerConfiguration &cpy);
		~ServerConfiguration();

		void									setName(std::string const & name);
		void									setHost(std::string const & host);
		void									addErrorPageLocation(int code, std::string location);
		void									setPort(int port);
		void									setLimitBodySize(int limit);
		void									setServerRoot(std::string const & server_root);
		void									addRoute(Route route);

		std::string const &						getName()														const;
		std::string const &						getHost()														const;
		std::string const &						getErrorPageLocation(int code);
		std::map<int, std::string> const &		getErrorPageLocations()											const;
		int										getPort()														const;
		int										getLimit()														const;
		std::string const &						getServerRoot(void)												const;
		std::vector<Route> const &				getRoutes()														const;

	private:
		std::string					_name;
		std::string					_host;
		std::map<int, std::string>	_error_page_locations;
		int							_port;
		int							_limit_body_size;
		std::string					_server_root;
		std::vector<Route>			_vecRoutes;
		std::string					default_error_page_loc;
};

std::ostream	&operator<<(std::ostream &stream, ServerConfiguration const & conf);


#endif
