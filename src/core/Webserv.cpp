#include "core/Webserv.hpp"


/*
** ------ PRIVATE HELPERS : IDENTIFIERS ------
*/
bool						Webserv::isServBlockStart(std::string const & line) const
{
	int		i = 0;

	while(line[i] != '\0' && Utils::ft_isspace(line[i]))
		i++;
	if (line.compare(i, 6, "server") != 0)
		return (false);
	i += 6;
	while(line[i] && Utils::ft_isspace(line[i]))
		i++;
	if (!line[i] || line[i] != '{')
		return (false);
	i++;
	while(line[i] && Utils::ft_isspace(line[i]))
		i++;
	if (line[i] != '\0')
		return (false);
	return (true);
}

bool						Webserv::isRouteBlockStart(std::string const & line) const
{
		int		i(0);
	while(line[i] && Utils::ft_isspace(line[i]))
		i++;
	if (line.compare(i, 8, "location") != 0)
		return (false);
	i += 8;
	if (!Utils::ft_isspace(line[i]))
		return (false);
	while(line[i] && line[i] != '{')
		i++;
	if (!line[i] || line[i] != '{')
		return (false);
	i++;
	while(line[i] && Utils::ft_isspace(line[i]))
		i++;
	if (line[i] != '\0')
		return (false);
	return (true);
}

bool						Webserv::isValidServDirective(std::string const & directive) const
{
	if (directive == "listen" || directive == "host" || directive == "server_name"
		|| directive == "root" || directive == "error")
		return (true);
	return (false);
}

bool						Webserv::isValidRouteDirective(std::string const & directive) const
{
	if (directive == "root" || directive == "index" || directive == "autoindex"
		|| directive == "cgi_extension" || directive == "cgi_bin"
		|| directive == "methods" || directive == "auth_basic" || directive == "auth_basic_user_file"
		|| directive == "language" || directive == "client_max_body_size")
		return (true);
	return (false);
}


/*
** ------ PRIVATE HELPERS : SERVER CREATION ------
*/
void						Webserv::createServers(std::vector<std::string> & lines)
{
	int			brace_level(0);
	size_t		i = 0;

	for (ConfIterator it = lines.begin(); it != lines.end() && i < lines.size(); it++)
	{
		if (isServBlockStart(*it))
		{
			ConfIterator block_start = it;
			it++;
			i++;
			brace_level++;
			while (brace_level > 0 && it != lines.end())
			{
				if ((*it).find('{') != std::string::npos)
					brace_level++;
				if ((*it).find('}') != std::string::npos)
					brace_level--;
				it++;
				i++;
			}
			if (brace_level == 0)
				this->createServer(block_start, it);
			else
			{
				Console::error("Configuration parsing failure: missing '}'");
				exit(1);
			}
		}
		i++;
	}
}

void	Webserv::createServer(ConfIterator start, ConfIterator end)
{
	Server								server;
	ServerConfiguration					conf;

	start++;
	end--;
	for (; start != end; start++)
	{
		if (isRouteBlockStart(*start))
		{
			ConfIterator block_start = start;
			start++;
			while (start != end && (*start).find('}') == std::string::npos)
				start++;
			ConfIterator block_end = start;
			Route parsedRoute = this->createRoute(block_start, block_end, conf);
			conf.addRoute(parsedRoute);
		}
		else if (!(Utils::is_empty(*start)))
			parseServerConfLine(*start, conf);
	}
	std::cout << conf << std::endl;

	for (std::vector<Server>::iterator it = _manager.getServers().begin(); it != _manager.getServers().end(); it++)
	{
		if ((*it).getDefaultVHConfig().getHost() == conf.getHost() && (*it).getDefaultVHConfig().getPort() == conf.getPort())
		{
			if (!conf.hasRootRoute())
			{
				Console::error("Configuration parsing failure: all servers and virtual hosts must have a '/' location");
				exit(1);
			}
			(*it).addVirtualHost(conf);
			return ;
		}
	}
	if (!conf.hasRootRoute())
	{
		Console::error("Configuration parsing failure: all servers and virtual hosts must have a '/' location");
		exit(1);
	}
	server.addVirtualHost(conf);
	_manager.addServer(server);
}


/*
** ------ PRIVATE HELPERS : SERVER CONF PARSING ------
*/
void						Webserv::parseServerConfLine(std::string & line, ServerConfiguration & conf)
{
	Utils::trim(line);
	if (line.size() < 4)
	{
		Console::error("Configuration parsing failure: unexpected line '" + line + "'");
		exit(1);
	}
	if (line[line.size() - 1] != ';')
	{
		Console::error("Configuration parsing failure: expected ';' after '" + line + "'");
		exit(1);
	}
	line = line.substr(0, line.size() - 1);
	if (!(handleServerConfLine(line, conf)))
	{
		Console::error("Configuration parsing failure: unknown directive on line '" + line + "'");
		exit(1);
	}
}

