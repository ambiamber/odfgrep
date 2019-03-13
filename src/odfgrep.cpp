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

/** @file odfgrep.cpp
 * Open Document Format grep utility
 * Implement grep to search ODF documents, such as those created by
 * OpenOffice.org, KWrite, and other tools.
 */

/** @mainpage odfgrep - Open Document Format grep utility
 * Search the contents of ODF documents in the manner of grep.
 * @author Ray Lischner
 * @version 0.1
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <stack>
#include <string>
#include <sstream>
#include <vector>

#include <boost/regex.hpp>

#include "action.hpp"
#include "unicode.hpp"
#include "xml.hpp"
#include "zip.hpp"

extern "C" {
#include <argp.h>
#include <libxml/parser.h>
}

namespace
{

enum exit_status { success, nomatch, io_error, cmdline_error };

enum when { never, always, multiple }; ///< When to print file names
when print_filename = multiple; ///< When to print filenames
bool have_documents = false; ///< True if at least one document has been processed
bool have_pattern = false;   ///< True if the pattern has been specified on the command line
bool search_meta = false;    ///< True means to search meta.xml in addition to content.xml
bool invert = false;         ///< True means a match is when the regexp does NOT match the text
bool search_deleted = false; ///< Search in deleted text, that is, inside \<deletion\> elements
long max_count = 0;          ///< Maximum number of matches per file
long match_count;            ///< Number of matches in one file
boost::regex_constants::syntax_option_type flags; ///< icase and other flags
boost::regex_constants::syntax_option_type flavor = boost::regex_constants::grep; ///< Pattern type: perl, grep, egrep, or literal

exit_status status = nomatch; ///< Exit status (set to success after any match)

std::vector<std::string> documents; ///< list of documents to search

boost::wregex pattern; ///< The regexp
std::string pattern_text; ///< The regexp string from the command line
std::auto_ptr<action> act; ///< The action to take when a match is found

std::string const emptystr; ///< global empty string

/** Read the pattern from a file.
  * @param filename the name of the file to read
  * @return the pattern, as read from @p filename
  */
std::string read_pattern(char const* filename)
{
  std::ifstream in(filename);
  if (not in)
  {
    perror(filename);
    std::exit(cmdline_error);
  }
  std::ostringstream out;
  out << in.rdbuf();
  return out.str();
}

/** Test one paragraph for a match.
 * If the paragraph matches, perform the action, set the exit status to success,
 * and increment the match count. The user can request that searching stop
 * at a predetermined match count.
 * @param text the text to search
 * @param filename the name of the file that contains the @p text
 * @return true to continue searching for matches or false to stop searching this file
 */
bool match(std::string const& text, std::string const& filename)
{
  bool result = true;
  if (boost::regex_search(utf8_to_utf32(text), pattern, boost::match_any) != invert)
  {
    if (max_count == 0 or match_count != max_count)
    {
      result = act->perform(text, filename);
      status = success;
      ++match_count;
    }
  }
  return result;
}
bool match(xmlChar const* text, int len, std::string const& filename)
{
  bool result = true;
  if (boost::regex_search(utf8_to_utf32(text, len), pattern, boost::match_any) != invert)
  {
    if (max_count == 0 or match_count != max_count)
    {
      result = act->perform(xml::string(text, len), filename);
      status = success;
      ++match_count;
    }
  }
  return result;
}

/** Grep a meta stream in a document.
 * Extract the text, one node at a time,
 * and match the pattern against the node's contents.
 * When a match is found, include the node name in the identifying file name.
 * @param file the stream in the document
 * @return true to keep searching this document
 */
bool grep_meta(Zip::File& file, std::string filename)
{
  std::string text(file.read());
  xml::doc doc;
  doc.parse(text);

  // The root element is <document:meta>, and its child is <meta>.
  for (xmlNode* node = doc.get_root_element()->children->children; node != 0; node = node->next)
  {
    std::string text = xml::get_content(node);
    std::string file = (print_filename ? filename + "<" + xml::string(node->name) + ">" : emptystr);
    if (not match(text, file))
      return false;
  }
  return true;
}

/** Grep a node in a document body.
 * @param parent the parent &lt;text&gt; node
 * @return true to continue searching for matches or false to stop searching this file
 */
