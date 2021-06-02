/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: user42 <user42@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/05/22 11:06:00 by user42            #+#    #+#             */
/*   Updated: 2021/06/02 14:04:42 by user42           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server/Server.hpp"

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
	if (!x._virtualHosts.empty())
		_virtualHosts = x._virtualHosts;
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
	Route					route = findCorrespondingRoute(request.getURL(), virtualHost);
	Console::info("Transmitting request to virtual host " + virtualHost.getName() + " with server_root " + virtualHost.getServerRoot());
	
	if ((route.requireAuth() && !request.hasAuthHeader()) || (route.requireAuth() && request.hasAuthHeader() && !this->credentialsMatch(request.getHeaders()["Authorization"], route.getUserFile())))
		this->handleUnauthorizedRequests(response, virtualHost);
	else if (!this->requestIsValid(response, request, route))
		this->handleRequestErrors(response, route, virtualHost);
	else if (requestRequireRedirection(request, route))
		this->generateRedirection(response, virtualHost);
	else
		this->setResponseBody(response, request, route, virtualHost);
<<<<<<< HEAD
	this->setResponseHeaders(response, route, request);
	std::string toSend = response.build(virtualHost.getErrors());
=======
	
	this->setResponseHeaders(response, request, route);
	std::string toSend = response.build();

>>>>>>> 805e2cc117c2ae13987f61f040e06b64279dfec8
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
<<<<<<< HEAD
void				Server::setResponseHeaders(Response & response, Route & route, Request & request)
=======
void				Server::setResponseHeaders(Response & response, Request request, Route & route)
>>>>>>> 805e2cc117c2ae13987f61f040e06b64279dfec8
{
	response.setHeader("Content-Length", Utils::to_string(response.getBody().length()));
	response.setHeader("Content-Location", request.getURL());
	response.setHeader("Server", "webserv/1.0.0");
	if (!route.getRouteLang().empty())
		response.setHeader("Content-Language", route.getFormattedLang());
	if (response.getStatus() == 301)
		response.setHeader("Location", request.getURL() + "/");
}

/*
** ------ PRIVATE HELPERS : RESPONSE BODY HANDLERS ------
*/
void				Server::setResponseBody(Response & response, Request const & request, Route & route, ServerConfiguration & virtualHost)
{
	std::string		targetPath = getLocalPath(request, route);

	if (request.getMethod() == "PUT")
		this->handlePUTRequest(request, response, targetPath, virtualHost);
	else if (request.getMethod() == "DELETE")
		this->handleDELETERequest(response, targetPath, virtualHost);
	else if (request.getMethod() == "POST")
		this->handlePOSTRequest(response, request, route, virtualHost);
	else if (request.getMethod() == "GET")
		this->handleGETRequest(response, request, route, virtualHost);
}

void				Server::handlePUTRequest(Request const & request, Response & response, std::string const & targetPath, ServerConfiguration & virtualHost)
{
	std::ofstream file;
	if (Utils::isRegularFile(targetPath))
	{
		file.open(targetPath.c_str());
		file << request.getBody();
		file.close();
		response.setStatus(204);
	}
	else if (Utils::isDirectory(targetPath))
	{
		response.setStatus(409);
		response.setBody(virtualHost.getErrorPage(409));
	}
	else
	{
		file.open(targetPath.c_str(), std::ofstream::out | std::ofstream::trunc);
		if (!(file.is_open() && file.good()))
		{
			response.setStatus(403);
			response.setBody(virtualHost.getErrorPage(403));
		}
		else
		{
			file << request.getBody();
			response.setStatus(201);
		}
	}
}

void				Server::handleDELETERequest(Response & response, std::string const & targetPath, ServerConfiguration & virtualHost)
{
	if (Utils::isRegularFile(targetPath))
	{
		if (remove(targetPath.c_str()) == 0)
			response.setStatus(204);
		else
		{
			response.setStatus(403);
			response.setBody(virtualHost.getErrorPage(403));
		}
	}
	else if (Utils::isDirectory(targetPath))
	{
		response.setStatus(403);
		response.setBody(virtualHost.getErrorPage(403));
	}
	else
	{
		response.setStatus(404);
		response.setBody(virtualHost.getErrorPage(404));
	}
}

