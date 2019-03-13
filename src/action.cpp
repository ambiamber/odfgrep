/***************************************************************************
 *   Copyright (C) 2006 by Ray Lischner   *
 *   odf@tempest-sw.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "action.hpp"

#include <cstdlib>
#include <iostream>
#include <ostream>
#include <string>

void action::finish_file(std::string const&, long)
{}

void action::finish_all()
{}

void action::initialize()
{}


bool count::perform(std::string const&, std::string const&)
{
  return true;
}

void count::finish_file(std::string const& filename, long count)
{
  if (not filename.empty())
    std::cout << filename << ": ";
  std::cout << count << '\n';
}


bool echo_text::perform(std::string const& text, std::string const& filename)
{
  if (not filename.empty())
    std::cout << filename << ": ";
  std::cout << text << '\n';
  return true;
}


bool echo_file::perform(std::string const& text, std::string const& filename)
{
  std::cout << filename << '\n';
  return false;
}


bool echo_nomatch::perform(std::string const&, std::string const&)
{
  return false;
}

void echo_nomatch::finish_file(std::string const& filename, long count)
{
  if (count == 0)
    std::cout << filename << '\n';
}


bool quiet::perform(std::string const&, std::string const&)
{
  std::exit(EXIT_SUCCESS);
}

void quiet::finish_all()
{
  std::exit(EXIT_FAILURE);
}
