/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yel-alou <yel-alou@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/05/22 11:06:00 by user42            #+#    #+#             */
/*   Updated: 2021/05/25 08:07:07 by yel-alou         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

/*
** ------ CONSTRUCTORS / DESTRUCTOR ------
*/
Server::Server()
{}

Server::Server(const Server &x)
{
	_fd = x._fd;
	_addr = x._addr;
	_requests = x._requests;
	_config = x._config;
}

Server::~Server()
{}



/*
** ------ SETUP AND CLOSE SERVER ------
*/
int		Server::setup(void)
{
	int	on(1);
	int	rc(0), flags(0);

	_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_fd < 0)
	{
		Console::error("Couldn't create server : socket() call failed");
		return (-1);
	}
	rc = setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&on, sizeof(on));
	if (rc < 0)
	{
		Console::error("Couldn't create server : setsockopt() call failed");
		close(_fd);
		return (-1);
	}
	flags = fcntl(_fd, F_GETFL, 0);
	rc = fcntl(_fd, F_SETFL, flags | O_NONBLOCK);
	if (rc < 0)
	{
		Console::error("Couldn't create server : fcntl() call failed");
		close(_fd);
		return (-1);
	}
	_addr.sin_family = AF_INET;
	_addr.sin_port = htons(_config.getPort());
	_addr.sin_addr.s_addr = inet_addr((_config.getHost().c_str()));
	rc = bind(_fd, (struct sockaddr *)&_addr, sizeof(_addr));
	if (rc < 0)
	{
		Console::error("Couldn't create server [" + _config.getHost() + ":" + Utils::to_string(_config.getPort()) + "]" + " : bind() call failed");
		close(_fd);
		return (-1);
	}
	rc = listen(_fd, 1000);
	if (rc < 0)
	{
		Console::error("Couldn't create server : listen() call failed");
		close(_fd);
		return (-1);
	}
	return(0);
}

void				Server::clean(void)
{
	_requests.clear();
	close(_fd);
}



/*
** ------ SEND / RECV / ACCEPT ------
*/
long				Server::send(long socket)
{
	Request		request;
	Response	response;

	std::string request_str = _requests[socket];
	_requests.erase(socket);
	request.load(request_str);

	this->handleRequestHeaders(request, response);
	this->handleRequestURL(request, response);

	std::string body = "<html><body><h1>YASSSS</h1></body></html>";
	response.setHeader("Content-Length", Utils::to_string(body.length()));
	response.setBody(body);
	std::string toSend = response.build();

	int ret = ::send(socket, toSend.c_str(), toSend.size(), 0);
	std::cout << std::endl << GREEN << "------ Sent response ------" << std::endl << "[" << std::endl << toSend << std::endl << "]" << NC << std::endl << std::endl;
	if (ret == -1)
	{
		close(socket);
		return (-1);
	}
	else
		return (0);
}

long				Server::recv(long socket)
{
	char buffer[2048] = {0};
	int ret;

	ret = ::recv(socket, buffer, 2047, 0);
	if (ret == 0 || ret == -1)
	{
		close(socket);
		if (!ret)
			Console::info("Connection on socket " + Utils::to_string(socket) + " was closed by client on server [" + _config.getHost() + ":" + Utils::to_string(_config.getPort()) + "]");
		else
			Console::info("Read error on socket " + Utils::to_string(socket) + ". Closing this connexion on server [" + _config.getHost() + ":" + Utils::to_string(_config.getPort()) + "]");
		return (-1);
	}
	_requests[socket] += std::string(buffer);
	std::cout << std::endl << CYAN << "------ Received request ------" << std::endl << "[" << std::endl << _requests[socket] << "]" << NC << std::endl << std::endl;
	return (0);
}

long				Server::accept(void)
{
	long	socket;

	socket = ::accept(_fd, NULL, NULL);
	fcntl(socket, F_SETFL, O_NONBLOCK);
	Console::info("Connexion received. Created non-blocking socket " + Utils::to_string(socket) + " for server [" + _config.getHost() + ":" + Utils::to_string(_config.getPort()) + "]");
	return (socket);
}


/*
** ------ GETTERS / SETTERS ------
*/
long				Server::getFD(void) const
{ return (this->_fd); }

ServerConfiguration	Server::getConfiguration() const
{ return (_config); }

void	Server::load(ServerConfiguration conf)
{ _config = conf; }



