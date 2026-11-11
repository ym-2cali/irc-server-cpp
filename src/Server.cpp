#include "Server.hpp"
#include "Utils.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <ctime>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

Server::Server(int port, const std::string& password)
	: _port(port), _password(password), _listen_fd(-1) {
	time_t	now = time(NULL);
	char	buf[64];
	std::strftime(buf, sizeof(buf), "%a %b %d %Y at %H:%M:%S", std::localtime(&now));
	_created = buf;

	_handlers["CAP"] = &Server::cmd_cap;
	_handlers["PASS"] = &Server::cmd_pass;
	_handlers["NICK"] = &Server::cmd_nick;
	_handlers["USER"] = &Server::cmd_user;
	_handlers["PING"] = &Server::cmd_ping;
	_handlers["PONG"] = &Server::cmd_pong;
	_handlers["QUIT"] = &Server::cmd_quit;
	_handlers["JOIN"] = &Server::cmd_join;
	_handlers["PART"] = &Server::cmd_part;
	_handlers["PRIVMSG"] = &Server::cmd_privmsg;
	_handlers["NOTICE"] = &Server::cmd_notice;
	_handlers["TOPIC"] = &Server::cmd_topic;
	_handlers["KICK"] = &Server::cmd_kick;
	_handlers["INVITE"] = &Server::cmd_invite;
	_handlers["MODE"] = &Server::cmd_mode;
	_handlers["NAMES"] = &Server::cmd_names;
	_handlers["WHO"] = &Server::cmd_who;
}

Server::~Server() {
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		close(it->first);
		delete it->second;
	}
	for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
		delete it->second;
	if (_listen_fd >= 0)
		close(_listen_fd);
}

/* ------------------------------------------------------------------------- */
/*                                networking                                 */
/* ------------------------------------------------------------------------- */

