#include "request/Request.hpp"

Request::Request(): _invalid(0)
{}


/*
** ------ REQUEST PARSING FUNCTIONS ------
*/
void						Request::load(std::string request)
{
	std::vector<std::string>	request_lines;

	request_lines = Utils::split(request, "\n");
	if (request_lines.size() <= 1)
	{
		_invalid = 400;
		return ;
	}
	
	this->parseRequestFirstLine(request_lines[0]);
	this->parseRequestHeaders(request_lines);
	this->parseRequestBody(request);
	this->parseLang();

	// Some debugging informations
	std::cout << "------ Request processing info ------" << std::endl << "method = " << _method << std::endl << "URL = " << _url << std::endl << "Version = " << _version << std::endl;
	std::cout << "Accept-Language = ";
	for (std::map<float, std::string>::iterator it = _lang.begin(); it != _lang.end(); it++)
		std::cout << "[" << it->first << "|" << it->second << "] ";
	std::cout << std::endl;	
}


/*
** ------ GETTERS ------
*/
std::string					Request::getMethod() const
{ return _method; }

std::string					Request::getURL() const
{ return _url; }

std::string					Request::getProtocolVersion() const
{ return _version; }

DoubleString				Request::getHeaders() const
{ return _headers; }

std::string					Request::getBody() const
{ return _body; }

std::string					Request::getQueryString()
{ return (_query_string); }

int							Request::getValidity(void) const
{ return (_invalid); }

/*
** ------ CHECKERS ------
*/
bool						Request::hasAuthHeader(void) const
{
	if (this->_headers.find("Authorization") != this->_headers.end())
		return (true);
	return (false);
}


/*
** ------ PRIVATE HELPERS : PARSING UTILITIES ------
*/
void						Request::parseRequestFirstLine(std::string const & first_line)
{
	std::vector<std::string>	line;

	line = Utils::split(first_line, " ");
	if (line.size() != 3)
	{
		_invalid = 400;
		return ;
	}
	_method = line[0];
	_url = Utils::split(line[1], "?")[0];
	_query_string = this->generateQueryString(line[1]);
	_version = Utils::trim(line[2]);
	if (_version != "HTTP/1.1")
		_invalid = 505;
}

std::string					Request::generateQueryString(std::string line)
{
	if (line.find('?') != std::string::npos)
	{
		line.erase(line.begin(), line.begin() + _url.length() + 1);
		Console::info("Query String = " + line);
		return (line);
	}
	else
		return ("");
}

void						Request::parseRequestHeaders(std::vector<std::string> const & request_lines)
{
	for (unsigned int row = 1; row < request_lines.size() && !Utils::is_empty(request_lines[row]); row++)
	{
		std::string name;
		std::string value;

		name = request_lines[row].substr(0, request_lines[row].find(":"));
		value = request_lines[row].substr(request_lines[row].find(":") + 1, request_lines[row].length());

		name = Utils::trim(name);
		value = Utils::trim(value);

		_headers[name] = value;
	}
	if (_headers.find("Host") == _headers.end())	
		_invalid = 400;
}

void						Request::parseLang(void)
{
	std::vector<std::string>	token;
	std::string					header;
	size_t						i;

	if ((header = this->_headers["Accept-Language"]) != "")
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

void						Request::parseRequestBody(std::string const & request)
{
	if (_headers.find("Transfer-Encoding") != _headers.end()
		&& _headers["Transfer-Encoding"] == "chunked")
	{
		this->parseRequestChunkedBody(request);
		return ;
	}
	size_t i = request.find("\r\n\r\n");
	if (i != std::string::npos)
	{
		i += 4;
		std::map<std::string, std::string>::iterator it = _headers.find("Content-Length");
		if (it != _headers.end())
		{
			int j(0);
			while (request[i] && j < atoi(it->second.c_str()))
			{
				_body += request[i];
				i++;
				j++;
			}
		}
	}
}

void				Request::parseRequestChunkedBody(std::string const & request)
{
	std::string	chunks = request.substr(request.find("\r\n\r\n") + 4, request.size() - 1);
	std::string	subchunk = chunks.substr(0, 100);
	int			chunksize = strtol(subchunk.c_str(), NULL, 16);
	size_t		i = 0;

	while (chunksize)
	{
		i = chunks.find("\r\n", i) + 2;
		_body += chunks.substr(i, chunksize);
		i += chunksize + 2;
		subchunk = chunks.substr(i, 100);
		chunksize = strtol(subchunk.c_str(), NULL, 16);
	}
}
