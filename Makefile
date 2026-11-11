NAME		= ircserv

CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98

SRC_DIR		= src
OBJ_DIR		= obj
INC_DIR		= inc

SRC			= main.cpp Server.cpp Commands.cpp Client.cpp Channel.cpp Message.cpp Utils.cpp
OBJ			= $(addprefix $(OBJ_DIR)/, $(SRC:.cpp=.o))
HEADERS		= $(wildcard $(INC_DIR)/*.hpp)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
