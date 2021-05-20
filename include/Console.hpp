#ifndef CONSOLE_HPP
# define CONSOLE_HPP

#include <iostream>
#include <ctime>
#include <vector>
#include <sstream>
#include "time.h"

#define COLOR_RESET  "\033[0m"
#define RED_TEXT     "\033[0;31m"
#define GREY_TEXT     "\033[1;30m"

class Console
{
	public:
		enum LogType
		{
			INFO,
			WARNING,
			NOTICE,
			ERROR,
			SUCCESS,
			CONNECT
		};

		Console()
		{

		}

		virtual ~Console()
		{

		}

		static void	info(std::string message)
		{
			Console::log(INFO, message);
		}

		static void	warning(std::string message)
		{
			Console::log(WARNING, message);
		}

		static void	error(std::string message)
		{
			Console::log(ERROR, message);
		}

		static void	log(LogType type, std::string message)
		{
			std::string prefix    = Console::get_prefix(type);
			std::string timestamp = Console::get_current_timestamp();
			std::cout << "\033[37m" << timestamp + " " + prefix;
			std::cout << " ";
			std::cout << message << "\033[0m" << std::endl;
		}

		static std::string get_prefix (LogType type)
		{
			if (type == INFO)
			{
				return "\033[33m[INFO]";
			}
			else if (type == WARNING)
			{
				return "\033[1m\033[33m[WARNING]";
			}
			else if (type == SUCCESS)
			{
				return "\033[32m[SUCCESS]";
			}
			else if (type == NOTICE)
			{
				return "\033[0;36m[NOTICE]";
			}
			else if (type == ERROR)
			{
				return "\033[31m[ERROR]";
			}
			else if (type == CONNECT)
			{
				return "\033[1;35m[CONNECT]";
			}
			else
			{
				return "";
			}
		}

		static std::string get_current_timestamp ()
		{
			std::string timestamp_str;
			time_t      time1 = 0;
			char        buffer[256];
			// on déclare la référence de temps (time 0)
			time1 = time(0);
			if (time1)
			{
				// on récupère dans la structure tm le timestamp actuel
				tm *local_time = localtime(&time1);
				// on met dans buffer la date (sous le format d/m/y T)
				strftime(buffer, 256, "%d/%m/%y %T", local_time);
				// on case la char * en std::string pour pouvoir y ajouter des crochets
				timestamp_str  = buffer;
				timestamp_str.insert(timestamp_str.begin(), '[');
				timestamp_str.insert(timestamp_str.end(), ']');
			}
			return (timestamp_str);
		}
};

#endif