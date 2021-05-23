#include "Request.hpp"

Request::Request()
{
	mustClose = false;
}

void						Request::load(std::string request)
{
	std::vector<std::string>	line;
	std::vector<std::string>	tmpVec;
	unsigned int				row;

	request_lines = Utils::split(request, "\n");
	
	if (request_lines.size() <= 1)
		return ;
	
	row = 0;

	// Set Request Line
	line = Utils::split(request_lines[0], " ");
	method = line[0];
	URL = line[1];
	version = line[2];

	std::cout << "------ Request processing info ------" << std::endl << "method = " << method << std::endl << "URL = " << URL << std::endl << "version = " << version << std::endl;

	row = 1;

	// Set headers
	for (unsigned int i = row; i < request_lines.size() && !Utils::is_empty(request_lines[i]); i++)
	{
		std::string name;
		std::string value;

		name = request_lines[i].substr(0, request_lines[i].find(":"));
		value = request_lines[i].substr(request_lines[i].find(":") + 1, request_lines[i].length());

		name = Utils::trim(name);
		value = Utils::trim(value);

		headers[name] = value;
		row++;
	}

	row++;

	if (request_lines.size() > row)
	{
		tmpVec = Utils::split(request_lines[row], "&");
		for (unsigned int i = 0; i < tmpVec.size(); i++)
		{
			std::string name;
			std::string value;

			name = tmpVec[i].substr(0, tmpVec[i].find("="));
			value = tmpVec[i].substr(tmpVec[i].find("=") + 1, tmpVec[i].length());

			name = Utils::trim(name);
			value = Utils::trim(value);

			data[name] = value;
		}
		data_str = request_lines[row];
	}
}

void						Request::print()
{
	std::cout << "--------------------------------------------------------" << std::endl;
	for (unsigned int i = 0; i < request_lines.size(); i++)
		std::cout << request_lines[i] << std::endl;
	std::cout << "--------------------------------------------------------" << std::endl;
}

std::vector<std::string>	Request::getRequestLines() const
{
	return request_lines;
}

std::string					Request::getMethod() const
{
	return method;
}

std::string			Request::getURL() const
{
	return URL;
}

std::string			Request::getProtocolVersion() const
{
	return version;
}

DoubleString		Request::getHeaders() const
{
	return headers;
}

std::string			Request::getBody() const
{
	return (data_str);
}

void				Request::setIP(std::string ip)
{
	_ip = ip;
}

std::string			Request::getIP() const
{
	return (_ip);
}
