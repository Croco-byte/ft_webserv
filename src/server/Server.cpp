/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: user42 <user42@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/05/22 11:06:00 by user42            #+#    #+#             */
/*   Updated: 2021/05/28 15:01:56 by user42           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server/Server.hpp"

/*s
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

	Route		route = findCorrespondingRoute(request.getURL());

	this->handleRequestHeaders(request, response);
	
	if (route.requireAuth() && !request.hasAuthHeader())
		this->handleUnauthorizedRequests(response);
	else if (route.requireAuth() && request.hasAuthHeader() && !this->credentialsMatch(request.getHeaders()["Authorization"], route.getUserFile()))
		this->handleUnauthorizedRequests(response);
	else if (!this->requestIsValid(request))
		this->handleRequestErrors(request, response);
	else if (requestRequireRedirection(request))
		this->generateRedirection(request, response);
	else
		response.setBody(this->generateResponseBody(request));
	
	response.setHeader("Content-Length", Utils::to_string(response.getBody().length()));
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
** ------ GET ERROR PAGES ------
*/
std::string				Server::get404Page(void)
{ return (Utils::getFileContent(_config.getErrorPageLocation(404))); }

std::string				Server::get401Page(void)
{ return (Utils::getFileContent(_config.getErrorPageLocation(401))); }



/*
** ------ PRIVATE HELPERS : RESPONSE BODY HANDLERS ------
*/
std::string			Server::generateResponseBody(Request const & request)
{
	Route			route = findCorrespondingRoute(request.getURL());
	std::string		targetPath = getLocalPath(request, route);

	if (this->requestRequireCGI(request, route))
		return (this->execCGI(request));

	Console::info("Target path = " + targetPath);
	if (!Utils::pathExists(targetPath))
		return (get404Page());
	if (Utils::isDirectory(targetPath))
	{
		std::string		indexPath = (targetPath[targetPath.size() - 1] == '/') ? targetPath + route.getIndex() : targetPath + "/" + route.getIndex();
		if (Utils::pathExists(indexPath) && Utils::isRegularFile(indexPath))
			return (Utils::getFileContent(indexPath));										// There is an index file for this directory : we display it.
		if (route.autoIndex())																// Else, if directory listing is enabled, we display listing.
		{
			AutoIndex autoindex(request.getURL(), targetPath);
			autoindex.createIndex();
			return (autoindex.getIndex());
		}
		else																				// Else, display a 403 Forbidden (TODO : replace 404 by 403 !)
			return (get404Page());
	}
	if (!Utils::pathExists(targetPath) || !Utils::isRegularFile(targetPath))
			return (get404Page());
	return(Utils::getFileContent(targetPath));
}


/*
** ------ PRIVATE HELPERS : HEADER HANDLERS ------
*/
void				Server::handleRequestHeaders(Request request, Response &response)
{
	DoubleString				headers = request.getHeaders();

	for (DoubleString::iterator it = headers.begin(); it != headers.end(); it++)
	{
		if (it->first == "Accept-Charset")
			this->handleCharset(Utils::split(it->second, ","), response);
		else if (it->first == "Accept-Language")
			this->handleLanguage(request, Utils::split(it->second, ","), response);
/*		else if (it->first == "Allow")
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		else if (it->first == "Authorization")
			this->handleAuthorization(request, Utils::split(it->second, " "), response);
		else if (it->first == "Content-Language")
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		else if (it->first == "Content-Length")
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		else if (it->first == "Content-Location")
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		else if (it->first == "Content-Type")
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		else if (it->first == "Date")
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		else if (it->first == "Host")
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		else if (it->first == "Last-Modified")
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		else if (it->first == "Location")
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		else if (it->first == "Referer")
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		else if (it->first == "Retry-After")
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		else if (it->first == "Server")
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		else if (it->first == "Transfer-Encoding")
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		else if (it->first == "User-Agent")
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl;
		else if (it->first == "WWW-Authenticate")
			// std::cout << "Handle " << it->first << " : " << it->second << std::endl; */
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
		if (Utils::pathExists(request.getURL() + "." + vecLang[0]))
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
				coef = std::atof(tmpVec[1].c_str());
			}
			else
				coef = 1.f;
			mapLang[coef] = lang;
		}

		for (std::map<float, std::string>::reverse_iterator it = mapLang.rbegin(); it != mapLang.rend(); it++)
		{
			if (Utils::pathExists(request.getURL() + "." + it->second))
			{
				response.setHeader("Content-Language", it->second);
				return ;
			}
		}
	}
}


/*
** ------ PRIVATE HELPERS : URL HANDLERS ------
*/

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
	std::string	localPath = request.getURL();

	if (route.getRoute() != "/")
		localPath.erase(localPath.find(route.getRoute()), route.getRoute().length());

	if (route.getLocalURL()[route.getLocalURL().length() - 1] != '/' && localPath[0] != '/')
		localPath = "/" + localPath;
	else if (route.getLocalURL()[route.getLocalURL().length() - 1] == '/' && localPath[0] == '/')
		localPath = localPath.substr(1);

	localPath = route.getLocalURL() + localPath;
	return (localPath);
}



/*
** ------ PRIVATE HELPERS : CGI HANDLERS ------
*/

