#ifndef SERVER_HPP
# define SERVER_HPP

# include "include.hpp"
# include "Client.hpp"
# include "Request.hpp"
# include "Console.hpp"
# include "ServerConfiguration.hpp"
# include "Route.hpp"
# include "CGI.hpp"

class Server
{
	public:
		Server();
		Server(const Server &x);
		virtual ~Server();
	
		void				load(ServerConfiguration config);
		void				init();
		void				start();
		void				handleRequest(Client *c);
		void				stop();
		void				loop();

		ServerConfiguration	getConfiguration() const;

	private:
		void		handleRequestHeaders(Request request, Response &reponse);
		void		handleCharset(std::vector<std::string> vecCharset, Response &response);
		void		handleLanguage(Request request, std::vector<std::string> vecLang, Response &response);
		void		handleAuthorization(Request request, std::vector<std::string> vecData, Response &response);

		void		handleRequestURL(Request request, Response &response);
		Route		findCorrespondingRoute(std::string url);
		std::string	getLocalPath(Request request, Route route);
		bool		requestRequireCGI(Request request, Route route);
		void		generateMetaVariables(CGI &cgi, Request &request, Route &route);

		t_socket				_sock;
		t_sockaddr_in			_sin;
		t_socket				_csock;
		t_sockaddr_in			_csin;
		socklen_t				_taille;
		std::vector<Client*>	_vecClients;
		bool					_isRunning;

		ServerConfiguration		_config;
};

#endif
