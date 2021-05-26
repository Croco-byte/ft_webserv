#ifndef SERVER_HPP
# define SERVER_HPP

# include "include.hpp"
# include "Request.hpp"
# include "Response.hpp"
# include "Console.hpp"
# include "ServerConfiguration.hpp"
# include "Route.hpp"
# include "CGI.hpp"

class Server
{
	public:
		Server();
		Server(const Server &x);
		~Server();
	
		/* SETUP | CLEAN SERVER */
		int						setup(void);
		void					clean(void);

		/* HANDLE INCOMING OR OUTGOING CONNEXIONS */
		long					send(long socket);
		long					recv(long socket);
		long					accept(void);

		/* GETTERS | SETTERS */
		long					getFD(void) const;
		void					load(ServerConfiguration config);
		ServerConfiguration		getConfiguration() const;

		/* GET ERROR PAGES */
		std::string				get404Page(void);


	private:
		ServerConfiguration				_config;
		long							_fd;
		t_sockaddr_in					_addr;
		std::map<long, std::string>		_requests;

		/* PRIVATE HELPERS : RESPONSE BODY HANDLERS */
		std::string						generateResponseBody(Request const & request);

		/* PRIVATE HELPERS : HEADER HANDLERS */
		void							handleRequestHeaders(Request request, Response &reponse);
		void							handleCharset(std::vector<std::string> vecCharset, Response &response);
		void							handleLanguage(Request request, std::vector<std::string> vecLang, Response &response);
		void							handleAuthorization(Request request, std::vector<std::string> vecData, Response &response);

		/* PRIVATE HELPERS : URL HANDLERS */
		Route							findCorrespondingRoute(std::string url);
		std::string						getLocalPath(Request request, Route route);

		/* PRIVATE HELPERS : CGI HANDLERS */
		std::string						execCGI(Request request);											// Make it return a string, which should be the output of the CGI binary
		bool							requestRequireCGI(Request request, Route route);
		void							generateMetaVariables(CGI &cgi, Request &request, Route &route);

};

#endif
