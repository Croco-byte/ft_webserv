/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: user42 <user42@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/05/22 11:06:00 by user42            #+#    #+#             */
/*   Updated: 2021/06/10 10:42:59 by user42           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server/Server.hpp"

/*
** ------ CONSTRUCTORS / DESTRUCTOR ------
*/
Server::Server()
{
	initMIMEType();
}

Server::Server(const Server &x)
{
	_fd = x._fd;
	_addr = x._addr;
	_requests = x._requests;
	if (!x._virtualHosts.empty())
		_virtualHosts = x._virtualHosts;
	_extensionMIMEType = x._extensionMIMEType;
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

	if (_sent.find(socket) == _sent.end())
	{
		Console::info("Transmitting request to virtual host " + virtualHost.getName() + " with server_root " + virtualHost.getServerRoot());
		if ((route.requireAuth() && !request.hasAuthHeader()) || (route.requireAuth() && request.hasAuthHeader() && !this->credentialsMatch(request.getHeaders()["Authorization"], route.getUserFile())))
			this->handleUnauthorizedRequests(response, virtualHost);
		else if (!this->requestIsValid(response, request, route))
			this->handleRequestErrors(response, route, virtualHost);
		else if (requestRequireRedirection(request, route))
			this->generateRedirection(response, virtualHost);
		else
			this->setResponseBody(response, request, route, virtualHost);
		this->setResponseHeaders(response, route, request);
	}

	return (this->sendResponse(response, virtualHost, socket));
}

long				Server::recv(long socket)
{
	char buffer[RECV_SIZE] = {0};
	long ret;

	ret = ::recv(socket, buffer, RECV_SIZE - 1, 0);
	if (ret == 0 || ret == -1)
	{
		_requests.erase(socket);
		close(socket);
		if (!ret)
			Console::info("Connection on socket " + Utils::to_string(socket) + " was closed by client on server [" + _virtualHosts[0].getHost() + ":" + Utils::to_string(_virtualHosts[0].getPort()) + "]");
		else
			Console::info("Read error on socket " + Utils::to_string(socket) + ". Closing this connexion on server [" + _virtualHosts[0].getHost() + ":" + Utils::to_string(_virtualHosts[0].getPort()) + "]");
		return (-1);
	}
	_requests[socket] += std::string(buffer);

	ret = 0;
	size_t i = _requests[socket].find("\r\n\r\n");
	if (i != std::string::npos)															// If we finished to parse the headers...
	{
		if (_requests[socket].find("Transfer-Encoding: chunked") != std::string::npos)	// If there is a Transfer-Encoding: chunked, it has the priority. If we received the end of the chunked message, we finished receiving the request.
		{
			if (Utils::receivedLastChunk(_requests[socket]))
				ret = 0;
			else
				ret = 1;
		}
		else if (_requests[socket].find("Content-Length: ") != std::string::npos)		// If there is no Transfer-Encoding: chunked, let's check if there is a Content-Length.
		{
			size_t len = Utils::extractContentLength(_requests[socket]);
			if (_requests[socket].size() >= i + 4 + len)
			{
				_requests[socket] = _requests[socket].substr(0, i + 4 + len);			// Dropping any superfluous data after we reached Content-Length
				ret = 0;
			}
			else
				ret = 1;
		}
		else
			ret = 0;																	// We finished parsing the headers and there is no body : we received the whole request
	}
	else
		ret = 1;

	if (ret == 0 && _requests[socket].size() < 2000)
		std::cout << std::endl << CYAN << "------ Received request ------" << std::endl << "[" << std::endl << _requests[socket] << "]" << NC << std::endl << std::endl;
	else if (ret == 0 && _requests[socket].size() >= 2000)
		std::cout << std::endl << CYAN << "------ Received request ------" << std::endl << "[" << std::endl << _requests[socket].substr(0, 500) << "...]" << NC << std::endl << std::endl;
//	 else if (ret == 1)
//	 	std::cout << "[DEBUG] Received " << _requests[socket].size() << " bytes" << std::endl;
	return (ret);
}

long				Server::accept(void)
{
	long	socket;

	socket = ::accept(_fd, NULL, NULL);
	fcntl(socket, F_SETFL, O_NONBLOCK);
	Console::info("Connexion received. Created non-blocking socket " + Utils::to_string(socket) + " for server [" + _virtualHosts[0].getHost() + ":" + Utils::to_string(_virtualHosts[0].getPort()) + "]");
	return (socket);
}

