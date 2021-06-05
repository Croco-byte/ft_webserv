#include "cgi/CGI.hpp"

CGI::CGI()
{
	_stdin = dup(STDIN);
	_stdout = dup(STDOUT);
}

CGI::CGI(const CGI &x)
{
	(void)x;
}

CGI::~CGI()
{
	
}

void	CGI::setBinary(std::string path)
{
	_binary = path;
}

void	CGI::setInput(std::string content)
{
	_input = content;
}

int		CGI::minipipe(void)
{
	pid_t		pid;
	int			pipefd[2];

	pipe(pipefd);
	pid = fork();
	if (pid == 0)
	{
		dup2(pipefd[0], STDIN_FILENO);
		dup2(pipefd[1], STDOUT_FILENO);
		_pipout = pipefd[1];
		_pipin = pipefd[0];
		return (2);
	}
	else
	{
		_pipin = pipefd[0];
		_pipout = pipefd[1];
		return (1);
	}
}

void	CGI::execute(std::string target)
{
	int ret(0);
	int pipe = minipipe();
	if (pipe == 2)
	{
		pid_t	pid;

		pid = fork();
		if (pid == 0)
		{
			if (_input.empty())
			{
				std::cerr << "EMPTY" << std::endl;
				_input = " ";
				this->addMetaVariable("CONTENT_LENGTH", "1");
			}
			std::cerr << "Hi :)" << std::endl;
			if (_input.length() > 100000)
			{
				write(_pipout, (_input.substr(0, 1000)).c_str(), 1000);
				this->addMetaVariable("CONTENT_LENGTH", "1000");
			}
			else
				write(_pipout, _input.c_str(), _input.length());
			std::cerr << "hello :(" << std::endl;
			close(_pipout);
			char **av = new char * [3];
			av[0] = new char [_binary.length() + 1];
			av[1] = new char [target.length() + 1];
			strcpy(av[0], _binary.c_str());
			strcpy(av[1], target.c_str());
			av[2] = NULL;
			execve(_binary.c_str(), av, this->doubleStringToChar(_metaVariables));
			exit(1);
		}
		else
			waitpid(-1, &ret, 0);
		std::cerr << "hello" << std::endl;
		exit(0);
	}
	else
	{
		char		tmp[RECV_SIZE];
		std::cout << "Hanging" << std::endl;
		waitpid(-1, &ret, 0);
		std::cout << "bruh" << std::endl;
		close(_pipout);
		while ((ret = read(_pipin, tmp, RECV_SIZE - 1)) != 0)
		{
			tmp[ret] = '\0';
			_output += std::string(tmp);
			std::cerr << "Read " << ret << " bytes." << std::endl;
		}
	}
	dup2(_stdin, STDIN_FILENO);
	dup2(_stdout, STDOUT_FILENO);
	close(_pipout);
	close(_pipin);
}

/*		 */

void	CGI::addMetaVariable(std::string name, std::string value)
{
	_metaVariables[name] = value;
}

char	**CGI::doubleStringToChar(DoubleString param)
{
	char		**ret;
	std::string	tmp;
	size_t		i;

	i = 0;
	ret = new char* [param.size() + 1];
	for (DoubleString::iterator it = param.begin(); it != param.end(); it++)
	{
		tmp = it->first + "=" + it->second;
		ret[i] = new char [tmp.length() + 1];
		ret[i] = strcpy(ret[i], (const char *)tmp.c_str());
		i++;
	}
	ret[i] = NULL;
	i = 0;
	while (ret[i])
	{
		std::cerr << "[DEBUG] Got these env variable : " << ret[i] << std::endl;
		i++;
	}
	return (ret);
}

std::string		CGI::getOutput()
{
	return (_output);
}

void			CGI::convertHeadersToMetaVariables(Request request)
{
	DoubleString	headers = request.getHeaders();
	for (DoubleString::iterator it = headers.begin(); it != headers.end(); it++)
	{
		std::string name = it->first;
		std::string value = it->second;
		Utils::to_upper(name);
		name = Utils::replace(name, "-", "_");
		this->addMetaVariable("HTTP_" + name, value);
	}
}
