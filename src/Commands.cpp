#include "Server.hpp"
#include "Utils.hpp"
#include <cstdlib>
#include <vector>

/* ------------------------------------------------------------------------- */
/*                              registration                                 */
/* ------------------------------------------------------------------------- */

// Capability negotiation is not supported; answer so modern clients
// (irssi, HexChat, WeeChat) stop waiting and continue registration.
void	Server::cmd_cap(Client& client, const Message& msg) {
	if (msg.params.empty())
		return;
	std::string	sub = to_upper(msg.params[0]);
	if (sub == "LS")
		client.send(":" SERVER_NAME " CAP * LS :");
	else if (sub == "REQ" && msg.params.size() > 1)
		client.send(":" SERVER_NAME " CAP * NAK :" + msg.params[1]);
}

void	Server::cmd_pass(Client& client, const Message& msg) {
	if (client.registered)
		return reply(client, "462", ":You may not reregister");
	if (msg.params.empty())
		return reply(client, "461", "PASS :Not enough parameters");
	client.pass_ok = (msg.params[0] == _password);
	if (!client.pass_ok)
		return reply(client, "464", ":Password incorrect");
	try_register(client);
}

void	Server::cmd_nick(Client& client, const Message& msg) {
	if (!client.pass_ok)
		return reply(client, "464", ":Password required (send PASS first)");
	if (msg.params.empty() || msg.params[0].empty())
		return reply(client, "431", ":No nickname given");

	const std::string&	nick = msg.params[0];
	if (!valid_nick(nick))
		return reply(client, "432", nick + " :Erroneous nickname");
	Client*	other = find_client(nick);
	if (other && other != &client)
		return reply(client, "433", nick + " :Nickname is already in use");
	if (nick == client.nick)
		return;
	if (client.registered)
		notify_peers(client, ":" + client.prefix() + " NICK :" + nick, true);
	client.nick = nick;
	try_register(client);
}

void	Server::cmd_user(Client& client, const Message& msg) {
	if (!client.pass_ok)
		return reply(client, "464", ":Password required (send PASS first)");
	if (client.registered)
		return reply(client, "462", ":You may not reregister");
	if (msg.params.size() < 4 || msg.params[0].empty())
		return reply(client, "461", "USER :Not enough parameters");
	client.user = msg.params[0].substr(0, 10);
	client.realname = msg.params[3];
	try_register(client);
}

/* ------------------------------------------------------------------------- */
/*                               connection                                  */
/* ------------------------------------------------------------------------- */

void	Server::cmd_ping(Client& client, const Message& msg) {
	if (msg.params.empty())
		return reply(client, "409", ":No origin specified");
	client.send(":" SERVER_NAME " PONG " SERVER_NAME " :" + msg.params[0]);
}

void	Server::cmd_pong(Client& client, const Message& msg) {
	(void)client;
	(void)msg;
}

void	Server::cmd_quit(Client& client, const Message& msg) {
	disconnect(client, msg.params.empty() ? "Client Quit" : "Quit: " + msg.params[0]);
}

/* ------------------------------------------------------------------------- */
/*                                channels                                   */
/* ------------------------------------------------------------------------- */

void	Server::cmd_join(Client& client, const Message& msg) {
	if (msg.params.empty())
		return reply(client, "461", "JOIN :Not enough parameters");

	// "JOIN 0" leaves every channel
	if (msg.params[0] == "0") {
		std::vector<Channel*>	joined;
		for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
			if (it->second->has(&client))
				joined.push_back(it->second);
		for (std::size_t i = 0; i < joined.size(); i++)
			part_channel(client, *joined[i], "");
		return;
	}

	std::vector<std::string>	names = split(msg.params[0], ',');
	std::vector<std::string>	keys;
	if (msg.params.size() > 1)
		keys = split(msg.params[1], ',');

	for (std::size_t i = 0; i < names.size(); i++) {
		const std::string&	name = names[i];
		std::string			key = i < keys.size() ? keys[i] : "";

		if (!valid_channel_name(name)) {
			reply(client, "476", name + " :Bad Channel Mask");
			continue;
		}
		Channel*	channel = find_channel(name);
		if (!channel) {
			channel = new Channel(name);
			_channels[to_lower(name)] = channel;
			channel->add(&client, true);
		}
		else {
			if (channel->has(&client))
				continue;
			if (channel->invite_only && !channel->is_invited(&client)) {
				reply(client, "473", channel->name() + " :Cannot join channel (+i)");
				continue;
			}
			if (!channel->key.empty() && key != channel->key) {
				reply(client, "475", channel->name() + " :Cannot join channel (+k)");
				continue;
			}
			if (channel->limit && channel->size() >= channel->limit) {
				reply(client, "471", channel->name() + " :Cannot join channel (+l)");
				continue;
			}
			channel->add(&client, false);
		}
		channel->broadcast(":" + client.prefix() + " JOIN " + channel->name());
		if (!channel->topic.empty())
			send_topic(client, *channel);
		send_names(client, *channel);
	}
}