bool						Webserv::handleServerConfLine(std::string const & line, ServerConfiguration & conf)
{
	std::vector<std::string>			tmp;
	std::string							instruction;
	std::vector<std::string>			params;

	tmp = Utils::split_any(line, " 	");
	if (tmp.size() < 2)
		return (false);
	instruction = Utils::trim(tmp[0]);
	params = Utils::split(tmp[1], ",");
	for (size_t i = 0; i < params.size(); i++)
		Utils::trim(params[i]);
	
	if (isValidServDirective(instruction))
	{
		if (instruction == "listen")
			conf.setPort(std::atoi(params[0].c_str()));
		else if (instruction == "host")
			conf.setHost(params[0]);
		else if (instruction == "server_name")
			conf.setName(params[0]);
		else if (instruction == "root")
			conf.setServerRoot(params[0]);
		else if (instruction == "error")
		{
			if (params.size() == 2 && Utils::is_positive_number(params[1]))
				conf.addCustomErrorPage(std::atoi(params[1].c_str()), params[0]);
			else
				return (false);
		}
		return (true);
	}
	return (false);
}


/*
** ------ PRIVATE HELPERS : ROUTE CONF PARSING ------
*/

Route	Webserv::createRoute(ConfIterator start, ConfIterator end, ServerConfiguration const & conf)
{
	Route						route;

	std::vector<std::string>	firstLine = Utils::split_any(Utils::trim(*start), " 	");
	if (firstLine.size() != 3)
	{
		Console::error("Configuration parsing failure: wrong format for location line '" + *start + "'");
		exit(1);
	}
	route.setRoute(firstLine[1], conf.getServerRoot());
	start++;
	for (; start != end; start++)
	{
		if (!(Utils::is_empty(*start)))
			parseRouteConfLine(*start, route);
	}
	return (route);
}

void						Webserv::parseRouteConfLine(std::string & line, Route & route)
{
	Utils::trim(line);
	if (line.size() < 4)
	{
		Console::error("Configuration parsing failure: unexpected line '" + line + "'");
		exit(1);
	}
	if (line[line.size() - 1] != ';')
	{
		Console::error("Configuration parsing failure: expected ';' after '" + line + "'");
		exit(1);
	}
	line = line.substr(0, line.size() - 1);
	if (!(handleRouteLine(line, route)))
	{
		Console::error("Configuration parsing failure: unknown directive on line '" + line + "'");
		exit(1);
	}
}

bool						Webserv::handleRouteLine(std::string const & line, Route & route)
{
	std::vector<std::string>			tmp;
	std::string							instruction;
	std::vector<std::string>			params;

	tmp = Utils::split_any(line, " 	");
	if (tmp.size() < 2)
		return (false);
	instruction = Utils::trim(tmp[0]);
	params = Utils::split(tmp[1], ",");
	for (size_t i = 0; i < params.size(); i++)
		Utils::trim(params[i]);
	
	if (isValidRouteDirective(instruction))
	{
		if (instruction == "root")
			route.setLocalURL(params[0]);
		else if (instruction == "index")
			route.setIndex(params[0]);
		else if (instruction == "autoindex")
			route.setAutoIndex(Utils::string_to_bool(params[0]));
		else if (instruction == "cgi_extension")
			route.setCGIExtensions(params);
		else if (instruction == "cgi_bin")
			route.setCGIBinary(params[0]);
		else if (instruction == "methods")
			route.setAcceptedMethods(params);
		else if (instruction == "auth_basic" && params[0] == "on")
			route.enableRequireAuth();
		else if (instruction == "auth_basic_user_file")
			route.setUserFile(params[0]);
		else if (instruction == "language")
			route.setRouteLang(params);
		else if (instruction == "client_max_body_size")
			route.setMaxBodySize(std::atoi(params[0].c_str()));
		return (true);
	}
	return (false);
}



/*
** ------ CONSTRUCTORS / DESTRUCTOR ------
*/

Webserv::Webserv(): _is_running(true)
{}

Webserv::Webserv(Webserv &x)
{ (void)x; }

Webserv::~Webserv()
{}



/*
** ------ CORE FUNCTIONS ------
*/

void	Webserv::run(void)
{
	if (_manager.setup() == -1)
	{
		Console::error("Couldn't setup manager");
		exit(1);
	}
	_manager.run();
}

void	Webserv::stop(void)
{
	_is_running = false;
	_manager.clean();
}


/*
** ------ CONFIGURATION PARSING FUNCTIONS ------
*/

void	Webserv::parseConfiguration(std::string filename)
{
	std::string							line;
	std::vector<std::string>			lines;
	std::ifstream						file(filename.c_str());

	if (!(file.is_open() && file.good()))
	{
		Console::error("Cannot open conf file");
		return ;
	}
	while (std::getline(file, line))
		lines.push_back(line);
	this->createServers(lines);
}