bool grep_node(xmlNode* parent, std::string const& filename)
{
  for (xmlNode* node = parent->children ; node != 0; node = node->next)
  {
    if (std::strcmp(xml::charptr(node->name), "p") == 0 or
        std::strcmp(xml::charptr(node->name), "h") == 0)
    {
      if (not match(xml::get_content(node), filename))
        return false;
    }
    else if (search_deleted or std::strcmp(xml::charptr(node->name), "deletion") != 0)
    {
      // Recursively search the contents of a section, table, list, index, etc.
      // The only elements not to check recursively are for deleted text.
      if (not grep_node(node, filename))
        return false;
    }
  }
  return true;
}

/** Grep a document body.
 * Find the \<text\> element as a child of \<body\>.
 * @param parent the \<body\> node.
 * @param filename the document file name
 */
void grep_body(xmlNode* parent, std::string const& filename)
{
  for (xmlNode* node = parent->children ; node != 0; node = node->next)
  {
    if (std::strcmp(xml::charptr(node->name), "text") == 0)
    {
      grep_node(node, filename);
      break;
    }
  }
}

/** Grep a content stream in a document.
 * Extract the text, one paragraph at a time,
 * and match the pattern against the paragraph.
 * All ODF documents have \<document-content\> as the root
 * element. Text documents can contain scripts and whatnot,
 * and the body of the document is contained in the \<body\>
 * element. It contains styles and whatnot, and the main text
 * is found in the \<text\> element.
 * @param file the stream in the document
 * @param filename the document filename
 */
void grep_content(Zip::File& file, std::string filename)
{
  std::string text(file.read());
  xml::doc doc;
  doc.parse(text);
  for (xmlNode* node = doc.get_root_element()->children ; node != 0; node = node->next)
    if (std::strcmp(xml::charptr(node->name), "body") == 0)
    {
      grep_body(node, filename);
      break;
    }
}

/** Grep a document.
 * Open the document as a ZIP file, and then open the content.xml stream
 * (and optionally the meta.xml stream). Grep the stream.
 * @param document the path to the document file
 */
void grep_document(std::string const& document)
{
  act->initialize();
  try
  {
    Zip::Archive zip(document);
    match_count = 0;


    if (search_meta)
    {
      Zip::File meta(zip, "meta.xml");
      grep_content(meta, print_filename ? document : emptystr);
    }

    Zip::File content(zip, "content.xml");
    grep_content(content, print_filename ? document : emptystr);
  }
  catch (Zip::Exception& ex)
  {
    std::cerr << ex.what() << '\n';
    status = io_error;
  }
  catch(...)
  {
    throw;
  }
  act->finish_file(document, match_count);
}

/** Command line argument parser. The ARGP package calls back
 * to this function for every command line argument.
 * @param key the command line option or a magic ARGP value
 * @param arg the argument to a command line option, if present
 * @param state internal state for the ARGP processor
 * @returns 0 for success or @c ARGP_ERR_UNKNOWN for any error
 */