// Trying to serve a directory with POST returns 403 ; trying to serve static content with POST returns 405
void				Server::handlePOSTRequest(Response & response, Request const & request, Route & route, ServerConfiguration & virtualHost)
{
	std::string		targetPath = getLocalPath(request, route);

	if (Utils::isDirectory(targetPath))
	{
		response.setStatus(403);
		response.setBody(virtualHost.getErrorPage(403));
	}
	else if (this->requestRequireCGI(request, route))
	{
		std::string 	output;
		DoubleString	CGIHeaders;
		std::string		CGIBody;
	
		output = this->execCGI(request, route, virtualHost);

		// SPLIT HEADERS AND BODY FROM CGI RETURN
		std::istringstream			origStream(output);
		std::string					curLine;
		bool						inHeader = true;
		std::string					body;

		while (std::getline(origStream, curLine))
		{
			Console::error(Utils::to_string(inHeader));
			if (curLine == "\r")
				inHeader = false;
			else
			{
				if (inHeader && Utils::split(curLine, ":").size() == 2)
					response.setHeader(Utils::split(curLine, ":")[0], Utils::split(curLine, ":")[1]);
				else if (!inHeader)
					body += curLine + "\r\n";
			}
			Console::info("  => '" + curLine + "' ");
			response.setBody(body);
		}
	}
	else
	{
		response.setStatus(405);
		response.setBody(virtualHost.getErrorPage(405));
	}
}

void				Server::handleGETRequest(Response & response, Request const & request, Route & route, ServerConfiguration & virtualHost)
{
	std::string		body;
	std::string		targetPath = getLocalPath(request, route);
	std::string		lastModified = Utils::getLastModified(targetPath);

	response.setHeader("Last-Modified", lastModified);
	if (Utils::isDirectory(targetPath))
	{
		std::string		indexPath = (targetPath[targetPath.size() - 1] == '/') ? targetPath + route.getIndex() : targetPath + "/" + route.getIndex();
		if (Utils::pathExists(indexPath) && Utils::isRegularFile(indexPath) && Utils::canOpenFile(indexPath))
			body = Utils::getFileContent(indexPath);
		else if (route.autoIndex())
		{
			response.setHeader("Last-Modified", "");
			AutoIndex autoindex(request.getURL(), targetPath);
			autoindex.createIndex();
			body = autoindex.getIndex();
		}
	}
	else if (this->requestRequireCGI(request, route))
	{
		std::string 	output;
		DoubleString	CGIHeaders;
		std::string		CGIBody;
	
		output = this->execCGI(request, route, virtualHost);

		// SPLIT HEADERS AND BODY FROM CGI RETURN
		std::istringstream			origStream(output);
		std::string					curLine;
		bool						inHeader = true;
		std::string					body;

		while (std::getline(origStream, curLine))
		{
			Console::error(Utils::to_string(inHeader));
			if (curLine == "\r")
				inHeader = false;
			else
			{
				if (inHeader && Utils::split(curLine, ":").size() == 2)
					response.setHeader(Utils::split(curLine, ":")[0], Utils::split(curLine, ":")[1]);
				else if (!inHeader)
					body += curLine + "\r\n";
			}
			Console::info("  => '" + curLine + "' ");
			response.setBody(body);
		}
	}
	else
	{
		if (Utils::pathExists(targetPath) && Utils::isRegularFile(targetPath) && Utils::canOpenFile(targetPath))
			body = Utils::getFileContent(targetPath);
	}
	response.setBody(body);
}


/*
** ------ PRIVATE HELPERS : HEADER HANDLERS ------
*/

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
			if (request.getMethod() == "POST" || request.getMethod() == "post")
				cgi.setInput(request.getBody());
			this->generateMetaVariables(cgi, request, route, virtualHost);
			cgi.convertHeadersToMetaVariables(request);
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
	
	if (Utils::find_in_vector(vecExtensions, "." + Utils::get_file_extension(localPath)))
		return (true);
	return (false);
}