void	Server::cmd_part(Client& client, const Message& msg) {
	if (msg.params.empty())
		return reply(client, "461", "PART :Not enough parameters");

	std::vector<std::string>	names = split(msg.params[0], ',');
	std::string					reason = msg.params.size() > 1 ? msg.params[1] : "";

	for (std::size_t i = 0; i < names.size(); i++) {
		Channel*	channel = find_channel(names[i]);
		if (!channel)
			reply(client, "403", names[i] + " :No such channel");
		else if (!channel->has(&client))
			reply(client, "442", channel->name() + " :You're not on that channel");
		else
			part_channel(client, *channel, reason);
	}
}

void	Server::cmd_topic(Client& client, const Message& msg) {
	if (msg.params.empty())
		return reply(client, "461", "TOPIC :Not enough parameters");

	Channel*	channel = find_channel(msg.params[0]);
	if (!channel)
		return reply(client, "403", msg.params[0] + " :No such channel");
	if (!channel->has(&client))
		return reply(client, "442", channel->name() + " :You're not on that channel");
	if (msg.params.size() == 1)
		return send_topic(client, *channel);
	if (channel->topic_restricted && !channel->is_op(&client))
		return reply(client, "482", channel->name() + " :You're not channel operator");

	channel->topic = msg.params[1];
	channel->topic_setter = client.nick;
	channel->topic_time = time(NULL);
	channel->broadcast(":" + client.prefix() + " TOPIC " + channel->name() + " :" + channel->topic);
}

void	Server::cmd_names(Client& client, const Message& msg) {
	if (msg.params.empty())
		return reply(client, "366", "* :End of /NAMES list");

	std::vector<std::string>	names = split(msg.params[0], ',');
	for (std::size_t i = 0; i < names.size(); i++) {
		Channel*	channel = find_channel(names[i]);
		if (channel)
			send_names(client, *channel);
		else
			reply(client, "366", names[i] + " :End of /NAMES list");
	}
}

void	Server::cmd_who(Client& client, const Message& msg) {
	std::string	mask = msg.params.empty() ? "*" : msg.params[0];
	Channel*	channel = find_channel(mask);

	if (channel) {
		const std::set<Client*>&	members = channel->members();
		for (std::set<Client*>::const_iterator it = members.begin(); it != members.end(); ++it)
			reply(client, "352", channel->name() + " " + (*it)->user + " " + (*it)->host + " "
				SERVER_NAME " " + (*it)->nick + " H" + (channel->is_op(*it) ? "@" : "")
				+ " :0 " + (*it)->realname);
	}
	else if (Client* target = find_client(mask)) {
		reply(client, "352", "* " + target->user + " " + target->host + " "
			SERVER_NAME " " + target->nick + " H :0 " + target->realname);
	}
	reply(client, "315", mask + " :End of /WHO list");
}

/* ------------------------------------------------------------------------- */
/*                                messaging                                  */
/* ------------------------------------------------------------------------- */

// PRIVMSG and NOTICE share delivery; NOTICE must never trigger error replies.
void	Server::deliver(Client& client, const Message& msg, bool notice) {
	std::string	cmd = notice ? "NOTICE" : "PRIVMSG";

	if (msg.params.empty()) {
		if (!notice)
			reply(client, "411", ":No recipient given (" + cmd + ")");
		return;
	}
	if (msg.params.size() < 2 || msg.params[1].empty()) {
		if (!notice)
			reply(client, "412", ":No text to send");
		return;
	}

	std::vector<std::string>	targets = split(msg.params[0], ',');
	const std::string&			text = msg.params[1];

	for (std::size_t i = 0; i < targets.size(); i++) {
		const std::string&	target = targets[i];
		if (is_channel_name(target)) {
			Channel*	channel = find_channel(target);
			if (!channel) {
				if (!notice)
					reply(client, "403", target + " :No such channel");
			}
			else if (!channel->has(&client)) {
				if (!notice)
					reply(client, "404", channel->name() + " :Cannot send to channel");
			}
			else
				channel->broadcast(":" + client.prefix() + " " + cmd + " " + channel->name() + " :" + text, &client);
		}
		else {
			Client*	dest = find_client(target);
			if (!dest) {
				if (!notice)
					reply(client, "401", target + " :No such nick/channel");
			}
			else
				dest->send(":" + client.prefix() + " " + cmd + " " + dest->nick + " :" + text);
		}
	}
}

void	Server::cmd_privmsg(Client& client, const Message& msg) {
	deliver(client, msg, false);
}

void	Server::cmd_notice(Client& client, const Message& msg) {
	deliver(client, msg, true);
}

/* ------------------------------------------------------------------------- */
/*                           operator commands                               */
/* ------------------------------------------------------------------------- */

