#ifndef ROUTE_HPP
# define ROUTE_HPP

#include "utils/include.hpp"

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

		void										setAcceptedMethods(std::vector<std::string> const & vec);
		void										addAcceptedMethod(std::string const & method);
		void										setRequireAuth(bool on);
		void										enableRequireAuth();
		void										disableRequireAuth(void);
		void										setAutoIndex(bool on);
		void										enableAutoIndex(void);
		void										disableAutoIndex(void);
		void										setIndex(std::string const & index);
		void										setRoute(std::string const & route, std::string const & server_root);
		void										setLocalURL(std::string const & url);
		void										setCGIBinary(std::string const & path);
		void										setCGIExtensions(std::vector<std::string> const & ext);
		void										addCGIExtension(std::string const & ext);
		void										setUploadDir(std::string const & dir);
		void										setAuthId(std::string const & id);
		void										setAuthPass(std::string const & pass);

		std::vector<std::string> const &			getAcceptedMethods(void) const;
		std::vector<std::string> const &			getCGIExtensions(void) const;
		bool										requireAuth(void) const;
		bool										autoIndex(void) const;
		std::string const &							getIndex(void) const;
		std::string const &							getRoute(void) const;
		std::string const &							getLocalURL(void) const;
		std::string const &							getCGIBinary(void) const;
		std::string const &							getUploadDir(void) const;
		std::string const &							getAuthId(void) const;
		std::string const &							getAuthPass(void) const;

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
