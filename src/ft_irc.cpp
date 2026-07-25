#include "../inc/ft_irc.hpp"

Server::Server(){}

Server::Server(int port):_port(port){}

Server::Server(Server& other):_port(other._port),in_data(other.in_data),_fds(other._fds),sockfd(other.sockfd){
}

Server::~Server(){}

Server& Server::operator=(const Server& other){
	if (this != &other)
	{
		this->sockfd = other.sockfd;
		this->_port = other._port;
		this->in_data = other.in_data;
		this->_fds = other._fds;
	}
	return *this;
}

void	Server::SetInData(){
	std::memset(&this->in_data, 0, sizeof(this->in_data));
	this->in_data.sin_family = AF_INET;
	this->in_data.sin_port = htons(this->_port);
	this->in_data.sin_addr.s_addr = htonl(INADDR_ANY);
}

void	Server::init_server(){
	this->sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (this->sockfd < 0)
		throw std::runtime_error("ERROR: failing opening sockets");
	int	opt = 1;
	if (setsockopt(this->sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
		std::cerr << "ERROR: failed setting reuseaddr\n";
	this->SetInData();
	if (bind(this->sockfd, (struct sockaddr *)&this->in_data, sizeof(this->in_data)))
		throw std::runtime_error("ERROR: failing binding sockets");
	if (listen(this->sockfd, SOMAXCONN))
		throw std::runtime_error("ERROR: socket failed listening");
}

bool	Server::accept_cl(){
	errno = 0;
	int	clientfd = accept(this->sockfd, NULL, NULL);
	if (clientfd < 0){
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return false;
		return (std::cerr << "ERROR: Failing accepting client\n",false);
	}
	int flags = fcntl(clientfd, F_GETFL, 0);
  	if (flags < 0)
  	    return (std::cerr << "ERROR: fcntl get flags failed\n", false);
  	if (fcntl(clientfd, F_SETFL, flags | O_NONBLOCK) < 0)
  		return (std::cerr << "ERROR: fcntl set nonblock failed\n", false);
	struct pollfd	cl;
	std::memset(&cl, 0, sizeof(cl));
	cl.fd = clientfd;
	cl.events = POLLIN;
	this->_fds.push_back(cl);
	this->clients.push_back(Client(clientfd));
	return true;
}

bool	Server::recv_msgs(){
	for (std::size_t i = 0; i < this->_fds.size(); i++)
	{
		if (this->_fds[i].revents & POLLIN)
		{
			errno = 0;
			char	buf[513];
			ssize_t	n = recv(this->_fds[i].fd, &buf, sizeof(buf) - 1, 0);
			if (n > 0)
			{
				buf[n] = '\0';
				this->clients[i].buf.append(buf, n);
				this->clients[i].get_msg();
			}
			else if (n < 0)
			{
				if (errno == EAGAIN || errno == EWOULDBLOCK){
					// send something  to the client
					continue ;
				}
				std::stringstream ss;
				ss << "Failed receiving from client " << (i + 1);
				// send something  to the client
				// throw std::runtime_error(ss.str());
			}
			else {
				close(this->_fds[i].fd);
				this->_fds.erase(this->_fds.begin() + i);
				this->clients.erase(this->clients.begin() + i);
				std::cout << "Client " << i + 1 << " disconnected\n";
			}
		}
	}
	return true;
}

bool	Server::handle_clients(){
	int	ready_fds = poll(&this->_fds[0], this->_fds.size(), 0);
	if (ready_fds)
	{
		this->recv_msgs();
		this->handle_commands();
		// throw msgs send messages
	}
	else if (ready_fds < 0)
		return false;
	return true;
}

void	Server::Server_ON(){
	while (true)
	{
		std::cout << "<----------<>LISTENING</>---------->\n";
		while (this->accept_cl())
			std::cout << "<------Client " << this->_fds.size() << " Connected------>\n";
		if (this->_fds.empty())
		{
			sleep(1);
			continue;
		}
		if (!this->handle_clients())
		{
			// poll error
			continue ;
		}
		sleep(1);
	}		
}

void	Server::close_server(){
	for (std::size_t i = 0; i < this->_fds.size(); i++)
		close(this->_fds[i].fd);
	close(this->sockfd);
}

