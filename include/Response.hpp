#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include "include.hpp"

class Response
{
	public:
		Response();

		void		setStatus(int status);
		void		setHeader(std::string name, std::string value);
		void		setBody(std::string text);

		int			getStatus() const;

		std::string	build();

	private:
		void						initResponseCodeExplications();

		std::string					_protocol;
		int							_status;
		DoubleString				_headers;
		std::string					_body;

		std::map<int, std::string>	_code_explications;
};

#endif
