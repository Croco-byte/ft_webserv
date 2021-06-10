/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfiguration.cpp                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: user42 <user42@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/05/27 17:58:54 by user42            #+#    #+#             */
/*   Updated: 2021/06/10 13:56:47 by user42           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server/ServerConfiguration.hpp"

ServerConfiguration::ServerConfiguration() : _is_default(true), _host("127.0.0.1"), _name(""), _port(8080), _server_root("/")
{
	_default_error_content = "<html>\r\n"
	"<head><title>error_code</title></head>\r\n"
	"<body bgcolor=\"white\">\r\n"
	"<center><h1>error_code</h1></center>"
	"<hr><center>Webserv/1.0.0 (Ubuntu)</center></body></html>";
	this->init_errors();
	this->init_default_error_content();
}

ServerConfiguration::ServerConfiguration(const ServerConfiguration &x)
{
	_is_default = x._is_default;
	_name = x._name;
	_host = x._host;
	_errors = x._errors;
	_error_content = x._error_content;
	_default_error_content = x._default_error_content;
	_port = x._port;
	_server_root = x._server_root;
	_vecRoutes = x._vecRoutes;
}

ServerConfiguration::~ServerConfiguration()
{}


/*
** ------ PRIVATE HELPERS : INITIALISING ERROR EXPLICATIONS AND CONTENT ------
*/
void									ServerConfiguration::init_errors(void)
{
	_errors[200] = "OK";
	_errors[201] = "Created";
	_errors[202] = "Accepted";
	_errors[204] = "No Content";
	_errors[300] = "Multiple Choices";
	_errors[301] = "Moved Permanently";
	_errors[302] = "Found";
	_errors[310] = "Too many Redirects";
	_errors[400] = "Bad request";
	_errors[401] = "Unauthorized";
	_errors[403] = "Forbidden";
	_errors[404] = "Not Found";
	_errors[405] = "Method Not Allowed";
	_errors[406] = "Non acceptable";
	_errors[409] = "Conflict";
	_errors[413] = "Request Entity Too Large";
	_errors[500] = "Internal Server Error";
	_errors[501] = "Not Implemented";
	_errors[502] = "Bad Gateway";
	_errors[503] = "Service Unavailable";
	_errors[505] = "HTTP Version Not Supported";
}

void									ServerConfiguration::init_default_error_content(void)
{
	for (std::map<int, std::string>::iterator it = _errors.begin(); it != _errors.end(); it++)
		_error_content[it->first] = Utils::replace(_default_error_content, "error_code", (Utils::to_string(it->first) + " " + it->second));
}

void									ServerConfiguration::setDefault(bool isDefault)
{ _is_default = isDefault; }

void									ServerConfiguration::setName(std::string const & name)
{ _name = name; }

void									ServerConfiguration::setHost(std::string const & host)
{ _host = host; }

void									ServerConfiguration::addCustomErrorPage(int code, std::string location)
{
	std::string customPath = (location[0] == '/') ? _server_root + location.substr(1) : _server_root + location;
	std::string customContent = Utils::getFileContent(customPath);
	if (customContent.empty())
		return ;
	_error_content[code] = customContent;
}

void									ServerConfiguration::setPort(int port)
{ _port = port; }

void									ServerConfiguration::setServerRoot(std::string const & server_root)
{ _server_root = (server_root[server_root.size() - 1] == '/') ? server_root : server_root + '/'; }

void									ServerConfiguration::addRoute(Route route)
{ _vecRoutes.push_back(route); }

std::string const &						ServerConfiguration::getName() const
{ return (_name); }

std::map<int, std::string> const &		ServerConfiguration::getErrors(void) const
{ return (_errors); }


std::string const &						ServerConfiguration::getHost() const
{ return (_host); }

int										ServerConfiguration::getPort() const
{ return (_port); }

std::string const &						ServerConfiguration::getServerRoot() const
{ return (_server_root); }

std::vector<Route> const &				ServerConfiguration::getRoutes() const
{ return (_vecRoutes); }

bool									ServerConfiguration::isDefault(void) const
{ return (_is_default); }

bool									ServerConfiguration::hasRootRoute(void) const
{
	if (_vecRoutes.empty())
		return (false);
	for (std::vector<Route>::const_iterator it = _vecRoutes.begin(); it != _vecRoutes.end(); it++)
	{
		if ((*it).getRoute() == "/")
			return (true);
	}
	return (false);
}

std::string								ServerConfiguration::getErrorExplanation(int code) const
{
	std::map<int, std::string>::const_iterator it = _errors.find(code);
	if (it != _errors.end())
		return (it->second);
	return ("");
}

std::string								ServerConfiguration::getErrorPage(int code) const
{
	std::map<int, std::string>::const_iterator it = _error_content.find(code);
	if (it != _errors.end())
		return (it->second);
	return ("");
}



std::ostream	& operator<<(std::ostream &stream, ServerConfiguration const & conf)
{
	stream << "\033[0;33m" << std::endl;
	stream << ">>>>>>>>>>>>>> Server configuration <<<<<<<<<<<<<<" << std::endl;
	stream << "\033[0m";
	stream << "    - listen				: " << conf.getPort() << std::endl;
	stream << "    - host				: " << conf.getHost() << std::endl;
	stream << "    - server_name		: " << conf.getName() << std::endl;
	stream << "    - server_root		: " << conf.getServerRoot() << std::endl;

	for (std::vector<Route>::const_iterator it = (conf.getRoutes()).begin(); it != (conf.getRoutes()).end(); it++)
		std::cout << *it << std::endl;

	return (stream);
}
