/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Route.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: user42 <user42@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/05/27 17:58:43 by user42            #+#    #+#             */
/*   Updated: 2021/05/28 13:11:49 by user42           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server/Route.hpp"

Route::Route()
 : _require_auth(false)
{}

Route::Route(const Route &x)
{
	_accepted_methods = x._accepted_methods;
	_auto_index = x._auto_index;
	_index = x._index;
	_route = x._route;
	_local_url = x._local_url;
	_cgi_bin = x._cgi_bin;
	_cgi_extensions = x._cgi_extensions;
	_upload_dir = x._upload_dir;
	_require_auth = x._require_auth;
	_auth_basic_user_file = x._auth_basic_user_file;
}

Route::~Route()
{}

void								Route::setAcceptedMethods(std::vector<std::string> const & vec)
{ _accepted_methods = vec; }

void								Route::addAcceptedMethod(std::string const & method)
{ _accepted_methods.push_back(method); }

void								Route::setRequireAuth(bool on)
{ _require_auth = on; }

void								Route::enableRequireAuth(void)
{ _require_auth = true; }

void								Route::disableRequireAuth(void)
{ _require_auth = false; }

void								Route::setAutoIndex(bool on)
{ _auto_index = on; }

void								Route::enableAutoIndex(void)
{ _auto_index = true; }

void								Route::disableAutoIndex(void)
{ _auto_index = false; }

void								Route::setIndex(std::string const & index)
{ _index = index; }

void								Route::setRoute(std::string const & route, std::string const & server_root)
{
	_route = route;
	_local_url = (route[0] == '/') ? server_root + route.substr(1) : server_root + route;
}

void								Route::setLocalURL(std::string const & url)
{ _local_url = url; }

void								Route::setCGIBinary(std::string const & path)
{ _cgi_bin = path; }

void								Route::setCGIExtensions(std::vector<std::string> const & ext)
{ _cgi_extensions = ext; }

void								Route::addCGIExtension(std::string const & ext)
{ _cgi_extensions.push_back(ext); }

void								Route::setUploadDir(std::string const & dir)
{ _upload_dir = dir; }

void								Route::setUserFile(std::string const & file)
{ _auth_basic_user_file = file; }


std::vector<std::string> const &	Route::getAcceptedMethods(void) const
{ return (_accepted_methods); }

std::vector<std::string> const &	Route::getCGIExtensions(void) const
{ return (_cgi_extensions); }

bool								Route::requireAuth(void) const
{ return (_require_auth); }

bool								Route::autoIndex(void) const
{ return (_auto_index); }

std::string	const &					Route::getIndex(void) const
{ return (_index); }

std::string const &					Route::getRoute(void) const
{ return (_route); }

std::string const &					Route::getLocalURL(void) const
{ return (_local_url); }

std::string const &					Route::getCGIBinary(void) const
{ return (_cgi_bin); }

std::string const &					Route::getUploadDir(void) const
{ return (_upload_dir); }

std::string const &					Route::getUserFile(void) const
{ return (_auth_basic_user_file); }

std::ostream	&operator<<(std::ostream &stream, Route const & route)
{
	stream	<< "\033[0;33m" << std::endl
			<< "############## Route " << route.getRoute() << " ##############" << std::endl
			<< "\033[0m"
			<< "    - accepted methods      : " << Utils::join(route.getAcceptedMethods()) << std::endl
			<< "    - cgi extensions        : " << Utils::join(route.getCGIExtensions()) << std::endl
			<< "    - require auth          : " << Utils::colorify(route.requireAuth()) << std::endl
			<< "    - auth_user_file        : " << route.getUserFile() << std::endl
			<< "    - auto index            : " << Utils::colorify(route.autoIndex()) << std::endl
			<< "    - index                 : " << route.getIndex() << std::endl
			<< "    - route                 : " << route.getRoute() << std::endl
			<< "    - local URL             : " << route.getLocalURL() << std::endl
			<< "    - CGI Binary            : " << route.getCGIBinary() << std::endl
			<< "    - upload dir            : " << route.getUploadDir() << std::endl;
	return (stream);
}
