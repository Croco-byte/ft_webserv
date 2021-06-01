#include "request/Request.hpp"

Request::Request()
{
	mustClose = false;
}

void						Request::parseLang(void)
{
	std::vector<std::string>	token;
	std::string					header;
	size_t						i;

	if ((header = this->headers["Accept-Language"]) != "")
	{
		token = Utils::split(header, ",");
		for (std::vector<std::string>::iterator it = token.begin(); it != token.end(); it++)
		{
			float			weight(1.0);
			std::string		lang;

			Utils::trim(*it);
			if ((i = ((*it).find(';'))) == std::string::npos)
				_lang[weight] = *it;
			else
			{
				lang = (*it).substr(0, i);
				weight = atof((*it).substr(i + 3).c_str());
				_lang[weight] = lang;
			}
		}
	}
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
	URL = Utils::split(line[1], "?")[0];
	query_string = this->generateQueryString(line[1]);
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
	this->parseLang();
	std::cout << "Accept-Language = ";
	for (std::map<float, std::string>::iterator it = _lang.begin(); it != _lang.end(); it++)
		std::cout << "[" << it->first << "|" << it->second << "] ";
	std::cout << std::endl;

	this->getRequestBody(request);
}

void						Request::getRequestBody(std::string const & request)
{
	size_t i = request.find("\r\n\r\n");
	if (i != std::string::npos)
	{
		i += 4;
		std::map<std::string, std::string>::iterator it = headers.find("Content-Length");
		if (it != headers.end())
		{
			int j(0);
			while (j < atoi((it->second.c_str())))
			{
				_body += request[i];
				i++;
				j++;
			}
		}
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

bool				Request::hasAuthHeader(void) const
{
	if (this->headers.find("Authorization") != this->headers.end())
		return (true);
	return (false);
}

std::string			Request::getIP() const
{
	return (_ip);
}

std::string			Request::generateQueryString(std::string line)
{
	if (line.find('?') != std::string::npos)
	{
		line.erase(line.begin(), line.begin() + URL.length() + 1);
		Console::info("Query String = " + line);
		return (line);
	}
	else
		return ("");
}

std::string			Request::getQueryString()
{
	return (query_string);
}
