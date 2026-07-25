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

typedef	enum s_cmpnt_type {
	PREFIX,
	CMD,
	PARAMS,
	MIDDLE,
	TRAILING
} t_cmpnt_type;

typedef struct s_msg_cmpnts {
	std::string _data;
	t_cmpnt_type	_type;
} t_msg_cmpnts;

class	Client {
	public:
		int	_fd;
		std::string	nickname;
		std::string	username;
		bool	registred;
		std::string	password;

		Client(int fd);

		std::string	buf;
		std::string	out_buf;
		std::vector<t_msg_cmpnts> cmpnts;

		bool	get_msg();
		void	set_cmpnts(std::string& msg);
		bool	tokenizer();

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
		void	handle_commands();
		void	cl_registration(Client& client, std::string& cmd);
		bool	nickname_coll(std::string& nick);
		void	recv_msgs();
		bool	send_msg(Client& client);

};

#endif