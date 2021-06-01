#include "cgi/CGI.hpp"

CGI::CGI()
{
	
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

void	CGI::execute(std::string target)
{
	pid_t		pid;
	int			output_fd[2];
	int			input_fd[2];
	char		tmp[1024];
	int			ret;
	std::string	output;

	if (pipe(output_fd) || pipe(input_fd))
	{
		Console::error("Pipe failed for CGI");
		return ;
	}
	pid = fork();
	if (pid == -1)
	{
		Console::error("Fork failed for CGI : PID = -1");
		return ;
	}
	else if (pid == 0)	// On est dans le fils
	{
		char **av = new char * [3];
		av[0] = new char [_binary.length() + 1];
		av[1] = new char [target.length() + 1];

		close(output_fd[0]);
		dup2(output_fd[1], STDOUT_FILENO);
		close(output_fd[1]);

		close(input_fd[1]);
        dup2(input_fd[0], STDIN_FILENO);
        close(input_fd[0]);

		strcpy(av[0], _binary.c_str());
		strcpy(av[1], target.c_str());
		av[2] = NULL;
		execve(_binary.c_str(), av, this->doubleStringToChar(_metaVariables));
	}
	else
	{
		int		status;

		close(input_fd[0]);
        write(input_fd[1], _input.c_str(), _input.length());
        close(input_fd[1]);

		close(output_fd[1]);
		waitpid(pid, &status, 0);
		while ((ret = read(output_fd[0], tmp, 1023)) != 0)
		{
			tmp[ret] = '\0';
			output += std::string(tmp);
		}
		_output = output;
	}
}

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
		strcpy(ret[i], tmp.c_str());
		i++;
	}
	ret[i] = NULL;
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
