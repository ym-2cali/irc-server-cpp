SERVER_NAME= server_exec

CLIENT_NAME= client_exec

CLIENT_SRC= client/client_src/client.cpp \

SERVER_SRC= src/server.cpp src/ft_irc.cpp src/client_tools.cpp #src/main.cpp

CLIENT_OBJ= $(CLIENT_SRC:.cpp=.o)

SERVER_OBJ= $(SERVER_SRC:.cpp=.o)

GXX= c++

FLAGS= -Wall -Wextra -Werror -std=c++98 #-fsanitize=address

HEADER= inc/ft_irc.hpp

all:$(SERVER_NAME) $(CLIENT_NAME)

client:$(CLIENT_NAME)

server:$(SERVER_NAME)

$(CLIENT_NAME):$(CLIENT_OBJ)
		$(GXX) $(FLAGS) $(CLIENT_OBJ) -o $(CLIENT_NAME)

$(SERVER_NAME):$(SERVER_OBJ)
		$(GXX) $(FLAGS) $(SERVER_OBJ) -o $(SERVER_NAME)

%.o:%.cpp $(HEADER)
		$(GXX) $(FLAGS) -c $< -o $@

clean:
		rm -f $(SERVER_OBJ) $(CLIENT_OBJ) 

fclean:clean
		rm -f server_exec client_exec

re:fclean all

.PHONY: clean