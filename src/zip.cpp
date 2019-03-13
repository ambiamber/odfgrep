/***************************************************************************
 *   Copyright (C) 2006 by Ray Lischner                                    *
 *   codex@tempest-sw.com                                                  *
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

/// @file zip.cpp
/// Implement the Zip namespace classes

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <vector>
#include <zip.hpp>

namespace Zip
{

std::string Exception::make_msg(std::string const& prefix, std::string const& msg)
{
  if (prefix.empty())
    return msg;
  else
    return prefix + ": " + msg;
}

std::string Exception::make_msg(std::string const& prefix, Archive& a)
{
  return make_msg(prefix, a.get_error_msg());
}

std::string Exception::make_msg(std::string const& prefix, File& f)
{
  return make_msg(prefix, f.get_error_msg());
}

std::string Exception::make_msg(std::string const& prefix, int zip_err)
{
  // Save @c errno immediately because other code might change it.
  int sys_err = errno;
  // The return value from zip_error_to_str does not include the trailing NUL byte,
  // so make sure to allocate an extra byte in buffer. Also make sure not to count
  // that byte when constructing the final string.
  int nbytes = zip_error_to_str(0, 0, zip_err, sys_err);
  std::vector<char> buffer(nbytes+1);
  zip_error_to_str(&buffer[0], buffer.size(), zip_err, sys_err);

  return make_msg(prefix, std::string(&buffer[0], buffer.size()-1));
}

File::File(Archive& a, char const* filename, Flags flags)
  : archive_(a), filename_(filename), zf_(0)
{
  zf_ = zip_fopen(a.zip_, filename, flags);
  if (zf_ == 0)
    throw Exception(pathname(), a);
}

File::~File()
{
  close();
}

void File::close()
{
  if (zf_ != 0)
  {
    int error = zip_fclose(zf_);
    if (error != 0)
      throw Exception(pathname(), error);
    zf_ = 0;
    filename_ = "";
  }
}

int File::read(unsigned char* buffer, int nbytes)
{
  assert(zf_ != 0);
  int bytes_read = zip_fread(zf_, buffer, nbytes);
  if (bytes_read < 0)
    throw Exception(pathname(), *this);
  return bytes_read;
}

std::string File::read()
{
  unsigned char buffer[1024];
  int nbytes;
  std::string result;

  while ((nbytes = read(buffer, sizeof(buffer))) > 0)
    result.append(reinterpret_cast<char*>(buffer), nbytes);

  return result;
}

std::string File::get_error_msg()
const
{
  return zip_file_strerror(zf_);
}

std::string File::pathname()
const
{
  return archive_.filename() + '[' + filename() + ']';
}



Archive::Archive(char const* filename, Flags flags)
: zip_(0), filename_(filename)
{
  open(filename, flags);
}

Archive::Archive(std::string const& filename, Flags flags)
  : zip_(0), filename_(filename)
{
  open(filename.c_str(), flags);
}

Archive::~Archive()
{
  close();
}

void Archive::open(char const* filename, Flags flags)
{
  close();

  int error;
  zip_ = zip_open(filename, flags, &error);
  if (zip_ == 0)
    throw Exception(filename, error);
  filename_ = filename;
}

void Archive::close()
{
  if (zip_ != 0 and zip_close(zip_) != 0)
    throw Exception(filename_, *this);
  zip_ = 0;
  filename_ = "";
////  clear_sources();
}

std::string Archive::get_error_msg()
const
{
  return zip_strerror(zip_);
}

std::string Archive::get_file(int n, FileFlags flags)
const
{
  return zip_get_name(zip_, n, flags);
}


void Archive::copy(Archive& source, int index)
{
  Source src(*this, source, index);
  if (zip_add(zip_, zip_get_name(source.zip_, index, 0), src.source()) != 0)
    throw Exception(filename_, *this);
}

void Archive::add(std::string const& name, std::string const& source)
{
  Source src(*this, source);
  if (zip_add(zip_, name.c_str(), src.source()) != 0)
    throw Exception(filename_, *this);
}

void Archive::replace(int index, std::string const& source)
{
  Source src(*this, source);
  if (zip_replace(zip_, index, src.source()) != 0)
    throw Exception(filename_, *this);
}

Archive::Source::Source(Archive& owner, Archive& source, int index)
: source_(zip_source_zip(owner.zip_, source.zip_, index, ZIP_FL_UNCHANGED, 0, -1))
{}

Archive::Source::Source(Archive& owner, std::string const& src)
: source_(0)
{
  void* buffer = std::malloc(src.size());
  std::memcpy(buffer, src.data(), src.size());
  source_ = zip_source_buffer(owner.zip_, buffer, src.size(), true);
}

}