std::string		Server::execCGI(Request request)
{
	Route		route;
	std::string	targetPath;

	route = this->findCorrespondingRoute(request.getURL());
	if (this->requestRequireCGI(request, route))
	{
		targetPath = getLocalPath(request, route);
		if (route.getCGIBinary().empty())
			Console::error("Error: no CGI configured !");
		else
		{
			CGI	cgi;
			this->generateMetaVariables(cgi, request, route);
			cgi.setBinary(route.getCGIBinary());
			cgi.execute(targetPath);
			return (cgi.getOutput());
		}
	}
	std::cout << "CGI required = no" << std::endl;
	return ("");
}

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
	DoubleString	headers = request.getHeaders();
	std::string		targetPath = getLocalPath(request, route);

	request.print();
	cgi.addMetaVariable("GATEWAY_INTERFACE", "CGI/1.1");
	cgi.addMetaVariable("SERVER_NAME", this->_config.getName());
	cgi.addMetaVariable("SERVER_SOFTWARE", "webserv/1.0");
	cgi.addMetaVariable("SERVER_PROTOCOL", "HTTP/1.1");
	cgi.addMetaVariable("SERVER_PORT", Utils::to_string(this->_config.getPort()));
	cgi.addMetaVariable("REQUEST_METHOD", request.getMethod());
	// cgi.addMetaVariable("PATH_INFO", "test");												// A COMPLETER
	cgi.addMetaVariable("PATH_TRANSLATED", targetPath);
	cgi.addMetaVariable("SCRIPT_NAME", route.getCGIBinary());
	cgi.addMetaVariable("DOCUMENT_ROOT", route.getLocalURL());									// A Vérifier
	cgi.addMetaVariable("QUERY_STRING", request.getQueryString());
	cgi.addMetaVariable("REMOTE_ADDR", request.getIP());
	cgi.addMetaVariable("AUTH_TYPE", (route.requireAuth() ? "BASIC" : ""));
	cgi.addMetaVariable("REMOTE_USER", "user");
	if (headers.find("Content-Type") != headers.end())
	{
		DoubleString::iterator it = headers.find("Content-Type");
		cgi.addMetaVariable("CONTENT_TYPE", it->second);
	}
	cgi.addMetaVariable("CONTENT_LENGTH", Utils::to_string(request.getBody().length()));
	cgi.addMetaVariable("HTTP_ACCEPT", request.getHeaders()["HTTP_ACCEPT"]);
	cgi.addMetaVariable("HTTP_USER_AGENT", request.getHeaders()["User-Agent"]);
	cgi.addMetaVariable("HTTP_REFERER", request.getHeaders()["Referer"]);
}


/*
** ------ PRIVATE HELPERS : REDIRECTION HANDLERS ------
*/
bool		Server::requestRequireRedirection(Request request)
{
	Route			route = findCorrespondingRoute(request.getURL());
	std::string		targetPath = getLocalPath(request, route);

	if (Utils::isDirectory(targetPath) && targetPath[targetPath.length() - 1] != '/')
		return (true);
	return (false);
}

void		Server::generateRedirection(Request request, Response &response)
{
	Route			route = findCorrespondingRoute(request.getURL());
	std::string		targetPath = getLocalPath(request, route);

	response.setStatus(301);
	response.setHeader("Location", request.getURL() + "/");
}


/*
** ------ PRIVATE HELPERS : ERRORS HANDLERS ------
*/
bool		Server::requestIsValid(Request request)
{
	Route			route = findCorrespondingRoute(request.getURL());
	std::string		targetPath = getLocalPath(request, route);

	if (!Utils::isDirectory(targetPath) && !Utils::isRegularFile(targetPath))
		return (false);
	return (true);
}

void		Server::handleRequestErrors(Request request, Response &response)
{
	Route			route = findCorrespondingRoute(request.getURL());
	std::string		targetPath = getLocalPath(request, route);

	if (!Utils::isDirectory(targetPath) && !Utils::isRegularFile(targetPath))
	{
		Console::error("404");
		response.setStatus(404);
		Console::info(_config.getErrorPageLocation(404));
		Console::info(this->get404Page());
		response.setBody(this->get404Page());
		return ;
	}
}


/*
** ------ PRIVATE HELPERS : UNAUTHORIZED REQUESTS HANDLERS ------
*/
void		Server::handleUnauthorizedRequests(Response & response)
{
	response.setStatus(401);
	response.setHeader("www-authenticate","Basic realm=\"HTTP auth required\"");
	response.setBody(this->get401Page());
}

bool		Server::credentialsMatch(std::string const & requestAuthHeader, std::string const & userFile)
{
	std::string					fileContent = Utils::getFileContent(userFile);
	std::vector<std::string>	fileCreds;
	std::string					requestCreds = Utils::base64_decode((Utils::split_any(requestAuthHeader, " 	"))[1]);

	if (fileContent.empty())
	{
		Console::error("Couldn't get credentials from auth_basic_user_file " + userFile + " : file is empty, or doesn't exist.");
		return (false);
	}
	fileCreds = Utils::split(fileContent, "\n");
	for (std::vector<std::string>::iterator it = fileCreds.begin(); it != fileCreds.end(); it++)
	{
		if (*it == requestCreds)
			return (true);
	}
	return (false);
}