/*
** ------ PRIVATE HELPERS : HEADER HANDLERS ------
*/
void	Server::handleRequestHeaders(Request request, Response &response)
{
	DoubleString				headers = request.getHeaders();

	for (DoubleString::iterator it = headers.begin(); it != headers.end(); it++)
	{
		if (it->first == "Accept-Charset")
		{
			this->handleCharset(Utils::split(it->second, ","), response);
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		}
		else if (it->first == "Accept-Language")
		{
			this->handleLanguage(request, Utils::split(it->second, ","), response);
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		}
		else if (it->first == "Allow")
		{
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		}
		else if (it->first == "Authorization")
		{
			this->handleAuthorization(request, Utils::split(it->second, " "), response);
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		}
		else if (it->first == "Content-Language")
		{
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		}
		else if (it->first == "Content-Length")
		{
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		}
		else if (it->first == "Content-Location")
		{
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		}
		else if (it->first == "Content-Type")
		{
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		}
		else if (it->first == "Date")
		{
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		}
		else if (it->first == "Host")
		{
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		}
		else if (it->first == "Last-Modified")
		{
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		}
		else if (it->first == "Location")
		{
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		}
		else if (it->first == "Referer")
		{
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		}
		else if (it->first == "Retry-After")
		{
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		}
		else if (it->first == "Server")
		{
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		}
		else if (it->first == "Transfer-Encoding")
		{
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		}
		else if (it->first == "User-Agent")
		{
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		}
		else if (it->first == "WWW-Authenticate")
		{
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		}
	}
}

void	Server::handleCharset(std::vector<std::string> vecCharset, Response &response)
{
	for (std::vector<std::string>::iterator it = vecCharset.begin(); it != vecCharset.end(); it++)
		*it = Utils::to_lower(Utils::trim(*it));

	if (vecCharset.size() == 1 && (vecCharset[0] == "*" || vecCharset[0] == "utf-8"))
		return ;

	for (std::vector<std::string>::iterator it = vecCharset.begin(); it != vecCharset.end(); it++)
	{
		if (*it == "*" || *it == "utf-8")
			return ;
	}
	
	Console::error("Charset not found !");
	response.setStatus(406);
}

void	Server::handleLanguage(Request request, std::vector<std::string> vecLang, Response &response)
{
	for (std::vector<std::string>::iterator it = vecLang.begin(); it != vecLang.end(); it++)
		*it = Utils::to_lower(Utils::trim(*it));

	if (vecLang.size() == 1 && vecLang[0] != "*")
	{
		if (Utils::file_exist(request.getURL() + "." + vecLang[0]))
		{
			response.setHeader("Content-Language", vecLang[0]);
			return ;
		}
	}
	else if (vecLang.size() > 1)
	{
		std::map<float, std::string>	mapLang;
		for (std::vector<std::string>::iterator it = vecLang.begin(); it != vecLang.end(); it++)
		{
			std::vector<std::string> tmpVec = Utils::split(*it, ";");

			float coef;
			std::string	lang = Utils::trim(tmpVec[0]);

			if (tmpVec.size() == 2)
			{
				tmpVec[1] = Utils::trim(tmpVec[1]);
				tmpVec[1] = tmpVec[1].substr(2, tmpVec[1].length() - 2);
				coef = std::stof(tmpVec[1]);
			}
			else
				coef = 1.f;
			mapLang[coef] = lang;
		}

		for (std::map<float, std::string>::reverse_iterator it = mapLang.rbegin(); it != mapLang.rend(); it++)
		{
			if (Utils::file_exist(request.getURL() + "." + it->second))
			{
				response.setHeader("Content-Language", it->second);
				return ;
			}
		}
	}
}

void	Server::handleAuthorization(Request request, std::vector<std::string> vecData, Response &response)
{
	std::string					type;
	std::string					id;
	std::string					password;
	std::string					decoded;
	std::vector<std::string>	vecCredentials;

	(void)response;
	(void)request;

	if (vecData.size() <= 1)
	{
		Console::error("Wrong format for authorization header");
		return ;
	}

	type = Utils::to_lower(vecData[0]);
	decoded = Utils::base64_decode(vecData[1]);
	vecCredentials = Utils::split(decoded, ":");
	id = vecCredentials[0];
	password = vecCredentials[1];
	Console::info("Try to connect with : " + id + ":" + password);

}

/*
** ------ PRIVATE HELPERS : URL HANDLERS ------
*/
void		Server::handleRequestURL(Request request, Response &response)
{
	Route	route;

	(void)response;
	route = this->findCorrespondingRoute(request.getURL());
	if (this->requestRequireCGI(request, route))
	{
		std::cout << "CGI required = yes" << std::endl;
		if (route.getCGIBinary().empty())
			Console::error("Error no CGI configured !");
		else
		{
			CGI	cgi;
			this->generateMetaVariables(cgi, request, route);
			cgi.setBinary("./a.out");//route.getCGIBinary());
			cgi.execute();

		}
	}
	else
		std::cout << "CGI required = no" << std::endl;
}

