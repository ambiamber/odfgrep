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

#include <cwchar>
#include <string>

/// @file
/// Unicode functions.

// Doxygen comment in unicode.cpp.
std::wstring utf8_to_utf32(unsigned char const* inbuf, std::size_t size);

/// Convert UTF-8 to UTF-32.
/// @pre wchar_t must be able to hold a UTF-32 code point.
/// @param inbuf pointer to the UTF-8 character array
/// @param size number of bytes in @p inbuf
/// @returns the UTF-32 string
inline std::wstring utf8_to_utf32(char const* inbuf, std::size_t size)
{
  return utf8_to_utf32(reinterpret_cast<unsigned char const*>(inbuf), size);
}

/// Convert UTF-8 to UTF-32.
/// @pre wchar_t must be able to hold a UTF-32 code point.
/// @param inbuf pointer to the UTF-8 character array
/// @param size number of bytes in @p inbuf
/// @returns the UTF-32 string
inline std::wstring utf8_to_utf32(std::string const& inbuf)
{
  return utf8_to_utf32(inbuf.c_str(), inbuf.size());
}
