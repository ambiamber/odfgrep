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

#include "unicode.hpp"
#include <cassert>
#include <cwchar>
#include <stdexcept>
#include <string>

/** Convert a UTF-8 string to UTF-32.
 * Only basic checking is performed. This function does not
 * detect all erroneous UTF-8 strings, only those that are
 * obvious or interfere with the logic of the conversion function.
 *
 * Note that I tried a table-lookup version of this function,
 * but it was slower (on an Athlon XP 3000+ running Suse Linux 10.0).
 *
 * @param inbuf pointer to the UTF-8 byte sequence
 * @param size the number of bytes in @p inbuf
 * @return the UTF-32 string
 * @pre wide execution character set is UTF-32
 * @throws std::runtime_error for an invalid UTF-8 string
 */
std::wstring utf8_to_utf32(unsigned char const* inbuf, std::size_t size)
{
  std::wstring result;

  for (unsigned char const *p = inbuf; size != 0; ++p, --size)
  {
    if ((*p & 0x80) == 0)
      result += *p;
    else if ((*p & 0xc0) == 0x80)
      throw std::runtime_error(std::string("invalid utf-8 encoding"));
    else
    {
      std::wint_t code;
      if ((*p & 0xe0) == 0xc0)
      {
        // two-byte encoding
        code = (*p & 0x1f) << 6;
        ++p;
        --size;
        if ((*p & 0xc0) != 0x80)
          throw std::runtime_error(std::string("invalid utf-8 encoding"));
        code |= *p & 0x3f;
      }
      else if ((*p & 0xf0) == 0xe0)
      {
        // three-byte encoding
        code = (*p & 0x0f) << 12;
        ++p;
        --size;
        if ((*p & 0xc0) != 0x80)
          throw std::runtime_error(std::string("invalid utf-8 encoding"));
        code |= (*p & 0x3f) << 6;
        ++p;
        --size;
        if ((*p & 0xc0) != 0x80)
          throw std::runtime_error(std::string("invalid utf-8 encoding"));
        code |= *p & 0x3f;
      }
      else
      {
        // four-byte encoding
        assert((*p & 0xf0) == 0xf0);
        code = (*p & 0x07) << 16;
        ++p;
        --size;
        if ((*p & 0xc0) != 0x80)
          throw std::runtime_error(std::string("invalid utf-8 encoding"));
        code |= (*p & 0x3f) << 12;
        ++p;
        --size;
        if ((*p & 0xc0) != 0x80)
          throw std::runtime_error(std::string("invalid utf-8 encoding"));
        code |= (*p & 0x3f) << 6;
        ++p;
        --size;
        if ((*p & 0xc0) != 0x80)
          throw std::runtime_error(std::string("invalid utf-8 encoding"));
        code |= *p & 0x3f;
      }
      result += static_cast<wchar_t>(code);
    }
  }

  return result;
}
