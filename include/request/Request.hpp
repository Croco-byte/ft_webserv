#ifndef REQUEST_HPP
# define REQUEST_HPP

# include "utils/include.hpp"

class Request
{
	public:
		Request();

		void									parseLang(void);
		void									load(std::string request);
		void									print();
		void									handleHeader(std::string header, std::string value);
		void									setIP(std::string ip);
		bool									hasAuthHeader(void) const;

		std::vector<std::string>				getRequestLines() const;
		std::string								getMethod() const;
		std::string								getURL() const;
		std::string								getProtocolVersion() const;
		DoubleString							getHeaders() const;
		std::string								getBody() const;
		std::string								getIP() const;
		std::string								getQueryString();

		void									getRequestBody(std::string const & request);


	private:

		std::string								generateQueryString(std::string line);

		std::vector<std::string>				request_lines;

		std::string								method;
		std::string								URL;
		std::string								version;
		std::string								_ip;
		std::map<float, std::string>			_lang;

		DoubleString							headers;

		DoubleString							data;
		std::string								data_str;
		std::string								query_string;
		std::string								_body;
		bool									mustClose;
};

#endif
