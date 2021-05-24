#ifndef ROUTE_HPP
# define ROUTE_HPP

#include "include.hpp"

/**
 * Class Route represent a route in a server (ex: /abcde)
 * Ex: if URL /abc/ 			 /tmp/www/test 		=> /abc/pouic/toto/pouet = /tmp/www/test/pouic/toto/pouet
 * 				|						|
 * 				|						|
 * 				|						|
 * 			  _route				_local_url
 */

class	Route
{
	public:
		Route();
		Route(const Route &route);
		virtual ~Route();

		void	setAcceptedMethods(std::vector<std::string> vec);
		void	addAcceptedMethod(std::string method);
		void	setRequireAuth(bool on);
		void	enableRequireAuth();
		void	disableRequireAuth();
		void	setAutoIndex(bool on);
		void	enableAutoIndex();
		void	disableAutoIndex();
		void	setIndex(std::string index);
		void	setRoute(std::string route);
		void	setLocalURL(std::string url);
		void	setCGIBinary(std::string path);
		void	setCGIExtensions(std::vector<std::string> ext);
		void	addCGIExtension(std::string ext);
		void	setUploadDir(std::string dir);
		void	setAuthId(std::string id);
		void	setAuthPass(std::string pass);

		std::vector<std::string>	getAcceptedMethods() const;
		std::vector<std::string>	getCGIExtensions() const;
		bool						requireAuth() const;
		bool						autoIndex() const;
		std::string					getIndex() const;
		std::string					getRoute() const;
		std::string					getLocalURL() const;
		std::string					getCGIBinary() const;
		std::string					getUploadDir() const;
		std::string					getAuthId() const;
		std::string					getAuthPass() const;

	private:
		std::vector<std::string>	_accepted_methods;
		std::vector<std::string>	_cgi_extensions;
		bool						_require_auth;
		bool						_auto_index;
		std::string					_index;
		std::string					_route;
		std::string					_local_url;
		std::string					_cgi_bin;
		std::string					_upload_dir;
		std::string					_auth_id;
		std::string					_auth_pass;
};

std::ostream	&operator<<(std::ostream &stream, Route const & route);

#endif
