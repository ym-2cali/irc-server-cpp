#ifndef SERVER_HPP
# define SERVER_HPP

# include <string>
# include <map>
# include <csignal>
# include "Client.hpp"
# include "Channel.hpp"
# include "Message.hpp"

# define SERVER_NAME	"ircserv"
# define SERVER_VERSION	"1.0"

extern volatile sig_atomic_t	g_running;

class Server {
	public:
		Server(int port, const std::string& password);
		~Server();

		void	run();

	private:
		Server(const Server& other);
		Server& operator=(const Server& other);

		typedef void (Server::*Handler)(Client&, const Message&);

		int								_port;
		std::string						_password;
		int								_listen_fd;
		std::string						_created;
		std::map<int, Client*>			_clients;
		std::map<std::string, Channel*>	_channels;	// keyed by lowercased name
		std::map<std::string, Handler>	_handlers;

		// networking (Server.cpp)
		void	setup_socket();
		void	accept_clients();
		void	read_from(Client& client);
		void	write_to(Client& client);
		void	process_lines(Client& client);
		void	dispatch(Client& client, const Message& msg);
		void	disconnect(Client& client, const std::string& reason);
		void	remove_closed();

		// helpers (Server.cpp)
		Client*		find_client(const std::string& nick);
		Channel*	find_channel(const std::string& name);
		void		delete_channel(Channel* channel);
		void		reply(Client& client, const std::string& code, const std::string& text);
		void		notify_peers(Client& client, const std::string& line, bool include_self);
		void		try_register(Client& client);
		void		send_names(Client& client, Channel& channel);
		void		send_topic(Client& client, Channel& channel);
		void		part_channel(Client& client, Channel& channel, const std::string& reason);
		void		deliver(Client& client, const Message& msg, bool notice);

		// commands (Commands.cpp)
		void	cmd_cap(Client& client, const Message& msg);
		void	cmd_pass(Client& client, const Message& msg);
		void	cmd_nick(Client& client, const Message& msg);
		void	cmd_user(Client& client, const Message& msg);
		void	cmd_ping(Client& client, const Message& msg);
		void	cmd_pong(Client& client, const Message& msg);
		void	cmd_quit(Client& client, const Message& msg);
		void	cmd_join(Client& client, const Message& msg);
		void	cmd_part(Client& client, const Message& msg);
		void	cmd_privmsg(Client& client, const Message& msg);
		void	cmd_notice(Client& client, const Message& msg);
		void	cmd_topic(Client& client, const Message& msg);
		void	cmd_kick(Client& client, const Message& msg);
		void	cmd_invite(Client& client, const Message& msg);
		void	cmd_mode(Client& client, const Message& msg);
		void	cmd_names(Client& client, const Message& msg);
		void	cmd_who(Client& client, const Message& msg);
};

#endif
