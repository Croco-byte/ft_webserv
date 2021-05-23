#include "Route.hpp"

Route::Route()
 : _require_auth(false)
{

}

Route::Route(const Route &x)
{
	_accepted_methods = x._accepted_methods;
	_auto_index = x._auto_index;
	_route = x._route;
	_local_url = x._local_url;
	_cgi_bin = x._cgi_bin;
	_cgi_extensions = x._cgi_extensions;
	_upload_dir = x._upload_dir;
	_require_auth = x._require_auth;
	_auth_id = x._auth_id;
	_auth_pass = x._auth_pass;
}

Route::~Route()
{

}

void	Route::setAcceptedMethods(std::vector<std::string> vec)
{
	_accepted_methods = vec;
}

void	Route::addAcceptedMethod(std::string method)
{
	_accepted_methods.push_back(method);
}

void	Route::setRequireAuth(bool on)
{
	_require_auth = on;
}

void	Route::enableRequireAuth()
{
	_require_auth = true;
}

void	Route::disableRequireAuth()
{
	_require_auth = false;
}

void	Route::setAutoIndex(bool on)
{
	_auto_index = on;
}

void	Route::enableAutoIndex()
{
	_auto_index = true;
}

void	Route::disableAutoIndex()
{
	_auto_index = false;
}

void	Route::setIndex(std::string index)
{
	_index = index;
}

void	Route::setRoute(std::string route)
{
	_route = route;
}

void	Route::setLocalURL(std::string url)
{
	_local_url = url;
}

void	Route::setCGIBinary(std::string path)
{
	_cgi_bin = path;
}

void	Route::setCGIExtensions(std::vector<std::string> ext)
{
	_cgi_extensions = ext;
}

void	Route::addCGIExtension(std::string ext)
{
	_cgi_extensions.push_back(ext);
}

void	Route::setUploadDir(std::string dir)
{
	_upload_dir = dir;
}

void	Route::setAuthId(std::string id)
{
	_auth_id = id;
	_require_auth = true;
}

void	Route::setAuthPass(std::string pass)
{
	_auth_pass = pass;
}

std::vector<std::string>	Route::getAcceptedMethods() const
{
	return (_accepted_methods);
}

std::vector<std::string>	Route::getCGIExtensions() const
{
	return (_cgi_extensions);
}

bool						Route::requireAuth() const
{
	return (_require_auth);
}

bool						Route::autoIndex() const
{
	return (_auto_index);
}

std::string					Route::getIndex() const
{
	return (_index);
}

std::string					Route::getRoute() const
{
	return (_route);
}

std::string					Route::getLocalURL() const
{
	return (_local_url);
}

std::string					Route::getCGIBinary() const
{
	return (_cgi_bin);
}

std::string					Route::getUploadDir() const
{
	return (_upload_dir);
}

std::string					Route::getAuthId() const
{
	return (_auth_id);
}

std::string					Route::getAuthPass() const
{
	return (_auth_pass);
}

std::ostream	&operator<<(std::ostream &stream, Route &route)
{
	stream	<< "\033[0;33m" << std::endl
			<< "############## Route " << route.getRoute() << " ##############" << std::endl
			<< "\033[0m"
			<< "    - accepted methods      : " << Utils::join(route.getAcceptedMethods()) << std::endl
			<< "    - cgi extensions        : " << Utils::join(route.getCGIExtensions()) << std::endl
			<< "    - require auth          : " << Utils::colorify(route.requireAuth()) << std::endl
			<< "    - auto index            : " << Utils::colorify(route.autoIndex()) << std::endl
			<< "    - index                 : " << route.getIndex() << std::endl
			<< "    - route                 : " << route.getRoute() << std::endl
			<< "    - local URL             : " << route.getLocalURL() << std::endl
			<< "    - CGI Binary            : " << route.getCGIBinary() << std::endl
			<< "    - upload dir            : " << route.getUploadDir() << std::endl
			<< "    - auth id               : " << route.getAuthId() << std::endl
			<< "    - auth pass             : " << route.getAuthPass() << std::endl;
	return (stream);
}
