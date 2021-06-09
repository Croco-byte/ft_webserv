#ifndef SERVER_CONFIGURATION_HPP
# define SERVER_CONFIGURATION_HPP

# include "utils/include.hpp"
# include "server/Route.hpp"

class	ServerConfiguration
{
	public:
		ServerConfiguration();
		ServerConfiguration(const ServerConfiguration &cpy);
		~ServerConfiguration();

		void									setDefault(bool isDefault);
		void									setName(std::string const & name);
		void									setHost(std::string const & host);
		void									addCustomErrorPage(int code, std::string location);
		void									setPort(int port);
		void									setServerRoot(std::string const & server_root);
		void									addRoute(Route route);

		std::string const &						getName()														const;
		std::map<int, std::string> const &		getErrors(void)													const;
		std::string const &						getHost()														const;
		int										getPort()														const;
		std::string const &						getServerRoot(void)												const;
		std::vector<Route> const &				getRoutes()														const;
		bool									isDefault(void)													const;
		bool									hasRootRoute(void)												const;

		std::string								getErrorExplanation(int code)									const;
		std::string								getErrorPage(int code)											const;

	private:
		bool						_is_default;
		std::string					_host;
		std::string					_name;
		std::map<int, std::string>	_errors;
		std::map<int, std::string>	_error_content;
		std::string					_default_error_content;
		int							_port;
		std::string					_server_root;
		std::vector<Route>			_vecRoutes;


		/* PRIVATE HELPERS : INITIALISING ERROR EXPLICATIONS AND CONTENT */
		void						init_errors(void);
		void						init_default_error_content(void);
};

std::ostream	&operator<<(std::ostream &stream, ServerConfiguration const & conf);


#endif
