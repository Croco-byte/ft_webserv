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
	char					tmp[CGI_SIZE];
	int						ret(1);

	ex_fd[0] = dup(STDIN_FILENO);
	ex_fd[1] = dup(STDOUT_FILENO);

	FILE	*input_tmpfile = tmpfile();
	FILE	*output_tmpfile = tmpfile();
	int		input_fd = fileno(input_tmpfile);
	int		output_fd = fileno(output_tmpfile);

	write(input_fd, _input.c_str(), _input.length());
	lseek(input_fd, 0, SEEK_SET);

	pid = fork();
	if (pid == -1)
	{
		Console::error("Fork failed for CGI : PID = -1");
		return ;
	}
	else if (pid == 0)
	{
		char **av = new char * [3];
		av[0] = new char [_binary.length() + 1];
		av[1] = new char [target.length() + 1];

		dup2(output_fd, STDOUT_FILENO);
		dup2(input_fd, STDIN_FILENO);

		strcpy(av[0], _binary.c_str());
		strcpy(av[1], target.c_str());
		av[2] = NULL;
		execve(_binary.c_str(), av, this->doubleStringToChar(_metaVariables));
	}
	else
	{
		waitpid(-1, NULL, 0);
		lseek(output_fd, 0, SEEK_SET);

		while (ret > 0)
		{
			memset(tmp, 0, CGI_SIZE);
			ret = read(output_fd, tmp, CGI_SIZE - 1);
			_output += tmp;
		}

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
{ return (_output); }

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
