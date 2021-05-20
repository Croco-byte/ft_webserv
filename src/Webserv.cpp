#include "Webserv.hpp"

Webserv::Webserv()
 : _is_running(true)
{

}

Webserv::Webserv(Webserv &x)
{
	(void)x;
}

Webserv::~Webserv()
{

}

void	Webserv::loadConfiguration(std::string filename)
{
	std::ifstream file(filename.c_str());

	if (file.is_open() && file.good())
	{
		std::string							line;
		int									brace_level = 0;
		std::vector<std::string>			lines;
		std::vector<std::string>::iterator	server_start_line;
		std::vector<std::string>::iterator	server_end_line;

		while (std::getline(file, line))
			lines.push_back(line);
		for (std::vector<std::string>::iterator it = lines.begin(); it != lines.end(); it++)
		{
			line = *it;
			if (line.find('{') != std::string::npos)
			{
				if (brace_level == 0)
				{
					Console::info("init");
					server_start_line = it + 1;
				}
				brace_level++;
			}
			if (line.find('}') != std::string::npos)
			{
				brace_level--;
				if (brace_level == 0)							// Si on parse un server
				{
					server_end_line = lines.end();
					this->createServer(server_start_line, server_end_line);
				}
			}
		}
	}
	else
		Console::error("Cannot open conf file");
}

void	Webserv::createServer(std::vector<std::string>::iterator start, std::vector<std::string>::iterator end)
{
	Server								server;
	int									brace_level = 0;
	ServerConfiguration					conf;
	std::string							instruction;
	std::string							param;
	std::vector<std::string>			vecParam;
	std::vector<std::string>			tmp;
	std::vector<std::string>::iterator	route_start_line;
	std::vector<std::string>::iterator	route_end_line;
	Route								tmp_route;

	while (start != end)
	{
		if ((*start).find('{') != std::string::npos)
		{
			if (brace_level == 0)
				route_start_line = start;
			brace_level++;
		}
		else if ((*start).find('}') != std::string::npos)
		{
			brace_level--;
			if (brace_level == 0)							// Si on parse une route
			{
				route_end_line = start;
				tmp_route = this->createRoute(route_start_line, route_end_line);
				conf.addRoute(tmp_route);
			}
			else if (brace_level == -1)						// Si on a finit de parse un server
			{
				server.load(conf);
				vecServer.push_back(server);
				server = vecServer.back();
				conf = server.getConfiguration();
				Console::info("Create server : " + conf.getName() + " " + conf.getHost() + ":" + std::to_string(conf.getPort()));
				conf = ServerConfiguration();
			}
		}
		Utils::trim(*start);
		*start = Utils::remove_char(*start, ";");
		tmp = Utils::split(*start, " ");
		if (tmp.size() >= 2 && brace_level == 0)
		{
			instruction = Utils::trim(tmp[0]);
			param = Utils::trim(tmp[1]);
			vecParam = Utils::split(param, ",");
			for (size_t i = 0; i < vecParam.size(); i++)
				Utils::trim(vecParam[i]);

			std::cout << instruction << " : " << param << std::endl;
			if (instruction == "listen")
				conf.setPort(std::atoi(param.c_str()));
			else if (instruction == "host")
				conf.setHost(param);
			else if (instruction == "server_name")
				conf.setName(param);
			else if (instruction == "error")
				conf.addErrorPageLocation(std::atoi(vecParam[0].c_str()), vecParam[1]);
			else if (instruction == "client_max_body_size")
				conf.setLimitBodySize(std::atoi(param.c_str()));
		}
		start++;
	}
}

Route	Webserv::createRoute(std::vector<std::string>::iterator start, std::vector<std::string>::iterator end)
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
	std::cout << route << std::endl;
	return (route);
}

void	Webserv::run()
{
	std::vector<Server>::iterator	iterator = vecServer.begin();

	while (iterator != vecServer.end())
	{
		iterator->init();
		iterator->start();
		iterator++;
	}

	while (_is_running)
	{
		iterator = vecServer.begin();
		while (iterator != vecServer.end())
		{
			iterator->loop();
			iterator++;
		}
	}
}

void	Webserv::stop()
{
	_is_running = false;
	std::vector<Server>::iterator	iterator = vecServer.begin();
	while (iterator != vecServer.end())
	{
		iterator->stop();
		iterator++;
	}
}