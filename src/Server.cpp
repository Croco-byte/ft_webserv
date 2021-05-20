#include "Server.hpp"

Server::Server()
{

}

Server::Server(const Server &x)
{
	_sock = x._sock;
	_sin = x._sin;
	_csock = x._csock;
	_csin = x._csin;
	_taille = x._taille;
	_vecClients = x._vecClients;
	_isRunning = x._isRunning;
	_config = x._config;
}

Server::~Server()
{

}

void	Server::load(ServerConfiguration conf)
{
	_config = conf;
}

void	Server::init()
{
	_isRunning = false;
	_sock = socket(AF_INET, SOCK_STREAM, 0);						// TCP, TCP, 0
	_sin.sin_addr.s_addr = htonl(INADDR_ANY);						// IP
	_sin.sin_family = AF_INET;										// TCP
	_sin.sin_port = htons(_config.getPort());
	_taille = sizeof(_csin);

	if (_sock == SOCKET_ERROR)
		Utils::quit("Error while creating socket");

	if (bind(_sock, (t_sockaddr*)&_sin, sizeof(_sin)) == SOCKET_ERROR)
	{
		Console::error("Error while attributing addr and port to socket on server " + _config.getHost() + ":" + std::to_string(_config.getPort()));
		perror(" =>");
		exit(1);
	}
	Console::info("Server " + _config.getName() + " initialized successfully");
}

void	Server::start()
{
	if (listen(_sock, 5) == SOCKET_ERROR)
		Utils::quit("Error socket could not listen.");
	
	std::cout << "Server started on port : " << _config.getPort() << std::endl;

	_isRunning = true;
}

void	Server::loop()
{
	fcntl(_sock, F_SETFL, O_NONBLOCK);
	_csock = accept(_sock, (t_sockaddr*)&_csin, &_taille);
	if (_csock != -1)
	{
		std::cout << "new client with fd " << _csock << std::endl;
		_vecClients.push_back(new Client(_csock, _csin));
	}

	for (unsigned int i = 0; i < _vecClients.size(); i++)
	{
		if (_vecClients[i]->waitRequest())
		{
			this->handleRequest(_vecClients[i]);
			if 	(!this->_isRunning)
				break ;
		}
	}
}

/**
 * Traite la requête, génère la réponse, et l'envoie au client
 * @param Client
 */
void	Server::handleRequest(Client *client)
{
	std::string	request_str;
	Request		request;
	Response	response;

	request_str = client->getRequest();
	request.load(request_str);
	request.setIP(client->getIP());

	this->handleRequestHeaders(request, response);
	this->handleRequestURL(request, response);

	std::string body = "<html><body><h1>YASSSS</h1></body></html>";
	response.setHeader("Content-Length", std::to_string(body.length()));
	response.setBody(body);
	client->sendResponse(response.build());
}

/**
 * Traite les headers de la requête
 * @param Request
 * @param Response
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

void	Server::stop()
{
	for (unsigned int i = 0; i < _vecClients.size(); i++)
			_vecClients[i]->closeConnection();
	_vecClients.clear();
	close(_sock);
	_isRunning = false;
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
		Console::error("Authorization header bad formatted");
		return ;
	}

	type = Utils::to_lower(vecData[0]);
	decoded = Utils::base64_decode(vecData[1]);
	vecCredentials = Utils::split(decoded, ":");
	id = vecCredentials[0];
	password = vecCredentials[1];
	Console::info("Try to connect with : " + id + ":" + password);

}

ServerConfiguration	Server::getConfiguration() const
{
	return (_config);
}

void		Server::handleRequestURL(Request request, Response &response)
{
	Route	route;
	// bool	is_directory = false;
	// bool	has_extension = false;
	// bool	must_execut_cgi = false;

	(void)response;
	route = this->findCorrespondingRoute(request.getURL());
	if (this->requestRequireCGI(request, route))
	{
		Console::info("Request require CGI !");
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
		Console::info("No need to execute the CGI.");
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
	Console::info("LOCAL PATH => " + localPath);
	return (localPath);
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
	cgi.addMetaVariable("GATEWAY_INTERFACE", "CGI/1.1");
	cgi.addMetaVariable("SERVER_NAME", this->_config.getName());
	cgi.addMetaVariable("SERVER_SOFTWARE", "webserv/1.0");
	cgi.addMetaVariable("SERVER_PROTOCOL", "HTTP/1.1");											// PEUT ETRE 1.0
	cgi.addMetaVariable("SERVER_PORT", std::to_string(this->_config.getPort()));
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
	cgi.addMetaVariable("CONTENT_LENGTH", std::to_string(request.getBody().length()));
	cgi.addMetaVariable("HTTP_ACCEPT", request.getHeaders()["HTTP_ACCEPT"]);
	cgi.addMetaVariable("HTTP_USER_AGENT", request.getHeaders()["User-Agent"]);
	cgi.addMetaVariable("HTTP_REFERER", request.getHeaders()["Referer"]);
}
