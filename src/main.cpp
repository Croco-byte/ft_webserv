#include "Webserv.hpp"

/**
 * malloc
 * free
 * write
 * open
 * read
 * close
 * mkdir
 * rmdir
 * unlink
 * fork
 * wait
 * waitpid
 * wait3
 * wait4
 * signal
 * kill
 * exit
 * getcwd
 * chdir
 * stat
 * lstat
 * fstat
 * lseek
 * opendir
 * readdir
 * closedir
 * execve
 * dup
 * dup2
 * pipe
 * strerror
 * errno
 * gettimeofday
 * strptime
 * strftime
 * usleep
 * select
 * socket
 * accept
 * listen
 * send
 * recv
 * bind
 * connect
 * inet_addr
 * setsockopt
 * getsockname
 * fcntl
*/

Webserv webserv;

void	quit(int unused)
{
	(void)unused;
	std::cout << "\b\b  " << std::endl;
	webserv.stop();
	Console::info("Quit by CTRL-C");
	exit(0);
}

int		main(int ac, char **av)
{
	if (ac == 1)
	{
		Console::error("Require a configuration file");
		return (1);
	}
	signal(SIGINT, quit);
	webserv.parseConfiguration(av[1]);
	webserv.run();
	return (0);
}
