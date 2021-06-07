/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: user42 <user42@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/05/27 17:58:32 by user42            #+#    #+#             */
/*   Updated: 2021/06/07 13:41:49 by user42           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "response/Response.hpp"

Response::Response() : _protocol("HTTP/1.1"), _status(200)
{
	this->setHeader("Date", Utils::get_current_time());
	this->setHeader("Server", "Webserv/1.0 (Ubuntu)");
	this->setHeader("Content-Length", "0");
}

Response::~Response()
{}

void		Response::setStatus(int status)
{ _status = status; }

void		Response::setHeader(std::string const & name, std::string const & value)
{ _headers[name] = value; }

void		Response::setBody(std::string const & text)
{ _body = text; }

int			Response::getStatus() const
{ return (_status); }

std::string	Response::build(std::map<int, std::string> const & errors)
{
	std::string	status_line;
	std::string	headers;
	std::string	response;

	status_line = _protocol + " " + Utils::to_string(_status) + " " + (errors.find(_status))->second + ENDL;
	for (DoubleString::iterator it = _headers.begin(); it != _headers.end(); it++)
	{
		if (!it->second.empty())
			headers.append(it->first + ": " + it->second + ENDL);
	}
	response = status_line;
	response.append(headers);
	response.append(ENDL);
	response.append(_body);

	return (response);
}

std::string		Response::getBody() const
{
	return (_body);
}
