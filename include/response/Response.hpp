#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include "utils/include.hpp"

class Response
{
	public:
		Response();
		~Response();

		void			setStatus(int status);
		void			setHeader(std::string const & name, std::string const & value);
		void			setBody(std::string const & content);

		std::string		getBody() const;

		int				getStatus(void) const;

		std::string		build(void);

	private:
		void						initResponseCodeExplications();

		std::string					_protocol;
		int							_status;
		DoubleString				_headers;
		std::string					_body;

		std::map<int, std::string>	_code_explications;
};

#endif
