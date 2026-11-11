#ifndef CHANNEL_HPP
# define CHANNEL_HPP

# include <string>
# include <set>
# include <ctime>

class Client;

class Channel {
	public:
		Channel(const std::string& name);
		~Channel();

		const std::string&	name() const;
		time_t				created() const;

		std::string	topic;
		std::string	topic_setter;
		time_t		topic_time;
		std::string	key;
		std::size_t	limit;
		bool		invite_only;
		bool		topic_restricted;

		bool	has(Client* client) const;
		bool	is_op(Client* client) const;
		bool	is_invited(Client* client) const;
		bool	empty() const;
		std::size_t	size() const;

		void	add(Client* client, bool op);
		void	remove(Client* client);
		void	set_op(Client* client, bool op);
		void	invite(Client* client);

		// send a line to every member, optionally skipping one
		void	broadcast(const std::string& line, Client* except = NULL);
		// "@alice bob carol" for RPL_NAMREPLY
		std::string	names() const;
		// "+itkl key 10"; the key is only shown to members
		std::string	modes(bool show_params) const;
		const std::set<Client*>&	members() const;

	private:
		Channel(const Channel& other);
		Channel& operator=(const Channel& other);

		std::string			_name;
		time_t				_created;
		std::set<Client*>	_members;
		std::set<Client*>	_ops;
		std::set<Client*>	_invited;
};

#endif