extern "C" error_t parse_func(int key, char *arg, struct argp_state *state)
{
  char *end;

  switch (key)
  {
    case 'c':
      act.reset(new count);
      break;
    case 'd':
      search_deleted = true;
      break;
    case 'e':
      pattern_text = arg;
      have_pattern = true;
      break;
    case 'E':
      flavor = boost::regex_constants::egrep;
      break;
    case 'f':
      pattern_text = read_pattern(arg);
      have_pattern = true;
      break;
    case 'F':
      flavor = boost::regex_constants::literal;
      break;
    case 'G':
      flavor = boost::regex_constants::grep;
      break;
    case 'h':
      print_filename = never;
      break;
    case 'H':
      print_filename = always;
      break;
    case 'i':
      flags |= boost::regex_constants::icase;
      break;
    case 'l':
      act.reset(new echo_file);
      break;
    case 'L':
      act.reset(new echo_nomatch);
      break;
    case 'P':
      flavor = boost::regex_constants::perl;
      break;
    case 'q':
      act.reset(new quiet);
      break;
    case 'm':
      max_count = std::strtol(arg, &end, 10);
      if (*end != '\0')
      {
        std::cerr << "Not a number: " << arg << '\n';
        std::exit(cmdline_error);
      }
    case 'M':
      search_meta = true;
      break;
    case 'v':
      invert = true;
      break;
    case 'V':
      std::cout << PACKAGE_STRING "\n"
          "Copyright (c) 2006 Ray Lischner <" PACKAGE_BUGREPORT ">\n"
          "This is free software; see the source for copying conditions.  There is NO\n"
          "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n";
      std::exit(EXIT_SUCCESS);
    case ARGP_KEY_ARG:
      if (have_pattern)
        documents.push_back(arg);
      else
      {
        pattern_text = arg;
        have_pattern = true;
      }
      break;
    case ARGP_KEY_INIT:
    case ARGP_KEY_END:
    case ARGP_KEY_SUCCESS:
    case ARGP_KEY_NO_ARGS:
      break;
    case ARGP_KEY_FINI:
      if (documents.empty())
        argp_usage(state); // does not return
      break;
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

} // end of namespace

/** The main program. As you can see, the main program is pretty simple.
 * It sets up some XML stuff, then calls on ARGP to process the command line.
 * @param argc number of command line arguments
 * @param argv the command line arguments
 * @returns 0 for success, EXIT_FAILURE if anything goes wrong.
 */
int main(int argc, char *argv[])
{
  static argp_option options[] = {
    { "basic-regexp",        'G', 0,         0, "PATTERN uses basic POSIX syntax" },
    { "count",               'c', 0,         0, "do not echo matching lines, but count the number of matches per file (or with -v, number of non-matching lines)" },
    { "deleted",             'd', 0,         0, "search in deleted text" },
    { "extended-regexp",     'E', 0,         0, "PATTERN uses exended POSIX regexp syntax" },
    { "file",                'f', "FILE",    0, "read regexps from FILE, one per line" },
    { "files-without-match", 'L', 0,         0, "print only names of files that contain no lines that match PATTERN"},
    { "files-with-match",    'l', 0,         0, "print only names of files that match PATTERN"},
    { "fixed-strings",       'F', 0,         0, "PATTERN is a list of newline-separated strings to match, not regular expressions" },
    { "ignore-case",         'i', 0,         0, "ignore case distinctions"},
    { "invert-match",        'v', 0,         0, "invert match: print lines that do not match PATTERN" },
    { "max-count",           'm', "COUNT",   0, "stop reading after COUNT matches in one document" },
    { "meta",                'M', 0,         0, "search meta.xml in addition to content.xml"},
    { "no-filename",         'h', 0,         0, "do not print filenames, even if multiple files are named on command line" },
    { "perl-regexp",         'P', 0,         0, "PATTERN uses Perl syntax" },
    { "quiet",               'q', 0,         0, "do not write anything; exit status is 0 for a match" },
    { "regexp",              'e', "PATTERN", 0, "match PATTERN; use this option if PATTERN starts with -"},
    { "version",             'V', 0,         0, "print version number and exit" },
    { "with-filename",       'H', 0,         0, "print filename even if only one file is named on command line" },
    { 0 }
  };

  static argp parse_info = { options, parse_func, "PATTERN DOCUMENTS...",
    "Search for regular expressions in ODF documents.\v"
        "Each file name named on the command line is opened as "
        "an OASIS Open Document Format document, that is, as a ZIP file "
        "that contains XML streams. The main content stream (content.xml) "
        "is parsed according to the ISO/OASIS ODF standard. "
        "Text paragraphs are compared with PATTERN, "
        "and matching lines are printed "
        "to the standard output."
        "\n\n"
        "ODF documents use UTF-8 encoding, so the PATTERN "
        "is also interpreted as UTF-8, regardless of current locale. "
        "All regular expression matching is performed internally "
        "using UTF-32 code points."
  };

  LIBXML_TEST_VERSION;
  xml::parser parser;
  try {
    argp_parse(&parse_info, argc, argv, 0, 0, 0);
    if (print_filename == multiple)
      print_filename = (documents.size() == 1 ? never : always);
    assert(have_pattern);
    pattern.assign(utf8_to_utf32(pattern_text), flavor | flags);
    if (act.get() == 0)
      act.reset(new echo_text);
    std::for_each(documents.begin(), documents.end(), grep_document);
    act->finish_all();
  } catch(std::exception& ex) {
    std::cerr << ex.what() << '\n';
    status = io_error;
  }
  return status;
}
