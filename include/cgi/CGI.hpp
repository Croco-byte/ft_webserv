#ifndef CGI_HPP
# define CGI_HPP

# include "utils/include.hpp"
# include "request/Request.hpp"

class CGI
{
	public:
		CGI();
		CGI(const CGI &x);
		virtual ~CGI();

		void		setBinary(std::string path);
		void		setInput(std::string content);
		int			minipipe(void);
		void		execute(std::string target);
		void		addMetaVariable(std::string name, std::string value);
		void		convertHeadersToMetaVariables(Request request);

		std::string	getOutput();

	private:
		char			**doubleStringToChar(DoubleString map);

		DoubleString	_metaVariables;
		DoubleString	_envVariables;
		DoubleString	_argvVariables;
		std::string		_binary;
		std::string		_output;
		std::string		_input;

		int				_stdin;
		int				_stdout;
		int				_pipin;
		int				_pipout;
};

#endif