void	Server::cmd_kick(Client& client, const Message& msg) {
	if (msg.params.size() < 2)
		return reply(client, "461", "KICK :Not enough parameters");

	Channel*	channel = find_channel(msg.params[0]);
	if (!channel)
		return reply(client, "403", msg.params[0] + " :No such channel");
	if (!channel->has(&client))
		return reply(client, "442", channel->name() + " :You're not on that channel");
	if (!channel->is_op(&client))
		return reply(client, "482", channel->name() + " :You're not channel operator");

	std::vector<std::string>	nicks = split(msg.params[1], ',');
	std::string					reason = msg.params.size() > 2 ? msg.params[2] : client.nick;

	for (std::size_t i = 0; i < nicks.size(); i++) {
		Client*	target = find_client(nicks[i]);
		if (!target || !channel->has(target)) {
			reply(client, "441", nicks[i] + " " + channel->name() + " :They aren't on that channel");
			continue;
		}
		channel->broadcast(":" + client.prefix() + " KICK " + channel->name() + " " + target->nick + " :" + reason);
		channel->remove(target);
	}
	if (channel->empty())
		delete_channel(channel);
}

void	Server::cmd_invite(Client& client, const Message& msg) {
	if (msg.params.size() < 2)
		return reply(client, "461", "INVITE :Not enough parameters");

	Client*	target = find_client(msg.params[0]);
	if (!target)
		return reply(client, "401", msg.params[0] + " :No such nick/channel");
	Channel*	channel = find_channel(msg.params[1]);
	if (!channel)
		return reply(client, "403", msg.params[1] + " :No such channel");
	if (!channel->has(&client))
		return reply(client, "442", channel->name() + " :You're not on that channel");
	if (channel->invite_only && !channel->is_op(&client))
		return reply(client, "482", channel->name() + " :You're not channel operator");
	if (channel->has(target))
		return reply(client, "443", target->nick + " " + channel->name() + " :is already on channel");

	channel->invite(target);
	reply(client, "341", target->nick + " " + channel->name());
	target->send(":" + client.prefix() + " INVITE " + target->nick + " :" + channel->name());
}

// MODE <channel> [(+|-)itkol [params...]]
void	Server::cmd_mode(Client& client, const Message& msg) {
	if (msg.params.empty())
		return reply(client, "461", "MODE :Not enough parameters");

	const std::string&	target = msg.params[0];
	if (!is_channel_name(target)) {
		// user modes are not supported; answer queries on ourselves only
		Client*	user = find_client(target);
		if (!user)
			return reply(client, "401", target + " :No such nick/channel");
		if (user != &client)
			return reply(client, "502", ":Cannot change mode for other users");
		if (msg.params.size() == 1)
			reply(client, "221", "+");
		return;
	}

	Channel*	channel = find_channel(target);
	if (!channel)
		return reply(client, "403", target + " :No such channel");
	if (msg.params.size() == 1) {
		reply(client, "324", channel->name() + " " + channel->modes(channel->has(&client)));
		reply(client, "329", channel->name() + " " + itos(channel->created()));
		return;
	}

	const std::string&	modestr = msg.params[1];
	if (modestr == "b" || modestr == "+b")
		return reply(client, "368", channel->name() + " :End of channel ban list");
	if (!channel->is_op(&client))
		return reply(client, "482", channel->name() + " :You're not channel operator");

	char		sign = '+';
	char		last_sign = 0;
	std::size_t	arg = 2;
	std::string	applied;
	std::string	applied_args;

	for (std::size_t i = 0; i < modestr.size(); i++) {
		char		c = modestr[i];
		std::string	param;

		if (c == '+' || c == '-') {
			sign = c;
			continue;
		}
		bool	adding = (sign == '+');
		switch (c) {
			case 'i':
				channel->invite_only = adding;
				break;
			case 't':
				channel->topic_restricted = adding;
				break;
			case 'k':
				if (adding) {
					if (arg >= msg.params.size()) {
						reply(client, "461", "MODE :Not enough parameters");
						continue;
					}
					param = msg.params[arg++];
					if (param.empty() || param.find(' ') != std::string::npos) {
						reply(client, "525", channel->name() + " :Key is not well-formed");
						continue;
					}
					channel->key = param;
				}
				else {
					if (arg < msg.params.size())
						arg++;
					channel->key.clear();
				}
				break;
			case 'o': {
				if (arg >= msg.params.size()) {
					reply(client, "461", "MODE :Not enough parameters");
					continue;
				}
				std::string	nick = msg.params[arg++];
				Client*		member = find_client(nick);
				if (!member || !channel->has(member)) {
					reply(client, "441", nick + " " + channel->name() + " :They aren't on that channel");
					continue;
				}
				channel->set_op(member, adding);
				param = member->nick;
				break;
			}
			case 'l':
				if (adding) {
					if (arg >= msg.params.size()) {
						reply(client, "461", "MODE :Not enough parameters");
						continue;
					}
					long	n = std::atol(msg.params[arg++].c_str());
					if (n <= 0)
						continue;
					channel->limit = n;
					param = itos(n);
				}
				else
					channel->limit = 0;
				break;
			default:
				reply(client, "472", std::string(1, c) + " :is unknown mode char to me for " + channel->name());
				continue;
		}
		if (sign != last_sign) {
			applied += sign;
			last_sign = sign;
		}
		applied += c;
		if (!param.empty())
			applied_args += " " + param;
	}
	if (!applied.empty())
		channel->broadcast(":" + client.prefix() + " MODE " + channel->name() + " " + applied + applied_args);
}
