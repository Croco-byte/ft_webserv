/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   AutoIndex.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: user42 <user42@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/05/26 11:27:38 by user42            #+#    #+#             */
/*   Updated: 2021/05/30 14:42:27 by user42           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "response/AutoIndex.hpp"

/*
** ------ CONSTRUCTORS | DESTRUCTOR ------
*/
AutoIndex::AutoIndex(std::string const & url, std::string const & dirPath): _url(url), _dirPath(dirPath)
{ _index = "<h1>Index of " + _url + "</h1><br/><hr><br/><table><td><h3>Path</h3></td><td style=\"padding-left: 250px;\"><h3>Last Modified</h3>"; }

AutoIndex::~AutoIndex()
{}


/*
** ------ CORE FUNCTIONS ------
*/
void					AutoIndex::createIndex(void)
{
	int		i(0);
	int		lineNb;

	this->getFilenames();
	this->createEntries();
	lineNb = _filenames.size();

	while (i < lineNb)
	{
		this->addIndexLine(i);
		i++;
	}
	_index += "</table><br/><hr>";
}


/*
** ------ GETTERS | SETTERS ------
*/

std::string const &		AutoIndex::getIndex(void) const
{ return (this->_index); }


/*
** ------ PRIVATE HELPERS : PROCESSING CURRENT DIRECTORY ------
*/
void				AutoIndex::getFilenames(void)
{
	DIR				*dir;
	struct dirent	*file;

	dir = opendir(_dirPath.c_str());
	if (!dir)
		return ;
	file = readdir(dir);
	while (file)
	{
		_filenames.push_back(file->d_name);
		file = readdir(dir);
	}
	closedir(dir);
	this->processFilenames();
}

void				AutoIndex::processFilenames(void)
{
	std::vector<std::string>::iterator first = _filenames.begin();
	std::vector<std::string>::iterator last = _filenames.end();
	while (first != last)
	{
		if ((*first)[0] == '.' && *first != "..")
		{
			first = _filenames.erase(first);
			last = _filenames.end();
		}
		else
		{
			if (Utils::isDirectory(_dirPath + *first))
				*first = ((*first)[(*first).size() - 1] == '/') ? *first : *first = *first + "/";
			first++;
		}
	}
	sort(_filenames.begin(), _filenames.end(), compareFilenames);
}

void				AutoIndex::createEntries(void)
{
	_entries = _filenames;
	for (std::vector<std::string>::iterator it = _entries.begin(); it != _entries.end(); it++)
		*it = (_url[_url.size() - 1] == '/') ? _url + *it : _url + "/" + *it;
}

void			AutoIndex::addIndexLine(int line)
{
	_index += "<tr><td><a href=\"" + _entries[line] + "\">		" + _filenames[line] + "</a></td><td style=\"padding-left: 250px;\">" + Utils::getLastModified(_dirPath + _entries[line]) + "</td></tr>";
}


/*
** ------ PRIVATE HELPERS : COMPARING FILENAMES ------
*/
bool				AutoIndex::compareFilenames(std::string const & entry_a, std::string const & entry_b)
{
	if (entry_a[entry_a.size() - 1] == '/' && entry_b[entry_b.size() - 1] != '/')
		return (true);
	else if (entry_a[entry_a.size() - 1] != '/' && entry_b[entry_b.size() - 1] == '/')
		return (false);
	else
		return (entry_a < entry_b);
}

