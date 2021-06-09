# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: user42 <user42@student.42.fr>              +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2021/05/27 17:52:43 by user42            #+#    #+#              #
#    Updated: 2021/06/09 17:00:15 by user42           ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME			= webserv

CC				= g++
RM				= rm -f
INCLUDE			= -I./include/
CFLAGS			= -Wall -Werror -Wextra -std=c++98 -g3 -fsanitize=address

CGI= CGI
CORE= ConnexionManager main Webserv
REQUEST= Request
RESPONSE= AutoIndex Response
SERVER= Route ServerConfiguration Server
UTILS= file quit string time

SRCS =	$(addsuffix .cpp, $(addprefix src/cgi/, $(CGI))) \
		$(addsuffix .cpp, $(addprefix src/core/, $(CORE))) \
		$(addsuffix .cpp, $(addprefix src/request/, $(REQUEST))) \
		$(addsuffix .cpp, $(addprefix src/response/, $(RESPONSE))) \
		$(addsuffix .cpp, $(addprefix src/server/, $(SERVER))) \
		$(addsuffix .cpp, $(addprefix src/utils/, $(UTILS)))


OBJS			= $(SRCS:.cpp=.o)


all:			$(NAME)

%.o: %.cpp
				${CC} -c ${CFLAGS} ${INCLUDE} $< -o $@

$(NAME):		$(OBJS)
				${CC} $(OBJS) ${INCLUDE} ${CFLAGS} -o $(NAME)

clean:
				$(RM) ${OBJS}

fclean:			clean
				$(RM) $(NAME)

re:				fclean all

.PHONY:			all clean fclean re bonus
