#ifndef CGI_HPP
# define CGI_HPP

# include "utils/include.hpp"

class CGI
{
	public:
		CGI();
		CGI(const CGI &x);
		virtual ~CGI();

		void		setBinary(std::string path);
		void		execute(std::string target);
		void		addMetaVariable(std::string name, std::string value);

		std::string	getOutput();

	private:
		char			**doubleStringToChar(DoubleString map);

		DoubleString	_metaVariables;
		DoubleString	_envVariables;
		DoubleString	_argvVariables;
		std::string		_binary;
		std::string		_output;

};

#endif
