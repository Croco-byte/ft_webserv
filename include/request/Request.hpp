#ifndef REQUEST_HPP
# define REQUEST_HPP

# include "utils/include.hpp"

class Request
{
	public:
		Request();

		/* -- Request parsing functions -- */
		void									load(std::string request);

		/* -- Getters -- */
		std::string								getMethod() const;
		std::string								getURL() const;
		std::string								getProtocolVersion() const;
		DoubleString							getHeaders() const;
		std::string								getBody() const;
		std::string								getQueryString();
		int										getValidity(void) const;

		/* -- Checkers -- */
		bool									hasAuthHeader(void) const;


	private:
		/* -- Private helpers : parsing utilities -- */
		void									parseRequestFirstLine(std::string const & first_line);
		std::string								generateQueryString(std::string line);
		void									parseRequestHeaders(std::vector<std::string> const & request_lines);
		void									parseLang(void);
		void									parseRequestBody(std::string const & request);
		void									parseRequestChunkedBody(std::string const &request);


		/* -- Class attributes -- */
		std::string								_method;
		std::string								_url;
		std::string								_version;

		std::map<float, std::string>			_lang;
		DoubleString							_headers;

		std::string								_query_string;
		std::string								_body;

		int										_invalid;
};

#endif
