#include "CGI.hpp"

CGI::CGI()
{
	this->doubleStringToChar(_metaVariables);
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

void	CGI::execute()
{
	pid_t		pid;
	int			fd[2];
	char		tmp[1024];
	int			ret;
	std::string	output;

	if (pipe(fd))
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
		av[1] = new char [5];

		close(fd[0]);
		dup2(fd[1], STDOUT_FILENO);
		close(fd[1]);

		strcpy(av[0], "./a.out");
		strcpy(av[1], "test");
		av[2] = NULL;
		execve(_binary.c_str(), av, NULL);
	}
	else
	{
		int		status;

		close(fd[1]);
		waitpid(pid, &status, 0);
		while ((ret = read(fd[0], tmp, 1023)) != 0)
		{
			tmp[ret] = '\0';
			output += std::string(tmp);
		}
		Console::info("Receive '" + output + "'");
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
	ret = new char* [param.size()];
	for (DoubleString::iterator it = param.begin(); it != param.end(); it++)
	{
		tmp = it->first + "=" + it->second;
		ret[i] = new char [tmp.length() + 1];
		strcpy(ret[i], tmp.c_str());
		i++;
	}
	return (ret);
}