Route		Server::findCorrespondingRoute(std::string URL)
{
	std::vector<Route>				routes = _config.getRoutes();
	std::vector<Route>::iterator	it = routes.begin();
	std::vector<std::string>		vecURLComponent = Utils::split(URL, "/");
	std::vector<std::string>		vecRouteComponent;
	std::map<int, Route>			matchedRoutes;
	int								priority_level = 0;

	while (it != routes.end())
	{
		if (it->getRoute() == "/")
			matchedRoutes[priority_level] = *it;

		vecRouteComponent = Utils::split(it->getRoute(), "/");
		priority_level = Utils::getCommonValues(vecURLComponent, vecRouteComponent);

		if (matchedRoutes.find(priority_level) == matchedRoutes.end())
			matchedRoutes[priority_level] = *it;
		it++;
	}

	std::map<int, Route>::reverse_iterator priority_route_it = matchedRoutes.rbegin();
	return (priority_route_it->second);
}

std::string	Server::getLocalPath(Request request, Route route)
{
	struct stat	path_stat;
	std::string	localPath = request.getURL();

	if (route.getRoute() != "/")
		localPath.erase(localPath.find(route.getRoute()), route.getRoute().length());

	// Si y a pas de "/" séparateur on rajoute
	if (route.getLocalURL()[route.getLocalURL().length() - 1] != '/' && localPath[0] != '/')
		localPath = "/" + localPath;

	localPath = route.getLocalURL() + localPath;

	stat(localPath.c_str(), &path_stat);
	if (S_ISDIR(path_stat.st_mode))
	{
		if (localPath[localPath.length() - 1] != '/')
			localPath += "/";
		localPath += route.getIndex();
	}
	std::cout << "LOCAL PATH = " + localPath << std::endl;
	return (localPath);
}



/*
** ------ PRIVATE HELPERS : CGI HANDLERS ------
*/
bool		Server::requestRequireCGI(Request request, Route route)
{
	std::vector<std::string>	vecExtensions = route.getCGIExtensions();
	std::string					localPath = this->getLocalPath(request, route);
	
	// If the extension of the request file is a cgi extension
	if (Utils::find_in_vector(vecExtensions, "." + Utils::get_file_extension(localPath)))
		return (true);
	return (false);
}

void		Server::generateMetaVariables(CGI &cgi, Request &request, Route &route)
{
	cgi.addMetaVariable("GATEWAY_INTERFACE", "CGI/1.1");
	cgi.addMetaVariable("SERVER_NAME", this->_config.getName());
	cgi.addMetaVariable("SERVER_SOFTWARE", "webserv/1.0");
	cgi.addMetaVariable("SERVER_PROTOCOL", "HTTP/1.1");											// PEUT ETRE 1.0
	cgi.addMetaVariable("SERVER_PORT", Utils::to_string(this->_config.getPort()));
	cgi.addMetaVariable("REQUEST_METHOD", request.getMethod());
	cgi.addMetaVariable("PATH_INFO", "test");													// A COMPLETER
	cgi.addMetaVariable("PATH_TRANSLATED", "test");												// A COMPLETER
	cgi.addMetaVariable("SCRIPT_NAME", route.getCGIBinary());
	cgi.addMetaVariable("DOCUMENT_ROOT", "test");												// A COMPLETER
	cgi.addMetaVariable("QUERY_STRING", "test");												// A COMPLETER
	cgi.addMetaVariable("REMOTE_ADDR", request.getIP());										// A COMPLETER
	cgi.addMetaVariable("AUTH_TYPE", (route.requireAuth() ? "BASIC" : ""));
	cgi.addMetaVariable("REMOTE_USER", "user");
	if (request.getHeaders().find("Content-Type") != request.getHeaders().end())
		cgi.addMetaVariable("CONTENT_TYPE", request.getHeaders().find("Content-Type")->second);
	cgi.addMetaVariable("CONTENT_LENGTH", Utils::to_string(request.getBody().length()));
	cgi.addMetaVariable("HTTP_ACCEPT", request.getHeaders()["HTTP_ACCEPT"]);
	cgi.addMetaVariable("HTTP_USER_AGENT", request.getHeaders()["User-Agent"]);
	cgi.addMetaVariable("HTTP_REFERER", request.getHeaders()["Referer"]);
}
