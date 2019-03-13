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

#ifndef ZIP_HPP
#define ZIP_HPP

/** @file zip.hpp
 * A few simple wrapper classes for libzip.
 * These classes serve my needs for extracting OpenOffice.org documents and
 * are not meant to be full-featured.
*/

#include <cstdlib>
#include <string>
#include <stdexcept>
#include <vector>
#include <zip.h>

/// All the zip wrappers are in this namespace.
namespace Zip
{

class Archive;
class File;
class Source;

/** Exception for zip errors. */
class Exception : public std::runtime_error
{
public:
  /// Construct an exception object.
  /// @param prefix a prefix to the error message, e.g., a file name
  /// @param msg the error message
  Exception(std::string const& prefix, std::string const& msg)
  : std::runtime_error(make_msg(prefix, msg))
  {}
  /// Construct an exception object.
  /// @param msg the error message
  Exception(std::string const& msg) : std::runtime_error(msg) {}
  /// Construct an exception object.
  /// @param prefix a prefix to the error message, e.g., a file name
  /// @param a a zip archive; the error message comes from its error code.
  Exception(std::string const& prefix, Archive& a)
  : std::runtime_error(make_msg(prefix, a))
  {}
  /// Construct an exception object.
  /// @param prefix a prefix to the error message, e.g., a file name
  /// @param f a zip file; the error message comes from its error code.
  Exception(std::string const& prefix, File& f)
  : std::runtime_error(make_msg(prefix, f))
  {}
  /// Construct an exception object.
  /// @param prefix a prefix to the error message, e.g., a file name
  /// @param error a ziplib error code
  Exception(std::string const& prefix, int error)
  : std::runtime_error(make_msg(prefix, error))
  {}
private:
  /// Make an error message string from a prefix and message body.
  /// If the prefix is not empty, insert <tt>": "</tt> between
  /// the prefix and the body.
  /// @param prefix the prefix string, e.g., a file name
  /// @param msg the error message body
  /// @returns the complete error message
  std::string make_msg(std::string const& prefix, std::string const& msg);
  /// Make an error message string from a prefix and message body.
  /// If the prefix is not empty, insert <tt>": "</tt> between
  /// the prefix and the body.
  /// @param prefix the prefix string, e.g., a file name
  /// @param a a zip archive; the error message comes from its error code
  /// @returns the complete error message
  std::string make_msg(std::string const& prefix, Archive& a);
  /// Make an error message string from a prefix and message body.
  /// If the prefix is not empty, insert <tt>": "</tt> between
  /// the prefix and the body.
  /// @param prefix the prefix string, e.g., a file name
  /// @param f a zip file; the error message comes from its error code
  /// @returns the complete error message
  std::string make_msg(std::string const& prefix, File& f);
  /// Make an error message string from a prefix and message body.
  /// If the prefix is not empty, insert <tt>": "</tt> between
  /// the prefix and the body.
  /// @param prefix the prefix string, e.g., a file name
  /// @param error a ziplib error code
  /// @returns the complete error message
  std::string make_msg(std::string const& prefix, int error);
};


/// Wrapper for a zip archive.
class Archive
{
  public:
    /// Flags for opening the archive file.
    enum Flags { create = ZIP_CREATE, exclusive = ZIP_EXCL, check = ZIP_CHECKCONS, noflags = 0 };
    enum FileFlags { unchanged = ZIP_FL_UNCHANGED, nofileflags = 0 };

    /// Construct the archive object and open the archive file.
    /// @param filename path to the zip archive file
    /// @param flags flags for opening the file
    Archive(std::string const& filename, Flags flags = noflags);
    /// Construct the archive object and open the archive file.
    /// @param filename path to the zip archive file
    /// @param flags flags for opening the file
    Archive(char const* filename, Flags flags = noflags);
    /// Default constructor. Does not open any file.
    Archive() : zip_(0), filename_("") {}
    /// Destructor automatically closes the archive.
    ~Archive();

    /// Opens a new archive. Closes the old archive if there was one open.
    /// @param filename path to the zip archive file
    /// @param flags flags for opening the file
    /// @throw Exception if the file cannot be opened
    void open(char const* filename, Flags flags = noflags);
    /// Closes the archive if open.
    /// @returns true for success or already closed; false for failure
    /// @throw Exception for any error when closing the file
    void close();

