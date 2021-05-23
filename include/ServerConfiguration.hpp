#ifndef SERVER_CONFIGURATION_HPP
# define SERVER_CONFIGURATION_HPP

# include "include.hpp"
# include "Route.hpp"

class	ServerConfiguration
{
	public:
		ServerConfiguration();
		ServerConfiguration(const ServerConfiguration &cpy);
		virtual ~ServerConfiguration();

		void							setName(std::string name);
		void							setHost(std::string host);
		void							addErrorPageLocation(int code, std::string location);
		void							setPort(int port);
		void							setLimitBodySize(int limit);
		void							addRoute(Route route);

		std::string						getName() const;
		std::string						getHost() const;
		std::string						getErrorPageLocation(int code);
		std::map<int, std::string>		getErrorPageLocations() const;
		int								getPort() const;
		int								getLimit() const;
		std::vector<Route>				getRoutes() const;

	private:
		std::string					_name;
		std::string					_host;
		std::map<int, std::string>	_error_page_locations;

		int							_port;
		int							_limit_body_size;
		
		std::vector<Route>			_vecRoutes;

		std::string					default_error_page_loc;
};

#endif
