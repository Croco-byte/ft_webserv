#ifndef REQUEST_HPP
# define REQUEST_HPP

# include "utils/include.hpp"

class Request
{
	public:
		Request();

		void						load(std::string request);
		void						print();
		void						handleHeader(std::string header, std::string value);
		void						setIP(std::string ip);

		std::vector<std::string>	getRequestLines() const;
		std::string					getMethod() const;
		std::string					getURL() const;
		std::string					getProtocolVersion() const;
		DoubleString				getHeaders() const;
		std::string					getBody() const;
		std::string					getIP() const;
		std::string					getQueryString();

	private:

		std::string					generateQueryString(std::string line);

		std::vector<std::string>	request_lines;

		std::string					method;
		std::string					URL;
		std::string					version;
		std::string					_ip;

		// std::map<std::string, std::string>
		DoubleString				headers;

		// data["name"] = "test"
		DoubleString				data;
		// name=test&password=pass
		std::string					data_str;
		std::string					query_string;
		bool						mustClose;
};

#endif