void	Server::setup_socket() {
	_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_listen_fd < 0)
		throw std::runtime_error("socket() failed");
	int	opt = 1;
	if (setsockopt(_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		throw std::runtime_error("setsockopt() failed");
	if (fcntl(_listen_fd, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error("fcntl() failed");

	struct sockaddr_in	addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(_port);
	if (bind(_listen_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0)
		throw std::runtime_error("bind() failed: port " + itos(_port) + " unavailable");
	if (listen(_listen_fd, SOMAXCONN) < 0)
		throw std::runtime_error("listen() failed");
}

void	Server::run() {
	setup_socket();
	std::cout << SERVER_NAME << " listening on port " << _port << std::endl;

	while (g_running) {
		// rebuild the poll set every round so removed clients never linger
		std::vector<struct pollfd>	pfds;
		struct pollfd				p;

		p.fd = _listen_fd;
		p.events = POLLIN;
		p.revents = 0;
		pfds.push_back(p);
		for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
			p.fd = it->first;
			p.events = POLLIN;
			if (!it->second->outbuf.empty())
				p.events |= POLLOUT;
			p.revents = 0;
			pfds.push_back(p);
		}

		if (poll(&pfds[0], pfds.size(), -1) < 0) {
			if (errno == EINTR)
				continue;
			throw std::runtime_error("poll() failed");
		}

		if (pfds[0].revents & POLLIN)
			accept_clients();
		for (std::size_t i = 1; i < pfds.size(); i++) {
			std::map<int, Client*>::iterator	it = _clients.find(pfds[i].fd);
			if (it == _clients.end() || it->second->closing)
				continue;
			Client&	client = *it->second;
			if (pfds[i].revents & POLLIN)
				read_from(client);
			else if (pfds[i].revents & (POLLHUP | POLLERR | POLLNVAL))
				disconnect(client, "Connection closed");
			if (!client.closing && (pfds[i].revents & POLLOUT))
				write_to(client);
		}
		remove_closed();
	}
}

void	Server::accept_clients() {
	while (true) {
		struct sockaddr_in	addr;
		socklen_t			len = sizeof(addr);
		int					fd = accept(_listen_fd, reinterpret_cast<struct sockaddr*>(&addr), &len);
		if (fd < 0)
			return;
		if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
			close(fd);
			continue;
		}
		_clients[fd] = new Client(fd, inet_ntoa(addr.sin_addr));
		std::cout << "[+] connection from " << inet_ntoa(addr.sin_addr) << " (fd " << fd << ")" << std::endl;
	}
}

void	Server::read_from(Client& client) {
	char	buf[4096];
	ssize_t	n = recv(client.fd, buf, sizeof(buf), 0);

	if (n <= 0) {
		disconnect(client, n == 0 ? "Connection closed" : "Read error");
		return;
	}
	client.inbuf.append(buf, n);
	process_lines(client);
}

void	Server::write_to(Client& client) {
	ssize_t	n = send(client.fd, client.outbuf.data(), client.outbuf.size(), 0);

	if (n < 0) {
		disconnect(client, "Write error");
		return;
	}
	client.outbuf.erase(0, n);
}

// Split the input buffer into complete lines. Partial data stays buffered
// until the rest arrives, so "PRI" + "VMSG ..." across packets works.
void	Server::process_lines(Client& client) {
	std::size_t	pos;

	while (!client.closing && (pos = client.inbuf.find('\n')) != std::string::npos) {
		std::string	line = client.inbuf.substr(0, pos);
		client.inbuf.erase(0, pos + 1);
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		if (line.size() > 510)
			line.resize(510);
		if (!line.empty())
			dispatch(client, Message::parse(line));
	}
	// a peer that never sends a newline must not grow the buffer forever
	if (client.inbuf.size() > 8192)
		client.inbuf.clear();
}

void	Server::dispatch(Client& client, const Message& msg) {
	if (msg.command.empty())
		return;
	std::map<std::string, Handler>::iterator	it = _handlers.find(msg.command);
	if (it == _handlers.end()) {
		if (client.registered)
			reply(client, "421", msg.command + " :Unknown command");
		else
			reply(client, "451", ":You have not registered");
		return;
	}
	static const char*	pre_register[] = {"CAP", "PASS", "NICK", "USER", "PING", "PONG", "QUIT", NULL};
	if (!client.registered) {
		bool	allowed = false;
		for (int i = 0; pre_register[i]; i++)
			if (msg.command == pre_register[i])
				allowed = true;
		if (!allowed) {
			reply(client, "451", ":You have not registered");
			return;
		}
	}
	(this->*(it->second))(client, msg);
}

// Tell everyone else, leave all channels, and mark the client for removal.
// The actual close() happens in remove_closed() at the end of the loop.
void	Server::disconnect(Client& client, const std::string& reason) {
	if (client.closing)
		return;
	if (client.registered)
		notify_peers(client, ":" + client.prefix() + " QUIT :" + reason, false);

	std::vector<Channel*>	to_delete;
	for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
		it->second->remove(&client);
		if (it->second->empty())
			to_delete.push_back(it->second);
	}
	for (std::size_t i = 0; i < to_delete.size(); i++)
		delete_channel(to_delete[i]);

	client.send("ERROR :Closing link: " + client.host + " (" + reason + ")");
	client.closing = true;
	std::cout << "[-] " << client.name() << " disconnected (" << reason << ")" << std::endl;
}

void	Server::remove_closed() {
	std::map<int, Client*>::iterator	it = _clients.begin();

	while (it != _clients.end()) {
		Client*	client = it->second;
		if (client->closing) {
			if (!client->outbuf.empty())
				send(client->fd, client->outbuf.data(), client->outbuf.size(), 0);
			close(client->fd);
			delete client;
			_clients.erase(it++);
		}
		else
			++it;
	}
}

/* ------------------------------------------------------------------------- */
/*                                 helpers                                   */
/* ------------------------------------------------------------------------- */

Client*	Server::find_client(const std::string& nick) {
	std::string	lower = to_lower(nick);
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		if (!it->second->closing && !it->second->nick.empty() && to_lower(it->second->nick) == lower)
			return it->second;
	return NULL;
}

Channel*	Server::find_channel(const std::string& name) {
	std::map<std::string, Channel*>::iterator	it = _channels.find(to_lower(name));
	return it == _channels.end() ? NULL : it->second;
}

void	Server::delete_channel(Channel* channel) {
	_channels.erase(to_lower(channel->name()));
	delete channel;
}

void	Server::reply(Client& client, const std::string& code, const std::string& text) {
	client.send(":" SERVER_NAME " " + code + " " + client.name() + " " + text);
}

// Send a line once to every client sharing at least one channel with `client`.
void	Server::notify_peers(Client& client, const std::string& line, bool include_self) {
	std::set<Client*>	peers;

	for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
		if (it->second->has(&client))
			peers.insert(it->second->members().begin(), it->second->members().end());
	peers.erase(&client);
	for (std::set<Client*>::iterator it = peers.begin(); it != peers.end(); ++it)
		(*it)->send(line);
	if (include_self)
		client.send(line);
}

void	Server::try_register(Client& client) {
	if (client.registered || client.nick.empty() || client.user.empty() || !client.pass_ok)
		return;
	client.registered = true;
	reply(client, "001", ":Welcome to the IRC network " + client.prefix());
	reply(client, "002", ":Your host is " SERVER_NAME ", running version " SERVER_VERSION);
	reply(client, "003", ":This server was created " + _created);
	reply(client, "004", SERVER_NAME " " SERVER_VERSION " o iklot");
	reply(client, "005", "CHANTYPES=#& PREFIX=(o)@ CHANMODES=,k,l,it NICKLEN=9 CHANNELLEN=50 :are supported by this server");
	reply(client, "422", ":MOTD File is missing");
	std::cout << "[*] " << client.prefix() << " registered" << std::endl;
}

void	Server::send_names(Client& client, Channel& channel) {
	reply(client, "353", "= " + channel.name() + " :" + channel.names());
	reply(client, "366", channel.name() + " :End of /NAMES list");
}

void	Server::send_topic(Client& client, Channel& channel) {
	if (channel.topic.empty()) {
		reply(client, "331", channel.name() + " :No topic is set");
		return;
	}
	reply(client, "332", channel.name() + " :" + channel.topic);
	reply(client, "333", channel.name() + " " + channel.topic_setter + " " + itos(channel.topic_time));
}

void	Server::part_channel(Client& client, Channel& channel, const std::string& reason) {
	std::string	line = ":" + client.prefix() + " PART " + channel.name();
	if (!reason.empty())
		line += " :" + reason;
	channel.broadcast(line);
	channel.remove(&client);
	if (channel.empty())
		delete_channel(&channel);
}
