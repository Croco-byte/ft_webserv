/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   AutoIndex.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: user42 <user42@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/05/26 11:07:45 by user42            #+#    #+#             */
/*   Updated: 2021/05/30 12:12:47 by user42           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef AUTOINDEX_HPP
# define AUTOINDEX_HPP

#include "server/Server.hpp"
#include "utils/include.hpp"
#include <iostream>
#include <dirent.h>
#include <vector>

class Server;

class AutoIndex
{
	public:
		/* CONSTRUCTORS | DESTRUCTOR */
		AutoIndex(std::string const & url, std::string const & dirPath);
		~AutoIndex();

		/* CORE FUNCTIONS */
		void					createIndex(void);

		/* GETTERS | SETTERS */
		std::string	const &		getIndex(void) const;

	private:
		std::string						_url;
		std::string						_dirPath;
		std::vector<std::string>		_filenames;
		std::vector<std::string>		_entries;

		std::string						_index;

		/* PRIVATE HELPERS : PROCESSING CURRENT DIRECTORY */
		void				getFilenames(void);
		void				processFilenames(void);
		void				createEntries(void);
		void				addIndexLine(int line);

		/* PRIVATE HELPERS : COMPARING FILENAMES */
		static bool			compareFilenames(std::string const & entry_a, std::string const & entry_b);

};

#endif
