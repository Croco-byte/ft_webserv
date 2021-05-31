/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: user42 <user42@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/05/22 11:06:00 by user42            #+#    #+#             */
/*   Updated: 2021/05/30 15:37:26 by user42           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server/Server.hpp"

/*
** ------ CONSTRUCTORS / DESTRUCTOR ------
*/
Server::Server()
 : _error_code(0)
{}

Server::Server(const Server &x)
{
	_fd = x._fd;
	_addr = x._addr;
	_requests = x._requests;
	if (!x._virtualHosts.empty())
		_virtualHosts = x._virtualHosts;
	_error_code = x._error_code;
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
	_addr.sin_port = htons(_virtualHosts[0].getPort());
	_addr.sin_addr.s_addr = inet_addr((_virtualHosts[0].getHost().c_str()));
	rc = bind(_fd, (struct sockaddr *)&_addr, sizeof(_addr));
	if (rc < 0)
	{
		Console::error("Couldn't create server [" + _virtualHosts[0].getHost() + ":" + Utils::to_string(_virtualHosts[0].getPort()) + "]" + " : bind() call failed");
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
	Request					request;
	Response				response;

	std::string request_str = _requests[socket];
	_requests.erase(socket);
	request.load(request_str);

	ServerConfiguration &	virtualHost = findVirtualHost(request.getHeaders());
	Console::info("Transmitting request to virtual host " + virtualHost.getName() + " with server_root " + virtualHost.getServerRoot());
	Route					route = findCorrespondingRoute(request.getURL(), virtualHost);

	this->handleRequestHeaders(request, response);
	
	if (route.requireAuth() && !request.hasAuthHeader())
		this->handleUnauthorizedRequests(response, virtualHost);
	else if (route.requireAuth() && request.hasAuthHeader() && !this->credentialsMatch(request.getHeaders()["Authorization"], route.getUserFile()))
		this->handleUnauthorizedRequests(response, virtualHost);
	else if (!this->requestIsValid(request, route))
		this->handleRequestErrors(request, response, route, virtualHost);
	else if (requestRequireRedirection(request, route))
		this->generateRedirection(request, response, route);
	else
		this->setResponseBody(response, request, route, virtualHost);
	
	this->setResponseHeaders(response);
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
			Console::info("Connection on socket " + Utils::to_string(socket) + " was closed by client on server [" + _virtualHosts[0].getHost() + ":" + Utils::to_string(_virtualHosts[0].getPort()) + "]");
		else
			Console::info("Read error on socket " + Utils::to_string(socket) + ". Closing this connexion on server [" + _virtualHosts[0].getHost() + ":" + Utils::to_string(_virtualHosts[0].getPort()) + "]");
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
	Console::info("Connexion received. Created non-blocking socket " + Utils::to_string(socket) + " for server [" + _virtualHosts[0].getHost() + ":" + Utils::to_string(_virtualHosts[0].getPort()) + "]");
	return (socket);
}



/*
** ------ GETTERS / SETTERS ------
*/
long				Server::getFD(void) const
{ return (this->_fd); }

std::vector<ServerConfiguration> &		Server::getVirtualHosts(void)
{ return (_virtualHosts); }

ServerConfiguration &					Server::getVHConfig(std::string const & server_name)
{
	for (std::vector<ServerConfiguration>::iterator it = _virtualHosts.begin(); it != _virtualHosts.end(); it++)
	{
		if ((*it).getName() == server_name)
			return (*it);
	}
	return (getDefaultVHConfig());
}


ServerConfiguration &					Server::getDefaultVHConfig(void)
{ return (_virtualHosts[0]); }

ServerConfiguration &					Server::findVirtualHost(DoubleString const & headers)
{
	for (DoubleString::const_iterator it = headers.begin(); it != headers.end(); it++)
	{
		if (it->first == "Host")
		{
			for (std::vector<ServerConfiguration>::iterator it2 = _virtualHosts.begin(); it2 != _virtualHosts.end(); it2++)
			{
				if (it->second == (*it2).getName() + ":" + Utils::to_string((*it2).getPort()) || it->second == (*it2).getName())
					return (*it2);
			}
		}
	}
	return (getDefaultVHConfig());
}


void	Server::addVirtualHost(ServerConfiguration conf)
{
	if (!_virtualHosts.empty())
		conf.setDefault(false);
	if ((conf.getName()).empty())
		conf.setName(conf.getHost());
	_virtualHosts.push_back(conf);
}


/*
** ------ PRIVATE HELPERS : RESPONSE HEADERS HANDLERS ------
*/
void				Server::setResponseHeaders(Response & response)
{
	response.setHeader("Content-Length", Utils::to_string(response.getBody().length()));
}

/*
** ------ PRIVATE HELPERS : RESPONSE BODY HANDLERS ------
*/
void				Server::setResponseBody(Response & response, Request const & request, Route & route, ServerConfiguration & virtualHost)
{
	std::string		body;
	std::string		targetPath = getLocalPath(request, route);
	std::string		lastModified = Utils::getLastModified(targetPath);

	if (Utils::isDirectory(targetPath))
	{
		std::string		indexPath = (targetPath[targetPath.size() - 1] == '/') ? targetPath + route.getIndex() : targetPath + "/" + route.getIndex();
		if (Utils::pathExists(indexPath) && Utils::isRegularFile(indexPath) && Utils::canOpenFile(indexPath))
		{
			response.setHeader("Last-Modified", lastModified);
			body = Utils::getFileContent(indexPath);
		}
		else if (route.autoIndex())
		{
			AutoIndex autoindex(request.getURL(), targetPath);
			autoindex.createIndex();
			body = autoindex.getIndex();
		}
		else
			body = virtualHost.getErrorPage(403);
	}
	else if (this->requestRequireCGI(request, route))
	{
		response.setHeader("Last-Modified", lastModified);
		body = this->execCGI(request, route, virtualHost);
	}
	else
	{
		if (Utils::pathExists(targetPath) && Utils::isRegularFile(targetPath) && Utils::canOpenFile(targetPath))
		{
			response.setHeader("Last-Modified", lastModified);
			body = Utils::getFileContent(targetPath);
		}
		else if (Utils::pathExists(targetPath) && Utils::isRegularFile(targetPath) && !Utils::canOpenFile(targetPath))
		{
			// ERROR 403
		}
	}
	response.setBody(body);
}


/*
** ------ PRIVATE HELPERS : HEADER HANDLERS ------
*/
void				Server::handleRequestHeaders(Request request, Response &response)
{
	DoubleString				headers = request.getHeaders();

	for (DoubleString::iterator it = headers.begin(); it != headers.end(); it++)
	{
		// if (it->first == "Accept-Charset")
		// 	this->handleCharset(Utils::split(it->second, ","), response);
		if (it->first == "Accept-Language")
			this->handleLanguage(request, Utils::split(it->second, ","), response);
	}
}

bool				Server::isCharsetValid(Request request)
{
	std::vector<std::string>	vecCharset;
	DoubleString				headers = request.getHeaders();
	DoubleString::iterator		find_it;

	find_it = headers.find("Accept-Charset");
	if (find_it == headers.end())
		return (true);
	vecCharset = Utils::split(find_it->second, ",");
	for (std::vector<std::string>::iterator it = vecCharset.begin(); it != vecCharset.end(); it++)
		*it = Utils::to_lower(Utils::trim(*it));

	if (vecCharset.size() == 1 && (vecCharset[0] == "*" || vecCharset[0] == "utf-8"))
		return (true);

	for (std::vector<std::string>::iterator it = vecCharset.begin(); it != vecCharset.end(); it++)
	{
		if (*it == "*" || *it == "utf-8")
			return (true);
	}
	return (false);
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

Route		Server::findCorrespondingRoute(std::string URL, ServerConfiguration & virtualHost)
{
	std::vector<Route>				routes = virtualHost.getRoutes();
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

std::string		Server::execCGI(Request request, Route & route, ServerConfiguration & virtualHost)
{
	std::string	targetPath;

	if (this->requestRequireCGI(request, route))
	{
		targetPath = getLocalPath(request, route);
		if (route.getCGIBinary().empty())
			Console::error("Error: no CGI configured !");
		else
		{
			CGI	cgi;
			this->generateMetaVariables(cgi, request, route, virtualHost);
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

void		Server::generateMetaVariables(CGI &cgi, Request &request, Route &route, ServerConfiguration & virtualHost)
{
	DoubleString	headers = request.getHeaders();
	std::string		targetPath = getLocalPath(request, route);

	request.print();
	cgi.addMetaVariable("GATEWAY_INTERFACE", "CGI/1.1");
	cgi.addMetaVariable("SERVER_NAME", virtualHost.getName());
	cgi.addMetaVariable("SERVER_SOFTWARE", "webserv/1.0");
	cgi.addMetaVariable("SERVER_PROTOCOL", "HTTP/1.1");
	cgi.addMetaVariable("SERVER_PORT", Utils::to_string(virtualHost.getPort()));
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
bool		Server::requestRequireRedirection(Request request, Route & route)
{
	std::string		targetPath = getLocalPath(request, route);

	if (Utils::isDirectory(targetPath) && request.getURL()[request.getURL().length() - 1] != '/')
	{
		Console::error("Require redirection on " + request.getURL() + "  => " + targetPath);
		return (true);
	}
	return (false);
}

void		Server::generateRedirection(Request request, Response &response, Route & route)
{
	std::string		targetPath = getLocalPath(request, route);

	response.setStatus(301);
	response.setHeader("Location", request.getURL() + "/");
}


/*
** ------ PRIVATE HELPERS : ERRORS HANDLERS ------
*/
bool		Server::requestIsValid(Request request, Route & route)
{
	std::string		targetPath = getLocalPath(request, route);

	if (this->check403(request, route))
	{
		_error_code = 403;
		return (false);
	}
	else if (!Utils::isDirectory(targetPath) && !Utils::isRegularFile(targetPath))
	{
		_error_code = 404;
		return (false);
	}
	else if (!this->isMethodAccepted(request, route))
	{
		_error_code = 405;
		return (false);
	}
	else if (!this->isCharsetValid(request))
	{
		_error_code = 406;
		return (false);
	}
	return (true);
}

void		Server::handleRequestErrors(Request request, Response &response, Route & route, ServerConfiguration & virtualHost)
{
	std::string		targetPath = getLocalPath(request, route);

	if (_error_code == 405)
		response.setHeader("Allow", Utils::join(route.getAcceptedMethods()));
	response.setStatus(_error_code);
	response.setBody(virtualHost.getErrorPage(_error_code));
}

bool		Server::isMethodAccepted(Request request, Route & route)
{
	return (route.acceptMethod(request.getMethod()));
}


/*
** ------ PRIVATE HELPERS : AUTHORIZATION HANDLERS ------
*/
void		Server::handleUnauthorizedRequests(Response & response, ServerConfiguration & virtualHost)
{
	response.setStatus(401);
	response.setHeader("www-authenticate","Basic realm=\"HTTP auth required\"");
	response.setBody(virtualHost.getErrorPage(401));
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

bool		Server::check403(Request request, Route route)
{
	std::string		targetPath = getLocalPath(request, route);

	if (Utils::isDirectory(targetPath))
	{
		std::string		indexPath = (targetPath[targetPath.size() - 1] == '/') ? targetPath + route.getIndex() : targetPath + "/" + route.getIndex();
		if (Utils::pathExists(indexPath) && Utils::isRegularFile(indexPath) && Utils::canOpenFile(indexPath))
			return (false);
		else if (route.autoIndex())
			return (false);
		else
			return (true);
	}
	else
		return (false);
}