void				Server::resetSocket(long socket)
{ _requests.erase(socket); }

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
void				Server::setResponseHeaders(Response & response, Route & route, Request & request)
{
	response.setHeader("Content-Length", Utils::to_string(response.getBody().length()));
	response.setHeader("Content-Location", request.getURL());
	response.setHeader("Server", "webserv/1.0.0");
	if (!route.getRouteLang().empty())
		response.setHeader("Content-Language", route.getFormattedLang());
	if (response.getStatus() == 301)
		response.setHeader("Location", request.getURL() + "/");
	if (response.getStatus() == 503)
		response.setHeader("Retry-After", "120");
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
	Console::info(targetPath);
	std::ofstream file;
	if (Utils::isRegularFile(targetPath))
	{
		file.open(targetPath.c_str());
		file << request.getBody();
		file.close();
		response.setHeader("Content-Type", request.getHeaders()["Content-Type"]);
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
			response.setHeader("Content-Type", request.getHeaders()["Content-Type"]);
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

void				Server::handlePOSTRequest(Response & response, Request const & request, Route & route, ServerConfiguration & virtualHost)
{
	std::string		targetPath = getLocalPath(request, route);
	if (Utils::isDirectory(targetPath) && !route.acceptMethod("post"))				// Moche, mais le testeur ne veut pas de 403 lorsque la destination est un dossier et que POST est une méthode acceptée de cette route...
	{
		response.setStatus(403);
		response.setBody(virtualHost.getErrorPage(403));
	}
	else if (this->requestRequireCGI(request, route) || (Utils::isDirectory(targetPath) && route.acceptMethod("post")))
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
			if (curLine == "\r")
				inHeader = false;
			else
			{
				if (inHeader && Utils::split(curLine, ":").size() == 2)
					response.setHeader(Utils::split(curLine, ":")[0], Utils::split(curLine, ":")[1]);
				else if (!inHeader)
					body += curLine;
			}
		}
		response.setBody(body);
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

	Console::info("Target path => " + targetPath);
	response.setHeader("Last-Modified", lastModified);
	if (Utils::isDirectory(targetPath))
	{
		std::string		indexPath = (targetPath[targetPath.size() - 1] == '/') ? targetPath + route.getIndex() : targetPath + "/" + route.getIndex();
		if (Utils::pathExists(indexPath) && Utils::isRegularFile(indexPath) && Utils::canOpenFile(indexPath))
		{
			body = Utils::getFileContent(indexPath);
			response.setHeader("Content-Type", _extensionMIMEType[Utils::get_file_extension(indexPath)]);
		}
		else if (route.autoIndex())
		{
			response.setHeader("Last-Modified", "");
			AutoIndex autoindex(request.getURL(), targetPath);
			autoindex.createIndex();
			body = autoindex.getIndex();
			response.setHeader("Content-Type", "text/html");
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

		while (std::getline(origStream, curLine))
		{
			if (curLine == "\r")
				inHeader = false;
			else
			{
				if (inHeader && Utils::split(curLine, ":").size() == 2)
					response.setHeader(Utils::split(curLine, ":")[0], Utils::split(curLine, ":")[1]);
				else if (!inHeader)
					body += curLine + "\r\n";
			}
		}
	}
	else
	{
		if (Utils::pathExists(targetPath) && Utils::isRegularFile(targetPath) && Utils::canOpenFile(targetPath))
		{
			body = Utils::getFileContent(targetPath);
			response.setHeader("Content-Type", _extensionMIMEType[Utils::get_file_extension(targetPath)]);
		}
	}
	response.setBody(body);
}


/*
** ------ PRIVATE HELPERS : SENDERS ------
*/
int				Server::sendChunkedResponse(Response & response, int body_length, int limit, ServerConfiguration & virtualHost, long socket)
{
	float						tmp = static_cast<float>(body_length) / limit;
	int							chunk_nb = std::ceil(tmp);
	std::string					full_body = response.getBody();
	std::vector<std::string>	vecChunks = Utils::divise_string(full_body, limit);

	Console::error("Require transfer-encoding : " + Utils::to_string(body_length) + " ; " + Utils::to_string(limit));
	Console::error("Require " + Utils::to_string(chunk_nb));
	response.setHeader("Transfer-Encoding", "chunked");
	response.setHeader("Content-Length", "");
	for (int i = 0; i <= chunk_nb; i++)
	{
		std::string toSend;

		if (i == chunk_nb)							// If we send the last chunk
			response.setBody("0\r\n\r\n");
		else										// Else we send content
			response.setBody(Utils::dec_to_hex(vecChunks[i].length()) + "\r\n" + vecChunks[i] + "\r\n");

		if (i == 0)									// If we send the first chunk we also send headers
			toSend = response.build(virtualHost.getErrors());
		else										// Else we send only content
			toSend = response.getBody();

		int ret = ::send(socket, toSend.c_str(), toSend.size(), 0);
		if (toSend.size() < 2000)
			std::cout << std::endl << GREEN << "------ Sent response ------" << std::endl << "[" << std::endl << toSend << std::endl << "]" << NC << std::endl << std::endl;
		else
			std::cout << std::endl << GREEN << "------ Sent response ------" << std::endl << "[" << std::endl << toSend.substr(0,1000) << std::endl << "...]" << NC << std::endl << std::endl;
		if (ret == -1)
		{
			close(socket);
			return (-1);
		}
	}
	return (0);
}

int					Server::sendResponse(Response & response, ServerConfiguration & virtualHost, long socket)
{
	if (_sent.find(socket) == _sent.end())
	{
		_sent[socket] = 0;
		_responses[socket] = response.build(virtualHost.getErrors());
	}
	
	std::string toSend = _responses[socket].substr(_sent[socket], SOCKET_MAX);
	int ret = ::send(socket, toSend.c_str(), toSend.size(), 0);
	
	if (ret == -1)
	{
		close(socket);
		_sent[socket] = 0;
		_responses.erase(socket);
		return (-1);
	}
	else
	{
		_sent[socket] += ret;
		if (_sent[socket] >= _responses[socket].size())
		{
			if (_responses[socket].size() < 2000)
				std::cout << std::endl << GREEN << "------ Sent response ------" << std::endl << "[" << std::endl << _responses[socket] << std::endl << "]" << NC << std::endl << std::endl;
			else
				std::cout << std::endl << GREEN << "------ Sent response ------" << std::endl << "[" << std::endl << _responses[socket].substr(0,1000) << std::endl << "...]" << NC << std::endl << std::endl;
			_sent.erase(socket);
			_responses.erase(socket);
			return (0);
		}
		else
			return (1);
	}
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
	cgi.addMetaVariable("REQUEST_URI", request.getURL());
	cgi.addMetaVariable("PATH_INFO", request.getURL());
	cgi.addMetaVariable("PATH_TRANSLATED", targetPath);
	cgi.addMetaVariable("SCRIPT_NAME", route.getCGIBinary());
	cgi.addMetaVariable("DOCUMENT_ROOT", route.getLocalURL());
	cgi.addMetaVariable("QUERY_STRING", request.getQueryString());
	cgi.addMetaVariable("REMOTE_ADDR", virtualHost.getHost());
	cgi.addMetaVariable("AUTH_TYPE", (route.requireAuth() ? "BASIC" : ""));
	cgi.addMetaVariable("REMOTE_USER", "user");
	cgi.addMetaVariable("CONTENT_TYPE", "text/html");
	if (headers.find("Content-Type") != headers.end())
	{
		DoubleString::iterator it = headers.find("Content-Type");
		cgi.addMetaVariable("CONTENT_TYPE", it->second);
	}
	cgi.addMetaVariable("CONTENT_LENGTH", Utils::to_string(request.getBody().length()));
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
		if (request.getMethod() == "POST" && route.acceptMethod("POST"))
			return (false);
		Console::info("Require redirection on " + request.getURL() + "  => " + targetPath);
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
	std::string		indexPath = (targetPath[targetPath.size() - 1] == '/') ? targetPath + route.getIndex() : targetPath + "/" + route.getIndex();
	unsigned int	limit = route.getMaxBodySize();
	
	Console::info(route.getRoute());
	if (request.getValidity() != 0)
	{
		response.setStatus(request.getValidity());
		return (false);
	}
	if (request.getBody().size() > limit)
	{
		response.setStatus(413);
		return (false);
	}
	if (this->check403(request, route))
	{
		response.setStatus(403);
		return (false);
	}
	else if ((request.getMethod() == "GET" || request.getMethod() == "POST") && !Utils::isDirectory(targetPath) && !Utils::isRegularFile(targetPath))
	{
		std::cout << "[DEBUG] Not a directory, not a file. Returning 404" << std::endl;
		response.setStatus(404);
		return (false);
	}
	else if (Utils::isDirectory(targetPath) && !route.autoIndex() && !Utils::pathExists(indexPath))
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
	else if (this->requestRequireCGI(request, route) && !Utils::canOpenFile(route.getCGIBinary()))
	{
		response.setStatus(500);
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
			return (false); //return (true); NGINX != ubuntu_tester
	}
	else if (Utils::pathExists(targetPath) && Utils::isRegularFile(targetPath) && !Utils::canOpenFile(targetPath))
		return (true);
	else
		return (false);
}

void		Server::initMIMEType()
{
	_extensionMIMEType["x3d"] = "application/vnd.hzn-3d-crossword";
	_extensionMIMEType["3gp"] = "video/3gpp";
	_extensionMIMEType["3g2"] = "video/3gpp2";
	_extensionMIMEType["mseq"] = "application/vnd.mseq";
	_extensionMIMEType["pwn"] = "application/vnd.3m.post-it-notes";
	_extensionMIMEType["plb"] = "application/vnd.3gpp.pic-bw-large";
	_extensionMIMEType["psb"] = "application/vnd.3gpp.pic-bw-small";
	_extensionMIMEType["pvb"] = "application/vnd.3gpp.pic-bw-var";
	_extensionMIMEType["tcap"] = "application/vnd.3gpp2.tcap";
	_extensionMIMEType["7z"] = "application/x-7z-compressed";
	_extensionMIMEType["abw"] = "application/x-abiword";
	_extensionMIMEType["ace"] = "application/x-ace-compressed";
	_extensionMIMEType["acc"] = "application/vnd.americandynamics.acc";
	_extensionMIMEType["acu"] = "application/vnd.acucobol";
	_extensionMIMEType["atc"] = "application/vnd.acucorp";
	_extensionMIMEType["adp"] = "audio/adpcm";
	_extensionMIMEType["aab"] = "application/x-authorware-bin";
	_extensionMIMEType["aam"] = "application/x-authorware-map";
	_extensionMIMEType["aas"] = "application/x-authorware-seg";
	_extensionMIMEType["air"] = "application/vnd.adobe.air-application-installer-package+zip";
	_extensionMIMEType["swf"] = "application/x-shockwave-flash";
	_extensionMIMEType["fxp"] = "application/vnd.adobe.fxp";
	_extensionMIMEType["pdf"] = "application/pdf";
	_extensionMIMEType["ppd"] = "application/vnd.cups-ppd";
	_extensionMIMEType["dir"] = "application/x-director";
	_extensionMIMEType["xdp"] = "application/vnd.adobe.xdp+xml";
	_extensionMIMEType["xfdf"] = "application/vnd.adobe.xfdf";
	_extensionMIMEType["aac"] = "audio/x-aac";
	_extensionMIMEType["ahead"] = "application/vnd.ahead.space";
	_extensionMIMEType["azf"] = "application/vnd.airzip.filesecure.azf";
	_extensionMIMEType["azs"] = "application/vnd.airzip.filesecure.azs";
	_extensionMIMEType["azw"] = "application/vnd.amazon.ebook";
	_extensionMIMEType["ami"] = "application/vnd.amiga.ami";
	_extensionMIMEType["N/A"] = "application/andrew-inset";
	_extensionMIMEType["apk"] = "application/vnd.android.package-archive";
	_extensionMIMEType["cii"] = "application/vnd.anser-web-certificate-issue-initiation";
	_extensionMIMEType["fti"] = "application/vnd.anser-web-funds-transfer-initiation";
	_extensionMIMEType["atx"] = "application/vnd.antix.game-component";
	_extensionMIMEType["dmg"] = "application/x-apple-diskimage";
	_extensionMIMEType["mpkg"] = "application/vnd.apple.installer+xml";
	_extensionMIMEType["aw"] = "application/applixware";
	_extensionMIMEType["les"] = "application/vnd.hhe.lesson-player";
	_extensionMIMEType["swi"] = "application/vnd.aristanetworks.swi";
	_extensionMIMEType["s"] = "text/x-asm";
	_extensionMIMEType["atomcat"] = "application/atomcat+xml";
	_extensionMIMEType["atomsvc"] = "application/atomsvc+xml";
	_extensionMIMEType["atom, .xml"] = "application/atom+xml";
	_extensionMIMEType["ac"] = "application/pkix-attr-cert";
	_extensionMIMEType["aif"] = "audio/x-aiff";
	_extensionMIMEType["avi"] = "video/x-msvideo";
	_extensionMIMEType["aep"] = "application/vnd.audiograph";
	_extensionMIMEType["dxf"] = "image/vnd.dxf";
	_extensionMIMEType["dwf"] = "model/vnd.dwf";
	_extensionMIMEType["par"] = "text/plain-bas";
	_extensionMIMEType["bcpio"] = "application/x-bcpio";
	_extensionMIMEType["bin"] = "application/octet-stream";
	_extensionMIMEType["bmp"] = "image/bmp";
	_extensionMIMEType["torrent"] = "application/x-bittorrent";
	_extensionMIMEType["cod"] = "application/vnd.rim.cod";
	_extensionMIMEType["mpm"] = "application/vnd.blueice.multipass";
	_extensionMIMEType["bmi"] = "application/vnd.bmi";
	_extensionMIMEType["sh"] = "application/x-sh";
	_extensionMIMEType["btif"] = "image/prs.btif";
	_extensionMIMEType["rep"] = "application/vnd.businessobjects";
	_extensionMIMEType["bz"] = "application/x-bzip";
	_extensionMIMEType["bz2"] = "application/x-bzip2";
	_extensionMIMEType["csh"] = "application/x-csh";
	_extensionMIMEType["c"] = "text/x-c";
	_extensionMIMEType["cdxml"] = "application/vnd.chemdraw+xml";
	_extensionMIMEType["css"] = "text/css";
	_extensionMIMEType["cdx"] = "chemical/x-cdx";
	_extensionMIMEType["cml"] = "chemical/x-cml";
	_extensionMIMEType["csml"] = "chemical/x-csml";
	_extensionMIMEType["cdbcmsg"] = "application/vnd.contact.cmsg";
	_extensionMIMEType["cla"] = "application/vnd.claymore";
	_extensionMIMEType["c4g"] = "application/vnd.clonk.c4group";
	_extensionMIMEType["sub"] = "image/vnd.dvb.subtitle";
	_extensionMIMEType["cdmia"] = "application/cdmi-capability";
	_extensionMIMEType["cdmic"] = "application/cdmi-container";
	_extensionMIMEType["cdmid"] = "application/cdmi-domain";
	_extensionMIMEType["cdmio"] = "application/cdmi-object";
	_extensionMIMEType["cdmiq"] = "application/cdmi-queue";
	_extensionMIMEType["c11amc"] = "application/vnd.cluetrust.cartomobile-config";
	_extensionMIMEType["c11amz"] = "application/vnd.cluetrust.cartomobile-config-pkg";
	_extensionMIMEType["ras"] = "image/x-cmu-raster";
	_extensionMIMEType["dae"] = "model/vnd.collada+xml";
	_extensionMIMEType["csv"] = "text/csv";
	_extensionMIMEType["cpt"] = "application/mac-compactpro";
	_extensionMIMEType["wmlc"] = "application/vnd.wap.wmlc";
	_extensionMIMEType["cgm"] = "image/cgm";
	_extensionMIMEType["ice"] = "x-conference/x-cooltalk";
	_extensionMIMEType["cmx"] = "image/x-cmx";
	_extensionMIMEType["xar"] = "application/vnd.xara";
	_extensionMIMEType["cmc"] = "application/vnd.cosmocaller";
	_extensionMIMEType["cpio"] = "application/x-cpio";
	_extensionMIMEType["clkx"] = "application/vnd.crick.clicker";
	_extensionMIMEType["clkk"] = "application/vnd.crick.clicker.keyboard";
	_extensionMIMEType["clkp"] = "application/vnd.crick.clicker.palette";
	_extensionMIMEType["clkt"] = "application/vnd.crick.clicker.template";
	_extensionMIMEType["clkw"] = "application/vnd.crick.clicker.wordbank";
	_extensionMIMEType["wbs"] = "application/vnd.criticaltools.wbs+xml";
	_extensionMIMEType["cryptonote"] = "application/vnd.rig.cryptonote";
	_extensionMIMEType["cif"] = "chemical/x-cif";
	_extensionMIMEType["cmdf"] = "chemical/x-cmdf";
	_extensionMIMEType["cu"] = "application/cu-seeme";
	_extensionMIMEType["cww"] = "application/prs.cww";
	_extensionMIMEType["curl"] = "text/vnd.curl";
	_extensionMIMEType["dcurl"] = "text/vnd.curl.dcurl";
	_extensionMIMEType["mcurl"] = "text/vnd.curl.mcurl";
	_extensionMIMEType["scurl"] = "text/vnd.curl.scurl";
	_extensionMIMEType["car"] = "application/vnd.curl.car";
	_extensionMIMEType["pcurl"] = "application/vnd.curl.pcurl";
	_extensionMIMEType["cmp"] = "application/vnd.yellowriver-custom-menu";
	_extensionMIMEType["dssc"] = "application/dssc+der";
	_extensionMIMEType["xdssc"] = "application/dssc+xml";
	_extensionMIMEType["deb"] = "application/x-debian-package";
	_extensionMIMEType["uva"] = "audio/vnd.dece.audio";
	_extensionMIMEType["uvi"] = "image/vnd.dece.graphic";
	_extensionMIMEType["uvh"] = "video/vnd.dece.hd";
	_extensionMIMEType["uvm"] = "video/vnd.dece.mobile";
	_extensionMIMEType["uvu"] = "video/vnd.uvvu.mp4";
	_extensionMIMEType["uvp"] = "video/vnd.dece.pd";
	_extensionMIMEType["uvs"] = "video/vnd.dece.sd";
	_extensionMIMEType["uvv"] = "video/vnd.dece.video";
	_extensionMIMEType["dvi"] = "application/x-dvi";
	_extensionMIMEType["seed"] = "application/vnd.fdsn.seed";
	_extensionMIMEType["dtb"] = "application/x-dtbook+xml";
	_extensionMIMEType["res"] = "application/x-dtbresource+xml";
	_extensionMIMEType["ait"] = "application/vnd.dvb.ait";
	_extensionMIMEType["svc"] = "application/vnd.dvb.service";
	_extensionMIMEType["eol"] = "audio/vnd.digital-winds";
	_extensionMIMEType["djvu"] = "image/vnd.djvu";
	_extensionMIMEType["dtd"] = "application/xml-dtd";
	_extensionMIMEType["mlp"] = "application/vnd.dolby.mlp";
	_extensionMIMEType["wad"] = "application/x-doom";
	_extensionMIMEType["dpg"] = "application/vnd.dpgraph";
	_extensionMIMEType["dra"] = "audio/vnd.dra";
	_extensionMIMEType["dfac"] = "application/vnd.dreamfactory";
	_extensionMIMEType["dts"] = "audio/vnd.dts";
	_extensionMIMEType["dtshd"] = "audio/vnd.dts.hd";
	_extensionMIMEType["dwg"] = "image/vnd.dwg";
	_extensionMIMEType["geo"] = "application/vnd.dynageo";
	_extensionMIMEType["es"] = "application/ecmascript";
	_extensionMIMEType["mag"] = "application/vnd.ecowin.chart";
	_extensionMIMEType["mmr"] = "image/vnd.fujixerox.edmics-mmr";
	_extensionMIMEType["rlc"] = "image/vnd.fujixerox.edmics-rlc";
	_extensionMIMEType["exi"] = "application/exi";
	_extensionMIMEType["mgz"] = "application/vnd.proteus.magazine";
	_extensionMIMEType["epub"] = "application/epub+zip";
	_extensionMIMEType["eml"] = "message/rfc822";
	_extensionMIMEType["nml"] = "application/vnd.enliven";
	_extensionMIMEType["xpr"] = "application/vnd.is-xpr";
	_extensionMIMEType["xif"] = "image/vnd.xiff";
	_extensionMIMEType["xfdl"] = "application/vnd.xfdl";
	_extensionMIMEType["emma"] = "application/emma+xml";
	_extensionMIMEType["ez2"] = "application/vnd.ezpix-album";
	_extensionMIMEType["ez3"] = "application/vnd.ezpix-package";
	_extensionMIMEType["fst"] = "image/vnd.fst";
	_extensionMIMEType["fvt"] = "video/vnd.fvt";
	_extensionMIMEType["fbs"] = "image/vnd.fastbidsheet";
	_extensionMIMEType["fe_launch"] = "application/vnd.denovo.fcselayout-link";
	_extensionMIMEType["f4v"] = "video/x-f4v";
	_extensionMIMEType["flv"] = "video/x-flv";
	_extensionMIMEType["fpx"] = "image/vnd.fpx";
	_extensionMIMEType["npx"] = "image/vnd.net-fpx";
	_extensionMIMEType["flx"] = "text/vnd.fmi.flexstor";
	_extensionMIMEType["fli"] = "video/x-fli";
	_extensionMIMEType["ftc"] = "application/vnd.fluxtime.clip";
	_extensionMIMEType["fdf"] = "application/vnd.fdf";
	_extensionMIMEType["f"] = "text/x-fortran";
	_extensionMIMEType["mif"] = "application/vnd.mif";
	_extensionMIMEType["fm"] = "application/vnd.framemaker";
	_extensionMIMEType["fh"] = "image/x-freehand";
	_extensionMIMEType["fsc"] = "application/vnd.fsc.weblaunch";
	_extensionMIMEType["fnc"] = "application/vnd.frogans.fnc";
	_extensionMIMEType["ltf"] = "application/vnd.frogans.ltf";
	_extensionMIMEType["ddd"] = "application/vnd.fujixerox.ddd";
	_extensionMIMEType["xdw"] = "application/vnd.fujixerox.docuworks";
	_extensionMIMEType["xbd"] = "application/vnd.fujixerox.docuworks.binder";
	_extensionMIMEType["oas"] = "application/vnd.fujitsu.oasys";
	_extensionMIMEType["oa2"] = "application/vnd.fujitsu.oasys2";
	_extensionMIMEType["oa3"] = "application/vnd.fujitsu.oasys3";
	_extensionMIMEType["fg5"] = "application/vnd.fujitsu.oasysgp";
	_extensionMIMEType["bh2"] = "application/vnd.fujitsu.oasysprs";
	_extensionMIMEType["spl"] = "application/x-futuresplash";
	_extensionMIMEType["fzs"] = "application/vnd.fuzzysheet";
	_extensionMIMEType["g3"] = "image/g3fax";
	_extensionMIMEType["gmx"] = "application/vnd.gmx";
	_extensionMIMEType["gtw"] = "model/vnd.gtw";
	_extensionMIMEType["txd"] = "application/vnd.genomatix.tuxedo";
	_extensionMIMEType["ggb"] = "application/vnd.geogebra.file";
	_extensionMIMEType["ggt"] = "application/vnd.geogebra.tool";
	_extensionMIMEType["gdl"] = "model/vnd.gdl";
	_extensionMIMEType["gex"] = "application/vnd.geometry-explorer";
	_extensionMIMEType["gxt"] = "application/vnd.geonext";
	_extensionMIMEType["g2w"] = "application/vnd.geoplan";
	_extensionMIMEType["g3w"] = "application/vnd.geospace";
	_extensionMIMEType["gsf"] = "application/x-font-ghostscript";
	_extensionMIMEType["bdf"] = "application/x-font-bdf";
	_extensionMIMEType["gtar"] = "application/x-gtar";
	_extensionMIMEType["texinfo"] = "application/x-texinfo";
	_extensionMIMEType["gnumeric"] = "application/x-gnumeric";
	_extensionMIMEType["kml"] = "application/vnd.google-earth.kml+xml";
	_extensionMIMEType["kmz"] = "application/vnd.google-earth.kmz";
	_extensionMIMEType["gpx"] = "application/gpx+xml";
	_extensionMIMEType["gqf"] = "application/vnd.grafeq";
	_extensionMIMEType["gif"] = "image/gif";
	_extensionMIMEType["gv"] = "text/vnd.graphviz";
	_extensionMIMEType["gac"] = "application/vnd.groove-account";
	_extensionMIMEType["ghf"] = "application/vnd.groove-help";
	_extensionMIMEType["gim"] = "application/vnd.groove-identity-message";
	_extensionMIMEType["grv"] = "application/vnd.groove-injector";
	_extensionMIMEType["gtm"] = "application/vnd.groove-tool-message";
	_extensionMIMEType["tpl"] = "application/vnd.groove-tool-template";
	_extensionMIMEType["vcg"] = "application/vnd.groove-vcard";
	_extensionMIMEType["h261"] = "video/h261";
	_extensionMIMEType["h263"] = "video/h263";
	_extensionMIMEType["h264"] = "video/h264";
	_extensionMIMEType["hpid"] = "application/vnd.hp-hpid";
	_extensionMIMEType["hps"] = "application/vnd.hp-hps";
	_extensionMIMEType["hdf"] = "application/x-hdf";
	_extensionMIMEType["rip"] = "audio/vnd.rip";
	_extensionMIMEType["hbci"] = "application/vnd.hbci";
	_extensionMIMEType["jlt"] = "application/vnd.hp-jlyt";
	_extensionMIMEType["pcl"] = "application/vnd.hp-pcl";
	_extensionMIMEType["hpgl"] = "application/vnd.hp-hpgl";
	_extensionMIMEType["hvs"] = "application/vnd.yamaha.hv-script";
	_extensionMIMEType["hvd"] = "application/vnd.yamaha.hv-dic";
	_extensionMIMEType["hvp"] = "application/vnd.yamaha.hv-voice";
	_extensionMIMEType["sfd-hdstx"] = "application/vnd.hydrostatix.sof-data";
	_extensionMIMEType["stk"] = "application/hyperstudio";
	_extensionMIMEType["hal"] = "application/vnd.hal+xml";
	_extensionMIMEType["html"] = "text/html";
	_extensionMIMEType["irm"] = "application/vnd.ibm.rights-management";
	_extensionMIMEType["sc"] = "application/vnd.ibm.secure-container";
	_extensionMIMEType["ics"] = "text/calendar";
	_extensionMIMEType["icc"] = "application/vnd.iccprofile";
	_extensionMIMEType["ico"] = "image/x-icon";
	_extensionMIMEType["igl"] = "application/vnd.igloader";
	_extensionMIMEType["ief"] = "image/ief";
	_extensionMIMEType["ivp"] = "application/vnd.immervision-ivp";
	_extensionMIMEType["ivu"] = "application/vnd.immervision-ivu";
	_extensionMIMEType["rif"] = "application/reginfo+xml";
	_extensionMIMEType["3dml"] = "text/vnd.in3d.3dml";
	_extensionMIMEType["spot"] = "text/vnd.in3d.spot";
	_extensionMIMEType["igs"] = "model/iges";
	_extensionMIMEType["i2g"] = "application/vnd.intergeo";
	_extensionMIMEType["cdy"] = "application/vnd.cinderella";
	_extensionMIMEType["xpw"] = "application/vnd.intercon.formnet";
	_extensionMIMEType["fcs"] = "application/vnd.isac.fcs";
	_extensionMIMEType["ipfix"] = "application/ipfix";
	_extensionMIMEType["cer"] = "application/pkix-cert";
	_extensionMIMEType["pki"] = "application/pkixcmp";
	_extensionMIMEType["crl"] = "application/pkix-crl";
	_extensionMIMEType["pkipath"] = "application/pkix-pkipath";
	_extensionMIMEType["igm"] = "application/vnd.insors.igm";
	_extensionMIMEType["rcprofile"] = "application/vnd.ipunplugged.rcprofile";
	_extensionMIMEType["irp"] = "application/vnd.irepository.package+xml";
	_extensionMIMEType["jad"] = "text/vnd.sun.j2me.app-descriptor";
	_extensionMIMEType["jar"] = "application/java-archive";
	_extensionMIMEType["class"] = "application/java-vm";
	_extensionMIMEType["jnlp"] = "application/x-java-jnlp-file";
	_extensionMIMEType["ser"] = "application/java-serialized-object";
	_extensionMIMEType["java"] = "text/x-java-source,java";
	_extensionMIMEType["js"] = "application/javascript";
	_extensionMIMEType["json"] = "application/json";
	_extensionMIMEType["joda"] = "application/vnd.joost.joda-archive";
	_extensionMIMEType["jpm"] = "video/jpm";
	_extensionMIMEType["jpeg, .jpg"] = "image/jpeg";
	_extensionMIMEType["jpeg, .jpg"] = "image/x-citrix-jpeg";
	_extensionMIMEType["pjpeg"] = "image/pjpeg";
	_extensionMIMEType["jpgv"] = "video/jpeg";
	_extensionMIMEType["ktz"] = "application/vnd.kahootz";
	_extensionMIMEType["mmd"] = "application/vnd.chipnuts.karaoke-mmd";
	_extensionMIMEType["karbon"] = "application/vnd.kde.karbon";
	_extensionMIMEType["chrt"] = "application/vnd.kde.kchart";
	_extensionMIMEType["kfo"] = "application/vnd.kde.kformula";
	_extensionMIMEType["flw"] = "application/vnd.kde.kivio";
	_extensionMIMEType["kon"] = "application/vnd.kde.kontour";
	_extensionMIMEType["kpr"] = "application/vnd.kde.kpresenter";
	_extensionMIMEType["ksp"] = "application/vnd.kde.kspread";
	_extensionMIMEType["kwd"] = "application/vnd.kde.kword";
	_extensionMIMEType["htke"] = "application/vnd.kenameaapp";
	_extensionMIMEType["kia"] = "application/vnd.kidspiration";
	_extensionMIMEType["kne"] = "application/vnd.kinar";
	_extensionMIMEType["sse"] = "application/vnd.kodak-descriptor";
	_extensionMIMEType["lasxml"] = "application/vnd.las.las+xml";
	_extensionMIMEType["latex"] = "application/x-latex";
	_extensionMIMEType["lbd"] = "application/vnd.llamagraphics.life-balance.desktop";
	_extensionMIMEType["lbe"] = "application/vnd.llamagraphics.life-balance.exchange+xml";
	_extensionMIMEType["jam"] = "application/vnd.jam";
	_extensionMIMEType["123"] = "application/vnd.lotus-1-2-3";
	_extensionMIMEType["apr"] = "application/vnd.lotus-approach";
	_extensionMIMEType["pre"] = "application/vnd.lotus-freelance";
	_extensionMIMEType["nsf"] = "application/vnd.lotus-notes";
	_extensionMIMEType["org"] = "application/vnd.lotus-organizer";
	_extensionMIMEType["scm"] = "application/vnd.lotus-screencam";
	_extensionMIMEType["lwp"] = "application/vnd.lotus-wordpro";
	_extensionMIMEType["lvp"] = "audio/vnd.lucent.voice";
	_extensionMIMEType["m3u"] = "audio/x-mpegurl";
	_extensionMIMEType["m4v"] = "video/x-m4v";
	_extensionMIMEType["hqx"] = "application/mac-binhex40";
	_extensionMIMEType["portpkg"] = "application/vnd.macports.portpkg";
	_extensionMIMEType["mgp"] = "application/vnd.osgeo.mapguide.package";
	_extensionMIMEType["mrc"] = "application/marc";
	_extensionMIMEType["mrcx"] = "application/marcxml+xml";
	_extensionMIMEType["mxf"] = "application/mxf";
	_extensionMIMEType["nbp"] = "application/vnd.wolfram.player";
	_extensionMIMEType["ma"] = "application/mathematica";
	_extensionMIMEType["mathml"] = "application/mathml+xml";
	_extensionMIMEType["mbox"] = "application/mbox";
	_extensionMIMEType["mc1"] = "application/vnd.medcalcdata";
	_extensionMIMEType["mscml"] = "application/mediaservercontrol+xml";
	_extensionMIMEType["cdkey"] = "application/vnd.mediastation.cdkey";
	_extensionMIMEType["mwf"] = "application/vnd.mfer";
	_extensionMIMEType["mfm"] = "application/vnd.mfmp";
	_extensionMIMEType["msh"] = "model/mesh";
	_extensionMIMEType["mads"] = "application/mads+xml";
	_extensionMIMEType["mets"] = "application/mets+xml";
	_extensionMIMEType["mods"] = "application/mods+xml";
	_extensionMIMEType["meta4"] = "application/metalink4+xml";
	_extensionMIMEType["mcd"] = "application/vnd.mcd";
	_extensionMIMEType["flo"] = "application/vnd.micrografx.flo";
	_extensionMIMEType["igx"] = "application/vnd.micrografx.igx";
	_extensionMIMEType["es3"] = "application/vnd.eszigno3+xml";
	_extensionMIMEType["mdb"] = "application/x-msaccess";
	_extensionMIMEType["asf"] = "video/x-ms-asf";
	_extensionMIMEType["exe"] = "application/x-msdownload";
	_extensionMIMEType["cil"] = "application/vnd.ms-artgalry";
	_extensionMIMEType["cab"] = "application/vnd.ms-cab-compressed";
	_extensionMIMEType["ims"] = "application/vnd.ms-ims";
	_extensionMIMEType["application"] = "application/x-ms-application";
	_extensionMIMEType["clp"] = "application/x-msclip";
	_extensionMIMEType["mdi"] = "image/vnd.ms-modi";
	_extensionMIMEType["eot"] = "application/vnd.ms-fontobject";
	_extensionMIMEType["xls"] = "application/vnd.ms-excel";
	_extensionMIMEType["xlam"] = "application/vnd.ms-excel.addin.macroenabled.12";
	_extensionMIMEType["xlsb"] = "application/vnd.ms-excel.sheet.binary.macroenabled.12";
	_extensionMIMEType["xltm"] = "application/vnd.ms-excel.template.macroenabled.12";
	_extensionMIMEType["xlsm"] = "application/vnd.ms-excel.sheet.macroenabled.12";
	_extensionMIMEType["chm"] = "application/vnd.ms-htmlhelp";
	_extensionMIMEType["crd"] = "application/x-mscardfile";
	_extensionMIMEType["lrm"] = "application/vnd.ms-lrm";
	_extensionMIMEType["mvb"] = "application/x-msmediaview";
	_extensionMIMEType["mny"] = "application/x-msmoney";
	_extensionMIMEType["pptx"] = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
	_extensionMIMEType["sldx"] = "application/vnd.openxmlformats-officedocument.presentationml.slide";
	_extensionMIMEType["ppsx"] = "application/vnd.openxmlformats-officedocument.presentationml.slideshow";
	_extensionMIMEType["potx"] = "application/vnd.openxmlformats-officedocument.presentationml.template";
	_extensionMIMEType["xlsx"] = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
	_extensionMIMEType["xltx"] = "application/vnd.openxmlformats-officedocument.spreadsheetml.template";
	_extensionMIMEType["docx"] = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
	_extensionMIMEType["dotx"] = "application/vnd.openxmlformats-officedocument.wordprocessingml.template";
	_extensionMIMEType["obd"] = "application/x-msbinder";
	_extensionMIMEType["thmx"] = "application/vnd.ms-officetheme";
	_extensionMIMEType["onetoc"] = "application/onenote";
	_extensionMIMEType["pya"] = "audio/vnd.ms-playready.media.pya";
	_extensionMIMEType["pyv"] = "video/vnd.ms-playready.media.pyv";
	_extensionMIMEType["ppt"] = "application/vnd.ms-powerpoint";
	_extensionMIMEType["ppam"] = "application/vnd.ms-powerpoint.addin.macroenabled.12";
	_extensionMIMEType["sldm"] = "application/vnd.ms-powerpoint.slide.macroenabled.12";
	_extensionMIMEType["pptm"] = "application/vnd.ms-powerpoint.presentation.macroenabled.12";
	_extensionMIMEType["ppsm"] = "application/vnd.ms-powerpoint.slideshow.macroenabled.12";
	_extensionMIMEType["potm"] = "application/vnd.ms-powerpoint.template.macroenabled.12";
	_extensionMIMEType["mpp"] = "application/vnd.ms-project";
	_extensionMIMEType["pub"] = "application/x-mspublisher";
	_extensionMIMEType["scd"] = "application/x-msschedule";
	_extensionMIMEType["xap"] = "application/x-silverlight-app";
	_extensionMIMEType["stl"] = "application/vnd.ms-pki.stl";
	_extensionMIMEType["cat"] = "application/vnd.ms-pki.seccat";
	_extensionMIMEType["vsd"] = "application/vnd.visio";
	_extensionMIMEType["vsdx"] = "application/vnd.visio2013";
	_extensionMIMEType["wm"] = "video/x-ms-wm";
	_extensionMIMEType["wma"] = "audio/x-ms-wma";
	_extensionMIMEType["wax"] = "audio/x-ms-wax";
	_extensionMIMEType["wmx"] = "video/x-ms-wmx";
	_extensionMIMEType["wmd"] = "application/x-ms-wmd";
	_extensionMIMEType["wpl"] = "application/vnd.ms-wpl";
	_extensionMIMEType["wmz"] = "application/x-ms-wmz";
	_extensionMIMEType["wmv"] = "video/x-ms-wmv";
	_extensionMIMEType["wvx"] = "video/x-ms-wvx";
	_extensionMIMEType["wmf"] = "application/x-msmetafile";
	_extensionMIMEType["trm"] = "application/x-msterminal";
	_extensionMIMEType["doc"] = "application/msword";
	_extensionMIMEType["docm"] = "application/vnd.ms-word.document.macroenabled.12";
	_extensionMIMEType["dotm"] = "application/vnd.ms-word.template.macroenabled.12";
	_extensionMIMEType["wri"] = "application/x-mswrite";
	_extensionMIMEType["wps"] = "application/vnd.ms-works";
	_extensionMIMEType["xbap"] = "application/x-ms-xbap";
	_extensionMIMEType["xps"] = "application/vnd.ms-xpsdocument";
	_extensionMIMEType["mid"] = "audio/midi";
	_extensionMIMEType["mpy"] = "application/vnd.ibm.minipay";
	_extensionMIMEType["afp"] = "application/vnd.ibm.modcap";
	_extensionMIMEType["rms"] = "application/vnd.jcp.javame.midlet-rms";
	_extensionMIMEType["tmo"] = "application/vnd.tmobile-livetv";
	_extensionMIMEType["prc"] = "application/x-mobipocket-ebook";
	_extensionMIMEType["mbk"] = "application/vnd.mobius.mbk";
	_extensionMIMEType["dis"] = "application/vnd.mobius.dis";
	_extensionMIMEType["plc"] = "application/vnd.mobius.plc";
	_extensionMIMEType["mqy"] = "application/vnd.mobius.mqy";
	_extensionMIMEType["msl"] = "application/vnd.mobius.msl";
	_extensionMIMEType["txf"] = "application/vnd.mobius.txf";
	_extensionMIMEType["daf"] = "application/vnd.mobius.daf";
	_extensionMIMEType["fly"] = "text/vnd.fly";
	_extensionMIMEType["mpc"] = "application/vnd.mophun.certificate";
	_extensionMIMEType["mpn"] = "application/vnd.mophun.application";
	_extensionMIMEType["mj2"] = "video/mj2";
	_extensionMIMEType["mpga"] = "audio/mpeg";
	_extensionMIMEType["mxu"] = "video/vnd.mpegurl";
	_extensionMIMEType["mpeg"] = "video/mpeg";
	_extensionMIMEType["m21"] = "application/mp21";
	_extensionMIMEType["mp4a"] = "audio/mp4";
	_extensionMIMEType["mp4"] = "video/mp4";
	_extensionMIMEType["mp4"] = "application/mp4";
	_extensionMIMEType["m3u8"] = "application/vnd.apple.mpegurl";
	_extensionMIMEType["mus"] = "application/vnd.musician";
	_extensionMIMEType["msty"] = "application/vnd.muvee.style";
	_extensionMIMEType["mxml"] = "application/xv+xml";
	_extensionMIMEType["ngdat"] = "application/vnd.nokia.n-gage.data";
	_extensionMIMEType["n-gage"] = "application/vnd.nokia.n-gage.symbian.install";
	_extensionMIMEType["ncx"] = "application/x-dtbncx+xml";
	_extensionMIMEType["nc"] = "application/x-netcdf";
	_extensionMIMEType["nlu"] = "application/vnd.neurolanguage.nlu";
	_extensionMIMEType["dna"] = "application/vnd.dna";
	_extensionMIMEType["nnd"] = "application/vnd.noblenet-directory";
	_extensionMIMEType["nns"] = "application/vnd.noblenet-sealer";
	_extensionMIMEType["nnw"] = "application/vnd.noblenet-web";
	_extensionMIMEType["rpst"] = "application/vnd.nokia.radio-preset";
	_extensionMIMEType["rpss"] = "application/vnd.nokia.radio-presets";
	_extensionMIMEType["n3"] = "text/n3";
	_extensionMIMEType["edm"] = "application/vnd.novadigm.edm";
	_extensionMIMEType["edx"] = "application/vnd.novadigm.edx";
	_extensionMIMEType["ext"] = "application/vnd.novadigm.ext";
	_extensionMIMEType["gph"] = "application/vnd.flographit";
	_extensionMIMEType["ecelp4800"] = "audio/vnd.nuera.ecelp4800";
	_extensionMIMEType["ecelp7470"] = "audio/vnd.nuera.ecelp7470";
	_extensionMIMEType["ecelp9600"] = "audio/vnd.nuera.ecelp9600";
	_extensionMIMEType["oda"] = "application/oda";
	_extensionMIMEType["ogx"] = "application/ogg";
	_extensionMIMEType["oga"] = "audio/ogg";
	_extensionMIMEType["ogv"] = "video/ogg";
	_extensionMIMEType["dd2"] = "application/vnd.oma.dd2+xml";
	_extensionMIMEType["oth"] = "application/vnd.oasis.opendocument.text-web";
	_extensionMIMEType["opf"] = "application/oebps-package+xml";
	_extensionMIMEType["qbo"] = "application/vnd.intu.qbo";
	_extensionMIMEType["oxt"] = "application/vnd.openofficeorg.extension";
	_extensionMIMEType["osf"] = "application/vnd.yamaha.openscoreformat";
	_extensionMIMEType["weba"] = "audio/webm";
	_extensionMIMEType["webm"] = "video/webm";
	_extensionMIMEType["odc"] = "application/vnd.oasis.opendocument.chart";
	_extensionMIMEType["otc"] = "application/vnd.oasis.opendocument.chart-template";
	_extensionMIMEType["odb"] = "application/vnd.oasis.opendocument.database";
	_extensionMIMEType["odf"] = "application/vnd.oasis.opendocument.formula";
	_extensionMIMEType["odft"] = "application/vnd.oasis.opendocument.formula-template";
	_extensionMIMEType["odg"] = "application/vnd.oasis.opendocument.graphics";
	_extensionMIMEType["otg"] = "application/vnd.oasis.opendocument.graphics-template";
	_extensionMIMEType["odi"] = "application/vnd.oasis.opendocument.image";
	_extensionMIMEType["oti"] = "application/vnd.oasis.opendocument.image-template";
	_extensionMIMEType["odp"] = "application/vnd.oasis.opendocument.presentation";
	_extensionMIMEType["otp"] = "application/vnd.oasis.opendocument.presentation-template";
	_extensionMIMEType["ods"] = "application/vnd.oasis.opendocument.spreadsheet";
	_extensionMIMEType["ots"] = "application/vnd.oasis.opendocument.spreadsheet-template";
	_extensionMIMEType["odt"] = "application/vnd.oasis.opendocument.text";
	_extensionMIMEType["odm"] = "application/vnd.oasis.opendocument.text-master";
	_extensionMIMEType["ott"] = "application/vnd.oasis.opendocument.text-template";
	_extensionMIMEType["ktx"] = "image/ktx";
	_extensionMIMEType["sxc"] = "application/vnd.sun.xml.calc";
	_extensionMIMEType["stc"] = "application/vnd.sun.xml.calc.template";
	_extensionMIMEType["sxd"] = "application/vnd.sun.xml.draw";
	_extensionMIMEType["std"] = "application/vnd.sun.xml.draw.template";
	_extensionMIMEType["sxi"] = "application/vnd.sun.xml.impress";
	_extensionMIMEType["sti"] = "application/vnd.sun.xml.impress.template";
	_extensionMIMEType["sxm"] = "application/vnd.sun.xml.math";
	_extensionMIMEType["sxw"] = "application/vnd.sun.xml.writer";
	_extensionMIMEType["sxg"] = "application/vnd.sun.xml.writer.global";
	_extensionMIMEType["stw"] = "application/vnd.sun.xml.writer.template";
	_extensionMIMEType["otf"] = "application/x-font-otf";
	_extensionMIMEType["osfpvg"] = "application/vnd.yamaha.openscoreformat.osfpvg+xml";
	_extensionMIMEType["dp"] = "application/vnd.osgi.dp";
	_extensionMIMEType["pdb"] = "application/vnd.palm";
	_extensionMIMEType["p"] = "text/x-pascal";
	_extensionMIMEType["paw"] = "application/vnd.pawaafile";
	_extensionMIMEType["pclxl"] = "application/vnd.hp-pclxl";
	_extensionMIMEType["efif"] = "application/vnd.picsel";
	_extensionMIMEType["pcx"] = "image/x-pcx";
	_extensionMIMEType["psd"] = "image/vnd.adobe.photoshop";
	_extensionMIMEType["prf"] = "application/pics-rules";
	_extensionMIMEType["pic"] = "image/x-pict";
	_extensionMIMEType["chat"] = "application/x-chat";
	_extensionMIMEType["p10"] = "application/pkcs10";
	_extensionMIMEType["p12"] = "application/x-pkcs12";
	_extensionMIMEType["p7m"] = "application/pkcs7-mime";
	_extensionMIMEType["p7s"] = "application/pkcs7-signature";
	_extensionMIMEType["p7r"] = "application/x-pkcs7-certreqresp";
	_extensionMIMEType["p7b"] = "application/x-pkcs7-certificates";
	_extensionMIMEType["p8"] = "application/pkcs8";
	_extensionMIMEType["plf"] = "application/vnd.pocketlearn";
	_extensionMIMEType["pnm"] = "image/x-portable-anymap";
	_extensionMIMEType["pbm"] = "image/x-portable-bitmap";
	_extensionMIMEType["pcf"] = "application/x-font-pcf";
	_extensionMIMEType["pfr"] = "application/font-tdpfr";
	_extensionMIMEType["pgn"] = "application/x-chess-pgn";
	_extensionMIMEType["pgm"] = "image/x-portable-graymap";
	_extensionMIMEType["png"] = "image/png";
	_extensionMIMEType["png"] = "image/x-citrix-png";
	_extensionMIMEType["png"] = "image/x-png";
	_extensionMIMEType["ppm"] = "image/x-portable-pixmap";
	_extensionMIMEType["pskcxml"] = "application/pskc+xml";
	_extensionMIMEType["pml"] = "application/vnd.ctc-posml";
	_extensionMIMEType["ai"] = "application/postscript";
	_extensionMIMEType["pfa"] = "application/x-font-type1";
	_extensionMIMEType["pbd"] = "application/vnd.powerbuilder6";
	_extensionMIMEType["pgp"] = "application/pgp-encrypted";
	_extensionMIMEType["pgp"] = "application/pgp-signature";
	_extensionMIMEType["box"] = "application/vnd.previewsystems.box";
	_extensionMIMEType["ptid"] = "application/vnd.pvi.ptid1";
	_extensionMIMEType["pls"] = "application/pls+xml";
	_extensionMIMEType["str"] = "application/vnd.pg.format";
	_extensionMIMEType["ei6"] = "application/vnd.pg.osasli";
	_extensionMIMEType["dsc"] = "text/prs.lines.tag";
	_extensionMIMEType["psf"] = "application/x-font-linux-psf";
	_extensionMIMEType["qps"] = "application/vnd.publishare-delta-tree";
	_extensionMIMEType["wg"] = "application/vnd.pmi.widget";
	_extensionMIMEType["qxd"] = "application/vnd.quark.quarkxpress";
	_extensionMIMEType["esf"] = "application/vnd.epson.esf";
	_extensionMIMEType["msf"] = "application/vnd.epson.msf";
	_extensionMIMEType["ssf"] = "application/vnd.epson.ssf";
	_extensionMIMEType["qam"] = "application/vnd.epson.quickanime";
	_extensionMIMEType["qfx"] = "application/vnd.intu.qfx";
	_extensionMIMEType["qt"] = "video/quicktime";
	_extensionMIMEType["rar"] = "application/x-rar-compressed";
	_extensionMIMEType["ram"] = "audio/x-pn-realaudio";
	_extensionMIMEType["rmp"] = "audio/x-pn-realaudio-plugin";
	_extensionMIMEType["rsd"] = "application/rsd+xml";
	_extensionMIMEType["rm"] = "application/vnd.rn-realmedia";
	_extensionMIMEType["bed"] = "application/vnd.realvnc.bed";
	_extensionMIMEType["mxl"] = "application/vnd.recordare.musicxml";
	_extensionMIMEType["musicxml"] = "application/vnd.recordare.musicxml+xml";
	_extensionMIMEType["rnc"] = "application/relax-ng-compact-syntax";
	_extensionMIMEType["rdz"] = "application/vnd.data-vision.rdz";
	_extensionMIMEType["rdf"] = "application/rdf+xml";
	_extensionMIMEType["rp9"] = "application/vnd.cloanto.rp9";
	_extensionMIMEType["jisp"] = "application/vnd.jisp";
	_extensionMIMEType["rtf"] = "application/rtf";
	_extensionMIMEType["rtx"] = "text/richtext";
	_extensionMIMEType["link66"] = "application/vnd.route66.link66+xml";
	_extensionMIMEType["rss, .xml"] = "application/rss+xml";
	_extensionMIMEType["shf"] = "application/shf+xml";
	_extensionMIMEType["st"] = "application/vnd.sailingtracker.track";
	_extensionMIMEType["svg"] = "image/svg+xml";
	_extensionMIMEType["sus"] = "application/vnd.sus-calendar";
	_extensionMIMEType["sru"] = "application/sru+xml";
	_extensionMIMEType["setpay"] = "application/set-payment-initiation";
	_extensionMIMEType["setreg"] = "application/set-registration-initiation";
	_extensionMIMEType["sema"] = "application/vnd.sema";
	_extensionMIMEType["semd"] = "application/vnd.semd";
	_extensionMIMEType["semf"] = "application/vnd.semf";
	_extensionMIMEType["see"] = "application/vnd.seemail";
	_extensionMIMEType["snf"] = "application/x-font-snf";
	_extensionMIMEType["spq"] = "application/scvp-vp-request";
	_extensionMIMEType["spp"] = "application/scvp-vp-response";
	_extensionMIMEType["scq"] = "application/scvp-cv-request";
	_extensionMIMEType["scs"] = "application/scvp-cv-response";
	_extensionMIMEType["sdp"] = "application/sdp";
	_extensionMIMEType["etx"] = "text/x-setext";
	_extensionMIMEType["movie"] = "video/x-sgi-movie";
	_extensionMIMEType["ifm"] = "application/vnd.shana.informed.formdata";
	_extensionMIMEType["itp"] = "application/vnd.shana.informed.formtemplate";
	_extensionMIMEType["iif"] = "application/vnd.shana.informed.interchange";
	_extensionMIMEType["ipk"] = "application/vnd.shana.informed.package";
	_extensionMIMEType["tfi"] = "application/thraud+xml";
	_extensionMIMEType["shar"] = "application/x-shar";
	_extensionMIMEType["rgb"] = "image/x-rgb";
	_extensionMIMEType["slt"] = "application/vnd.epson.salt";
	_extensionMIMEType["aso"] = "application/vnd.accpac.simply.aso";
	_extensionMIMEType["imp"] = "application/vnd.accpac.simply.imp";
	_extensionMIMEType["twd"] = "application/vnd.simtech-mindmapper";
	_extensionMIMEType["csp"] = "application/vnd.commonspace";
	_extensionMIMEType["saf"] = "application/vnd.yamaha.smaf-audio";
	_extensionMIMEType["mmf"] = "application/vnd.smaf";
	_extensionMIMEType["spf"] = "application/vnd.yamaha.smaf-phrase";
	_extensionMIMEType["teacher"] = "application/vnd.smart.teacher";
	_extensionMIMEType["svd"] = "application/vnd.svd";
	_extensionMIMEType["rq"] = "application/sparql-query";
	_extensionMIMEType["srx"] = "application/sparql-results+xml";
	_extensionMIMEType["gram"] = "application/srgs";
	_extensionMIMEType["grxml"] = "application/srgs+xml";
	_extensionMIMEType["ssml"] = "application/ssml+xml";
	_extensionMIMEType["skp"] = "application/vnd.koan";
	_extensionMIMEType["sgml"] = "text/sgml";
	_extensionMIMEType["sdc"] = "application/vnd.stardivision.calc";
	_extensionMIMEType["sda"] = "application/vnd.stardivision.draw";
	_extensionMIMEType["sdd"] = "application/vnd.stardivision.impress";
	_extensionMIMEType["smf"] = "application/vnd.stardivision.math";
	_extensionMIMEType["sdw"] = "application/vnd.stardivision.writer";
	_extensionMIMEType["sgl"] = "application/vnd.stardivision.writer-global";
	_extensionMIMEType["sm"] = "application/vnd.stepmania.stepchart";
	_extensionMIMEType["sit"] = "application/x-stuffit";
	_extensionMIMEType["sitx"] = "application/x-stuffitx";
	_extensionMIMEType["sdkm"] = "application/vnd.solent.sdkm+xml";
	_extensionMIMEType["xo"] = "application/vnd.olpc-sugar";
	_extensionMIMEType["au"] = "audio/basic";
	_extensionMIMEType["wqd"] = "application/vnd.wqd";
	_extensionMIMEType["sis"] = "application/vnd.symbian.install";
	_extensionMIMEType["smi"] = "application/smil+xml";
	_extensionMIMEType["xsm"] = "application/vnd.syncml+xml";
	_extensionMIMEType["bdm"] = "application/vnd.syncml.dm+wbxml";
	_extensionMIMEType["xdm"] = "application/vnd.syncml.dm+xml";
	_extensionMIMEType["sv4cpio"] = "application/x-sv4cpio";
	_extensionMIMEType["sv4crc"] = "application/x-sv4crc";
	_extensionMIMEType["sbml"] = "application/sbml+xml";
	_extensionMIMEType["tsv"] = "text/tab-separated-values";
	_extensionMIMEType["tiff"] = "image/tiff";
	_extensionMIMEType["tao"] = "application/vnd.tao.intent-module-archive";
	_extensionMIMEType["tar"] = "application/x-tar";
	_extensionMIMEType["tcl"] = "application/x-tcl";
	_extensionMIMEType["tex"] = "application/x-tex";
	_extensionMIMEType["tfm"] = "application/x-tex-tfm";
	_extensionMIMEType["tei"] = "application/tei+xml";
	_extensionMIMEType["txt"] = "text/plain";
	_extensionMIMEType["dxp"] = "application/vnd.spotfire.dxp";
	_extensionMIMEType["sfs"] = "application/vnd.spotfire.sfs";
	_extensionMIMEType["tsd"] = "application/timestamped-data";
	_extensionMIMEType["tpt"] = "application/vnd.trid.tpt";
	_extensionMIMEType["mxs"] = "application/vnd.triscape.mxs";
	_extensionMIMEType["t"] = "text/troff";
	_extensionMIMEType["tra"] = "application/vnd.trueapp";
	_extensionMIMEType["ttf"] = "application/x-font-ttf";
	_extensionMIMEType["ttl"] = "text/turtle";
	_extensionMIMEType["umj"] = "application/vnd.umajin";
	_extensionMIMEType["uoml"] = "application/vnd.uoml+xml";
	_extensionMIMEType["unityweb"] = "application/vnd.unity";
	_extensionMIMEType["ufd"] = "application/vnd.ufdl";
	_extensionMIMEType["uri"] = "text/uri-list";
	_extensionMIMEType["utz"] = "application/vnd.uiq.theme";
	_extensionMIMEType["ustar"] = "application/x-ustar";
	_extensionMIMEType["uu"] = "text/x-uuencode";
	_extensionMIMEType["vcs"] = "text/x-vcalendar";
	_extensionMIMEType["vcf"] = "text/x-vcard";
	_extensionMIMEType["vcd"] = "application/x-cdlink";
	_extensionMIMEType["vsf"] = "application/vnd.vsf";
	_extensionMIMEType["wrl"] = "model/vrml";
	_extensionMIMEType["vcx"] = "application/vnd.vcx";
	_extensionMIMEType["mts"] = "model/vnd.mts";
	_extensionMIMEType["vtu"] = "model/vnd.vtu";
	_extensionMIMEType["vis"] = "application/vnd.visionary";
	_extensionMIMEType["viv"] = "video/vnd.vivo";
	_extensionMIMEType["ccxml"] = "application/ccxml+xml,";
	_extensionMIMEType["vxml"] = "application/voicexml+xml";
	_extensionMIMEType["src"] = "application/x-wais-source";
	_extensionMIMEType["wbxml"] = "application/vnd.wap.wbxml";
	_extensionMIMEType["wbmp"] = "image/vnd.wap.wbmp";
	_extensionMIMEType["wav"] = "audio/x-wav";
	_extensionMIMEType["davmount"] = "application/davmount+xml";
	_extensionMIMEType["woff"] = "application/x-font-woff";
	_extensionMIMEType["wspolicy"] = "application/wspolicy+xml";
	_extensionMIMEType["webp"] = "image/webp";
	_extensionMIMEType["wtb"] = "application/vnd.webturbo";
	_extensionMIMEType["wgt"] = "application/widget";
	_extensionMIMEType["hlp"] = "application/winhlp";
	_extensionMIMEType["wml"] = "text/vnd.wap.wml";
	_extensionMIMEType["wmls"] = "text/vnd.wap.wmlscript";
	_extensionMIMEType["wmlsc"] = "application/vnd.wap.wmlscriptc";
	_extensionMIMEType["wpd"] = "application/vnd.wordperfect";
	_extensionMIMEType["stf"] = "application/vnd.wt.stf";
	_extensionMIMEType["wsdl"] = "application/wsdl+xml";
	_extensionMIMEType["xbm"] = "image/x-xbitmap";
	_extensionMIMEType["xpm"] = "image/x-xpixmap";
	_extensionMIMEType["xwd"] = "image/x-xwindowdump";
	_extensionMIMEType["der"] = "application/x-x509-ca-cert";
	_extensionMIMEType["fig"] = "application/x-xfig";
	_extensionMIMEType["xhtml"] = "application/xhtml+xml";
	_extensionMIMEType["xml"] = "application/xml";
	_extensionMIMEType["xdf"] = "application/xcap-diff+xml";
	_extensionMIMEType["xenc"] = "application/xenc+xml";
	_extensionMIMEType["xer"] = "application/patch-ops-error+xml";
	_extensionMIMEType["rl"] = "application/resource-lists+xml";
	_extensionMIMEType["rs"] = "application/rls-services+xml";
	_extensionMIMEType["rld"] = "application/resource-lists-diff+xml";
	_extensionMIMEType["xslt"] = "application/xslt+xml";
	_extensionMIMEType["xop"] = "application/xop+xml";
	_extensionMIMEType["xpi"] = "application/x-xpinstall";
	_extensionMIMEType["xspf"] = "application/xspf+xml";
	_extensionMIMEType["xul"] = "application/vnd.mozilla.xul+xml";
	_extensionMIMEType["xyz"] = "chemical/x-xyz";
	_extensionMIMEType["yaml"] = "text/yaml";
	_extensionMIMEType["yang"] = "application/yang";
	_extensionMIMEType["yin"] = "application/yin+xml";
	_extensionMIMEType["zir"] = "application/vnd.zul";
	_extensionMIMEType["zip"] = "application/zip";
	_extensionMIMEType["zmm"] = "application/vnd.handheld-entertainment+xml";
	_extensionMIMEType["zaz"] = "application/vnd.zzazz.deck+xml";
}
