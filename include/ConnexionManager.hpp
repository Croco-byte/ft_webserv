/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConnexionManager.hpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: user42 <user42@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/05/22 10:19:29 by user42            #+#    #+#             */
/*   Updated: 2021/05/23 12:56:47 by user42           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONNEXIONMANAGER_HPP
# define CONNEXIONMANAGER_HPP

#include "Server.hpp"
#include "Console.hpp"
#include <utility>
#include <vector>
#include <map>

class	ConnexionManager
{
	public:

		/* --- CONSTRUCTORS / DESTRUCTOR --- */
		ConnexionManager();
		~ConnexionManager();

		/* --- CORE FUNCTIONS ---*/
		int						setup(void);
		void					run(void);
		void					clean(void);

		/* --- UTILITY FUNCTIONS --- */
		void					addServer(Server toAdd);


	private:
		ConnexionManager(ConnexionManager const & src);
		ConnexionManager &			operator=(ConnexionManager const & rhs);

		std::vector<Server>			_vecServers;
		std::map<long,Server *>		_listen_fds;
		std::map<long,Server *>		_read_fds;
		std::vector<int>			_write_fds;

		fd_set						_fd_set;
		unsigned int				_fd_size;
		long						_max_fd;

};

#endif
