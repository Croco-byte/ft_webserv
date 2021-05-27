#include "ServerConfiguration.hpp"

ServerConfiguration::ServerConfiguration()
 : _name("Default server name"), _host("127.0.0.1"), _port(8080), _limit_body_size(INT_MAX - 1)
{
	char cwd[PATH_MAX];

	getcwd(cwd, sizeof(cwd));
	default_error_page_loc = std::string(&cwd[0]);
	default_error_page_loc += "/www/error.html";
	_error_page_locations[400] = default_error_page_loc;
	_error_page_locations[401] = default_error_page_loc;
	_error_page_locations[403] = default_error_page_loc;
	_error_page_locations[404] = default_error_page_loc;
	_error_page_locations[405] = default_error_page_loc;
	_error_page_locations[406] = default_error_page_loc;
	_error_page_locations[413] = default_error_page_loc;
	_error_page_locations[500] = default_error_page_loc;
	_error_page_locations[501] = default_error_page_loc;
	_error_page_locations[502] = default_error_page_loc;
	_error_page_locations[503] = default_error_page_loc;
}

ServerConfiguration::ServerConfiguration(const ServerConfiguration &x)
{
	_name = x._name;
	_host = x._host;
	_error_page_locations = x._error_page_locations;
	_port = x._port;
	_limit_body_size = x._limit_body_size;
	_vecRoutes = x._vecRoutes;
}

ServerConfiguration::~ServerConfiguration()
{
	// for (std::vector<Route*>::iterator it = _vecRoutes.begin(); it != _vecRoutes.end(); it++)
	// 	delete (*it);
}

void	ServerConfiguration::setName(std::string name)
{
	_name = name;
}

void	ServerConfiguration::setHost(std::string host)
{
	_host = host;
}

void	ServerConfiguration::addErrorPageLocation(int code, std::string location)
{
	_error_page_locations[code] = location;
	Console::info("Add error : " + Utils::to_string(code) + " => " + location);
}

void	ServerConfiguration::setPort(int port)
{
	_port = port;
}

void	ServerConfiguration::setLimitBodySize(int limit)
{
	_limit_body_size = limit;
}

void		ServerConfiguration::addRoute(Route route)
{
	_vecRoutes.push_back(route);
}

std::string	ServerConfiguration::getName() const
{
	return (_name);
}

std::string	ServerConfiguration::getHost() const
{
	return (_host);
}

std::string	ServerConfiguration::getErrorPageLocation(int code)
{
	return (_error_page_locations[code]);
}

std::map<int, std::string> const &		ServerConfiguration::getErrorPageLocations() const
{
	return (_error_page_locations);
}

int			ServerConfiguration::getPort() const
{
	return (_port);
}

int			ServerConfiguration::getLimit() const
{
	return (_limit_body_size);
}

std::vector<Route> const &	ServerConfiguration::getRoutes() const
{
	return (_vecRoutes);
}


std::ostream	& operator<<(std::ostream &stream, ServerConfiguration const & conf)
{
	stream << "\033[0;33m" << std::endl;
	stream << ">>>>>>>>>>>>>> Server configuration <<<<<<<<<<<<<<" << std::endl;
	stream << "\033[0m";
	stream << "    - listen				: " << conf.getPort() << std::endl;
	stream << "    - host				: " << conf.getHost() << std::endl;
	stream << "    - server_name			: " << conf.getName() << std::endl;
	stream << "    - limit_body_size			: " << conf.getLimit() << std::endl;

	for (std::vector<Route>::const_iterator it = (conf.getRoutes()).begin(); it != (conf.getRoutes()).end(); it++)
		std::cout << *it << std::endl;

	return (stream);
}
