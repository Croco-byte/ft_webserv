SRCS			=	src/main.cpp  \
					src/utils/file.cpp \
					src/utils/time.cpp  \
					src/utils/quit.cpp   \
					src/utils/string.cpp  \
					src/Server.cpp \
					src/Request.cpp  \
					src/Response.cpp  \
					src/ServerConfiguration.cpp \
					src/Route.cpp \
					src/Webserv.cpp \
					src/ConnexionManager.cpp \
					src/CGI.cpp \
					src/AutoIndex.cpp

OBJS			= $(SRCS:.cpp=.o)

CC				= g++
RM				= rm -f
INCLUDE			= -I./include/
CFLAGS			= -Wall -Werror -Wextra -std=c++98 -g -fsanitize=address
# -std=c++98
LIBS			= 

NAME			= webserv

all:			$(NAME)
				clear
				./webserv config/default.conf

%.o: %.cpp
				${CC} -c ${CFLAGS} ${INCLUDE} $< -o $@

$(NAME):		$(OBJS)
				${CC} $(OBJS) ${INCLUDE} ${CFLAGS} ${LIBS} -o $(NAME)

clean:
				$(RM) ${OBJS}

fclean:			clean
				$(RM) $(NAME)

%:
		@:

bonus: re


re:				fclean all

.PHONY:			all clean fclean re bonus
