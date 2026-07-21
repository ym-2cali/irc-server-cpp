#ifndef FT_IRC_HPP
# define FT_IRC_HPP

# include <iostream>
# include <unistd.h>
# include <sys/socket.h>
# include <cstring>
# include <string>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <poll.h>
# include <vector>
# include <cerrno>
# include <sstream>
# include <fcntl.h>

class	Client {
	public:
		int	_fd;
		Client(int fd);
		std::string	buf;
};

class   Server {
	int	_port;
	struct sockaddr_in in_data;
	std::vector<struct pollfd>	_fds;
	std::vector<Client>	clients;
	int	sockfd;
	public:
		Server();
		Server(int port);
		Server(Server& other);
		~Server();
		Server& operator=(const Server& other);

		void	init_server();
		void	close_server();
		void	Server_ON();
		void	SetInData();
		bool	accept_cl();
		bool	handle_clients();
		bool	recv_msgs();
};

#endif