    /// Get error message text.
    /// @returns an error message for the current error state
    std::string get_error_msg() const;

    /// Get the archive file name.
    /// @returns the file name that was used to open the archive or an empty string.
    std::string filename() const { return filename_; }

    /// Return the number of files in the archive.
    int get_num_files() const { return zip_get_num_files(zip_); }
    std::string get_file(int n, FileFlags flags = unchanged) const;

    /// Add a file to the archive.
    /// @param name the name of the file to add
    /// @param source the contents of the file to add
    void add(std::string const& name, std::string const& source);
    /// Copy a file from another archive
    /// @param src the source archive, from which to copy
    /// @param index the index (0-based) of the file to copy from @p src
    void copy(Archive& src, int index);
    /// Replace the contents of a file.
    /// @param index the index (0-based) of the file to modify in this archive
    /// @param source the new contents of the file
    void replace(int index, std::string const& source);

  private:

    /// Represent a ZIP source.
    /// A source provides the contents of a new file to be added
    /// to an archive. A libzip source can be a memory buffer,
    /// a file, a member of another archive, or a callback function
    /// that provides the source contents. This class implements
    /// only memory buffer and archive element because that's
    /// all I needed at the time.
    class Source
    {
      public:
        /// Create a Source object from a raw zip_source pointer.
        /// @param src the zip_source pointer
        Source(zip_source* src) : source_(src) {}
        /// Create a Source object that points to another archive object.
        /// That other archive must remain open until this Source has been consumed.
        /// @param owner the owner of the archive that will receive this Source
        /// @param source the source archive, from which a file will be copied
        /// @param index the index (0-based) of the file to copy from @p source
        Source(Archive& owner, Archive& source, int index);
        /// Create a Source object from a string's contents.
        /// The string must remain live until this Source has been consumed.
        /// @param owner the owner of the archive that will receive this Source
        /// @param src the file contents
        Source(Archive& owner, std::string const& src);
        // Do not call zip_source_free. When this source is used
        // to create an entry in an archive, that archive owns
        // the source.

        /// Return the raw zip_source pointer.
        zip_source* source() const { return source_; }

      private:
        void operator=(Source const&);   // do not implement
        Source(Source const&);           // do not implement
        zip_source* source_;
    };

    friend class File;
    friend class Source;

    Archive(Archive&);        ///< not implemented to avoid problems copying zip_
    void operator=(Archive&); ///< not implemented to avoid problems copying zip_

    std::string filename_;    ///< save the error code in case zip_ is NULL
    zip* zip_;                ///< pointer to the zip archive object
};

/// Represent a single file within an archive.
class File
{
public:
  /// Flags for opening a file.
  enum Flags { nocase = ZIP_FL_NOCASE, nodir = ZIP_FL_NODIR, compressed = ZIP_FL_COMPRESSED,
               unchanged = ZIP_FL_UNCHANGED, noflags = 0 };
  /// Constructor to open a file within an archive.
  /// @param a Archive that contains the file
  /// @param filename name of the file to open
  /// @param flags flags for opening the file
  /// @throw Exception if the file cannot be opened
  File(Archive& a, char const* filename, Flags flags = noflags);
  /// Destructor automatically closes the file
  ~File();

  /// Close the file.
  /// @throw Exception for any error closing the file
  void close();
  /// Read a buffer of data from the current position in the file.
  /// @param buffer pointer to start of buffer where data will be stored
  /// @param nbytes read up to @p nbytes characters from the file
  /// @returns the number of bytes actually read from the file and stored in @p buffer
  /// @throw Exception for read errors
  int read(unsigned char* buffer, int nbytes);
  /// Read the entire file and return the contents as a string.
  /// @returns a string that contains the entire contents of the file
  /// @throw Exception for read errors
  std::string read();

  /// Get error message text.
  /// @returns an error message for the current error state
  std::string get_error_msg() const;

  /// Return the file name.
  /// @returns the file name
  std::string filename() const { return filename_; }
  /// Return the complete path name.
  /// The path name includes the archive name and the file name.
  std::string pathname() const;

private:
  File(File&);           ///< not implemented to avoid problems copying zf_
  void operator=(File&); ///< not implemented to avoid problems copying zf_

  Archive& archive_;     ///< a reference back to the archive that owns this file
  std::string filename_; ///< name of the file
  zip_file* zf_;         ///< pointer to the zip file object
};

}
#endif
