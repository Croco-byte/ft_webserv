#include "Webserv.hpp"

/*
** ------ PRIVATE CONF PARSING HELPERS ------
*/

bool						Webserv::isServBlockStart(std::string const & line) const
{
	int		i(0);
	while(line[i] && Utils::ft_isspace(line[i]))
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

bool						Webserv::isValidDirective(std::string const & directive) const
{
	if (directive == "listen" || directive == "host" || directive == "server_name"
		|| directive == "error" || directive == "client_max_body_size")
		return (true);
	return (false);
}

bool						Webserv::handleConfLine(std::string const & line, ServerConfiguration & conf)
{
	std::vector<std::string>			tmp;
	std::string							instruction;
	std::vector<std::string>			params;

	tmp = Utils::split(line, " ");
	if (tmp.size() < 2)
		return (false);
	instruction = Utils::trim(tmp[0]);
	params = Utils::split(tmp[1], ",");
	for (size_t i = 0; i < params.size(); i++)
		params[i] = Utils::trim(params[i]);
	
	if (isValidDirective(instruction))
	{
		if (instruction == "listen")
			conf.setPort(std::atoi(params[0].c_str()));
		else if (instruction == "host")
			conf.setHost(params[0]);
		else if (instruction == "server_name")
			conf.setName(params[0]);
		else if (instruction == "error")
		{
			if (params.size() == 2 && Utils::is_positive_number(params[1]))
				conf.addErrorPageLocation(std::atoi(params[0].c_str()), params[1]);
			else
				Console::error("On parsing '" + line + "'");
		}
		else if (instruction == "client_max_body_size")
			conf.setLimitBodySize(std::atoi(params[0].c_str()));
		return (true);
	}
	return (false);
}

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
	if (!(handleConfLine(line, conf)))
	{
		Console::error("Configuration parsing failure: unknown directive on line '" + line + "'");
		exit(1);
	}
}

void						Webserv::createServers(std::vector<std::string> & lines)
{
	int			brace_level(0);

	for (ConfIterator it = lines.begin(); it != lines.end(); it++)
	{
		if (isServBlockStart(*it))
		{
			ConfIterator block_start = it;
			it++;
			brace_level++;
			while (brace_level && it != lines.end())
			{
				if ((*it).find('{') != std::string::npos)
					brace_level++;
				if ((*it).find('}') != std::string::npos)
					brace_level--;
				it++;
			}
			if (brace_level == 0)
				this->createServer(block_start, it);
			else
			{
				Console::error("Configuration parsing failure: missing '}'");
				exit(1);
			}
		}
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
			Route parsedRoute = this->createRoute(block_start, block_end);
			conf.addRoute(parsedRoute);
		}
		else if (!(Utils::is_empty(*start)))
			parseServerConfLine(*start, conf);
	}
	std::cout << conf << std::endl;
	server.load(conf);
	_manager.addServer(server);
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

Route	Webserv::createRoute(ConfIterator start, ConfIterator end)
{
	Route						route;
	std::string					instruction;
	std::string					param;
	std::vector<std::string>	vecParam;
	std::vector<std::string>	tmp;

	*start = Utils::remove_char(*start, "{");
	while (start != end)
	{
		tmp = Utils::split(Utils::remove_char(Utils::trim(*start), ";"), " ");
		if (tmp.size() == 2)
		{
			instruction = tmp[0];
			param = tmp[1];
			vecParam = Utils::split(param, ",");
			for (size_t i = 0; i < vecParam.size(); i++)
				Utils::trim(vecParam[i]);

			if (instruction == "location")
				route.setRoute(param);
			else if (instruction == "root")
				route.setLocalURL(param);
			else if (instruction == "index")
				route.setIndex(param);
			else if (instruction == "autoindex")
				route.setAutoIndex(Utils::string_to_bool(param));
			else if (instruction == "upload_dir")
				route.setUploadDir(param);
			else if (instruction == "cgi_extension")
				route.setCGIExtensions(vecParam);
			else if (instruction == "cgi_bin")
				route.setCGIBinary(param);
			else if (instruction == "methods")
				route.setAcceptedMethods(vecParam);
			else if (instruction == "auth")
			{
				std::vector<std::string> vec = Utils::split(param, ":");
				if (vec.size() != 2)
					Console::error("auth bad configured in route " + route.getRoute());
				else
				{
					route.setAuthId(vec[0]);
					route.setAuthPass(vec[1]);
				}
			}
		}
		else
			Console::error("in parsing of route : " + *start);
		start++;
	}
	return (route);
}
