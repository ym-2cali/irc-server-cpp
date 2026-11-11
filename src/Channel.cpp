#include "Channel.hpp"
#include "Client.hpp"
#include "Utils.hpp"

Channel::Channel(const std::string& name)
	: topic_time(0), limit(0), invite_only(false), topic_restricted(false),
	  _name(name), _created(time(NULL)) {}

Channel::~Channel() {}

const std::string&	Channel::name() const { return _name; }
time_t				Channel::created() const { return _created; }

bool	Channel::has(Client* client) const { return _members.count(client) != 0; }
bool	Channel::is_op(Client* client) const { return _ops.count(client) != 0; }
bool	Channel::is_invited(Client* client) const { return _invited.count(client) != 0; }
bool	Channel::empty() const { return _members.empty(); }
std::size_t	Channel::size() const { return _members.size(); }

const std::set<Client*>&	Channel::members() const { return _members; }

void	Channel::add(Client* client, bool op) {
	_members.insert(client);
	_invited.erase(client);
	if (op)
		_ops.insert(client);
}

void	Channel::remove(Client* client) {
	_members.erase(client);
	_ops.erase(client);
	_invited.erase(client);
}

void	Channel::set_op(Client* client, bool op) {
	if (op)
		_ops.insert(client);
	else
		_ops.erase(client);
}

void	Channel::invite(Client* client) {
	_invited.insert(client);
}

void	Channel::broadcast(const std::string& line, Client* except) {
	for (std::set<Client*>::iterator it = _members.begin(); it != _members.end(); ++it)
		if (*it != except)
			(*it)->send(line);
}

std::string	Channel::names() const {
	std::string	out;
	for (std::set<Client*>::const_iterator it = _members.begin(); it != _members.end(); ++it) {
		if (!out.empty())
			out += " ";
		if (is_op(*it))
			out += "@";
		out += (*it)->nick;
	}
	return out;
}

std::string	Channel::modes(bool show_params) const {
	std::string	flags = "+";
	std::string	params;
	if (invite_only)
		flags += "i";
	if (topic_restricted)
		flags += "t";
	if (!key.empty()) {
		flags += "k";
		if (show_params)
			params += " " + key;
	}
	if (limit) {
		flags += "l";
		if (show_params)
			params += " " + itos(limit);
	}
	return flags + params;
}
