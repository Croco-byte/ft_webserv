/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: user42 <user42@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/05/27 17:58:32 by user42            #+#    #+#             */
/*   Updated: 2021/05/28 12:44:28 by user42           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "response/Response.hpp"

Response::Response() : _protocol("HTTP/1.1"), _status(200)
{
	this->initResponseCodeExplications();
	this->setHeader("Date", Utils::get_current_time());
	this->setHeader("Server", "Webserv/1.0 (Ubuntu)");
	this->setHeader("Content-Length", "0");
}

Response::~Response()
{}

void		Response::initResponseCodeExplications()
{
	_code_explications[200] = "OK";
	_code_explications[201] = "Created";
	_code_explications[202] = "Accepted";
	_code_explications[204] = "No Content";
	_code_explications[300] = "Multiple Choices";
	_code_explications[301] = "Moved Permanently";
	_code_explications[302] = "Found";
	_code_explications[310] = "Too many Redirects";
	_code_explications[400] = "Bad request";
	_code_explications[401] = "Unauthorized";
	_code_explications[403] = "Forbidden";
	_code_explications[404] = "Not Found";
	_code_explications[405] = "Method Not Allowed";
	_code_explications[406] = "Non acceptable";
	_code_explications[413] = "Request Entity Too Large";
	_code_explications[500] = "Internal Server Error";
	_code_explications[501] = "Not Implemented";
	_code_explications[502] = "Bad Gateway";
	_code_explications[503] = "Service Unavailable";
}

void		Response::setStatus(int status)
{ _status = status; }

void		Response::setHeader(std::string const & name, std::string const & value)
{ _headers[name] = value; }

void		Response::setBody(std::string const & text)
{ _body = text; }

int			Response::getStatus() const
{ return (_status); }

std::string	Response::build()
{
	std::string	status_line;
	std::string	headers;
	std::string	response;

	/**
	 * VERSION-HTTP CODE EXPLICATION-CODE <crlf>
	 */
	status_line = _protocol + " " + Utils::to_string(_status) + " " + _code_explications[_status] + ENDL;

	/**
	 * EN-TETE : Valeur <crlf>...
	 */
	for (DoubleString::iterator it = _headers.begin(); it != _headers.end(); it++)
		headers.append(it->first + ": " + it->second + ENDL);

	/**
	 * Build response
	 */
	response = status_line;
	response.append(headers);
	response.append(ENDL);					// Empty line between headers and body
	response.append(_body);

	return (response);
}

std::string		Response::getBody() const
{
	return (_body);
}