void		Server::generateMetaVariables(CGI &cgi, Request &request, Route &route, ServerConfiguration & virtualHost)
{
	DoubleString	headers = request.getHeaders();
	std::string		targetPath = getLocalPath(request, route);

	cgi.addMetaVariable("GATEWAY_INTERFACE", "CGI/1.1");
	cgi.addMetaVariable("SERVER_NAME", virtualHost.getName());
	cgi.addMetaVariable("SERVER_SOFTWARE", "webserv/1.0");
	cgi.addMetaVariable("SERVER_PROTOCOL", "HTTP/1.1");
	cgi.addMetaVariable("SERVER_PORT", Utils::to_string(virtualHost.getPort()));
	cgi.addMetaVariable("REQUEST_METHOD", request.getMethod());
	cgi.addMetaVariable("PATH_INFO", "");												// A COMPLETER
	cgi.addMetaVariable("PATH_TRANSLATED", targetPath);
	cgi.addMetaVariable("SCRIPT_NAME", route.getCGIBinary());
	cgi.addMetaVariable("DOCUMENT_ROOT", route.getLocalURL());									// A Vérifier
	cgi.addMetaVariable("QUERY_STRING", request.getQueryString());
	cgi.addMetaVariable("REMOTE_ADDR", "127.0.0.1");											// A COMPLETER
	cgi.addMetaVariable("AUTH_TYPE", (route.requireAuth() ? "BASIC" : ""));
	cgi.addMetaVariable("REMOTE_USER", "user");
	cgi.addMetaVariable("CONTENT_TYPE", "text/html");
	if (headers.find("Content-Type") != headers.end())
	{
		DoubleString::iterator it = headers.find("Content-Type");
		cgi.addMetaVariable("CONTENT_TYPE", it->second);
	}
	cgi.addMetaVariable("CONTENT_LENGTH", Utils::to_string(request.getBody().length()));
	if (request.getMethod() == "get" || request.getMethod() == "GET")
		cgi.addMetaVariable("CONTENT_LENGTH", "0");
	cgi.addMetaVariable("REDIRECT_STATUS", "200");
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

void		Server::generateRedirection(Response &response, ServerConfiguration & virtualHost)
{
	response.setStatus(301);
	response.setBody(virtualHost.getErrorPage(301));
}


/*
** ------ PRIVATE HELPERS : ERRORS HANDLERS ------
*/
bool		Server::requestIsValid(Response & response, Request request, Route & route)
{
	std::string		targetPath = getLocalPath(request, route);
	if (request.getValidity() != 0)
	{
		response.setStatus(request.getValidity());
		return (false);
	}
	if (this->check403(request, route))
	{
		response.setStatus(403);
		return (false);
	}
	else if ((request.getMethod() == "GET" || request.getMethod() == "POST") && !Utils::isDirectory(targetPath) && !Utils::isRegularFile(targetPath))
	{
		response.setStatus(404);
		return (false);
	}
	else if (!this->isMethodAccepted(request, route))
	{
		response.setStatus(405);
		return (false);
	}
	else if (!this->isCharsetValid(request))
	{
		response.setStatus(406);
		return (false);
	}
	return (true);
}

void		Server::handleRequestErrors(Response &response, Route & route, ServerConfiguration & virtualHost)
{
	std::vector<std::string>	vecMethods = route.getAcceptedMethods();

	for (std::vector<std::string>::iterator it = vecMethods.begin(); it != vecMethods.end(); it++)
		*it = Utils::to_upper(*it);
	if (response.getStatus() == 405)
		response.setHeader("Allow", Utils::join(vecMethods));
	response.setBody(virtualHost.getErrorPage(response.getStatus()));
}

bool		Server::isMethodAccepted(Request request, Route & route)
{ return (route.acceptMethod(request.getMethod())); }


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
	else if (Utils::pathExists(targetPath) && Utils::isRegularFile(targetPath) && !Utils::canOpenFile(targetPath))
		return (true);
	else
		return (false);
}
