#ifndef SERVER_HPP
# define SERVER_HPP

# include "utils/include.hpp"
# include "request/Request.hpp"
# include "response/Response.hpp"
# include "utils/Console.hpp"
# include "server/ServerConfiguration.hpp"
# include "server/Route.hpp"
# include "cgi/CGI.hpp"
# include "response/AutoIndex.hpp"

class Server
{
	public:
		Server();
		Server(const Server &x);
		~Server();
	
		/* SETUP | CLEAN SERVER */
		int								setup(void);
		void							clean(void);

		/* HANDLE INCOMING OR OUTGOING CONNEXIONS */
		long							send(long socket);
		long							recv(long socket);
		long							accept(void);

		/* GETTERS | SETTERS */
		long							getFD(void) const;
		void							load(ServerConfiguration config);
		ServerConfiguration				getConfiguration() const;


	private:
		ServerConfiguration				_config;
		long							_fd;
		t_sockaddr_in					_addr;
		std::map<long, std::string>		_requests;
		int								_error_code;

		/* PRIVATE HELPERS : RESPONSE BODY HANDLERS */
		std::string						generateResponseBody(Request const & request);

		/* PRIVATE HELPERS : HEADER HANDLERS */
		void							handleRequestHeaders(Request request, Response &reponse);
		void							handleCharset(std::vector<std::string> vecCharset, Response &response);
		void							handleLanguage(Request request, std::vector<std::string> vecLang, Response &response);

		/* PRIVATE HELPERS : URL HANDLERS */
		Route							findCorrespondingRoute(std::string url);
		std::string						getLocalPath(Request request, Route route);

		/* PRIVATE HELPERS : CGI HANDLERS */
		std::string						execCGI(Request request);
		bool							requestRequireCGI(Request request, Route route);
		void							generateMetaVariables(CGI &cgi, Request &request, Route &route);

		/* PRIVATE HELPERS : REDIRECTION HANDLERS*/
		bool							requestRequireRedirection(Request);
		void							generateRedirection(Request request, Response &response);

		/* PRIVATE HELPERS : ERRORS HANDLERS */
		bool							requestIsValid(Request request);
		bool							isMethodAccepted(Request request);
		void							handleRequestErrors(Request request, Response &response);

		/* PRIVATE HELPERS : AUTHORIZATION HANDLERS*/
		void							handleUnauthorizedRequests(Response & response);
		bool							credentialsMatch(std::string const & requestAuthHeader, std::string const & userFile);

};

#endif
