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

void		 CGI::setBinary(std::string path)
{
_binary = path;
}

void		 CGI::setInput(std::string content)
{
_input = content;
}

void		 CGI::execute(std::string target)
{
	pid_t					pid;
	int						ex_fd[2];
	int						output_fd;
	int						input_fd;
	char					tmp[1024];
	int						ret;
	std::string				output;

	ex_fd[0] = dup(STDIN_FILENO);
	ex_fd[1] = dup(STDOUT_FILENO);
	pid = fork();
	if (pid == -1)
	{
		Console::error("Fork failed for CGI : PID = -1");
		return ;
	}
	else if (pid == 0)		 // On est dans le fils
	{
		char **av = new char * [3];
		av[0] = new char [_binary.length() + 1];
		av[1] = new char [target.length() + 1];

		input_fd = open("tmp/.cgi_input", O_RDWR, 0777);
		output_fd = open("tmp/.cgi_output", O_RDWR | O_TRUNC, 0777);

		dup2(output_fd, STDOUT_FILENO);
		close(output_fd);
		dup2(input_fd, STDIN_FILENO);
		close(input_fd);

		strcpy(av[0], _binary.c_str());
		strcpy(av[1], target.c_str());
		av[2] = NULL;
		execve(_binary.c_str(), av, this->doubleStringToChar(_metaVariables));
	}
	else
	{
		int				  status;

		input_fd = open("tmp/.cgi_input", O_RDWR | O_TRUNC, 0777);
		output_fd = open("tmp/.cgi_output", O_RDWR, 0777);

		write(input_fd, _input.c_str(), _input.length());

		waitpid(pid, &status, 0);

		while ((ret = read(output_fd, tmp, 1023)) != 0)
		{
			tmp[ret] = '\0';
			output += std::string(tmp);
		}
		_output = output;

		close(output_fd);
		close(input_fd);

		dup2(ex_fd[0], STDIN_FILENO);
		dup2(ex_fd[1], STDOUT_FILENO);
	}
}

void		 CGI::addMetaVariable(std::string name, std::string value)
{
	_metaVariables[name] = value;
}

char		 **CGI::doubleStringToChar(DoubleString param)
{
	char				  **ret;
	std::string		 tmp;
	size_t				  i;

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

std::string				  CGI::getOutput()
{
	return (_output);
}

void						   CGI::convertHeadersToMetaVariables(Request request)
{
	DoubleString		 headers = request.getHeaders();
	for (DoubleString::iterator it = headers.begin(); it != headers.end(); it++)
	{
		std::string name = it->first;
		std::string value = it->second;
		Utils::to_upper(name);
		name = Utils::replace(name, "-", "_");
		this->addMetaVariable("HTTP_" + name, value);
	}
}