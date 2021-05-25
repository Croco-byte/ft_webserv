#ifndef CGI_HPP
# define CGI_HPP

# include "include.hpp"

class CGI
{
	public:
		CGI();
		CGI(const CGI &x);
		virtual ~CGI();

		void	setBinary(std::string path);
		void	execute();
		void	addMetaVariable(std::string name, std::string value);

	private:
		char			**doubleStringToChar(DoubleString map);

		DoubleString	_metaVariables;
		DoubleString	_envVariables;
		DoubleString	_argvVariables;
		std::string		_binary;

};

#endif