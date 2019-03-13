/***************************************************************************
 *   Copyright (C) 2006 by Ray Lischner                                    *
 *   odf@tempest-sw.com                                                    *
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
/// @file action.hpp Actions to take for a match

#ifndef ACTION_HPP
#define ACTION_HPP

#include <string>

/** Abstract base class for all actions.
 * An action is invoked for each match. The action does whatever the user
 * requested. The command line options determine which action to invoke.
 */
struct action
{
  /** Initialize prior to searching a file.
   */
  virtual void initialize();
  /** Invoke the action.
   * @param text the paragraph that matched
   * @param filename the name of the file that matched
   * @return true to continue looking for matches, false to stop reading this file
   */
  virtual bool perform(std::string const& text, std::string const& filename) = 0;
  /** Perform any required clean-up actions after searching a single file.
   * Default is to do nothing.
   * @param filename The name of the file that was just finished
   * @param count the number of matches in the file
   */
  virtual void finish_file(std::string const& filename, long count);
  /** Perform any required clean-up actions after finishing all files.
   * Default is to do nothing.
   */
  virtual void finish_all();
};

/** Print a count of the number of matches in a file.
 */
struct count : action
{
  /** Do nothing. */
  virtual bool perform(std::string const& text, std::string const& filename);
  /** Print the count */
  virtual void finish_file(std::string const& filename, long count);
};

/** Echo the matching text.
 */
struct echo_text : action
{
  virtual bool perform(std::string const& text, std::string const& filename);
};

/** Echo only the file name.
 */
struct echo_file : action
{
  virtual bool perform(std::string const& text, std::string const& filename);
};

/** Echo only the file name of files that contain no matching lines.
 */
struct echo_nomatch : action
{
  /** Stop searching after finding a match. */
  virtual bool perform(std::string const& text, std::string const& filename);
  /** Print the filename if it did not contain a match */
  virtual void finish_file(std::string const& filename, long count);
};

/** Exit successfully without printing anything.
 */
struct quiet : action
{
  /** Exit the program. */
  virtual bool perform(std::string const&, std::string const&);
  /** Exit with a failure status because no files contained a match. */
  virtual void finish_all();
};

#endif
