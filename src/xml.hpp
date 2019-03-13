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

/** @file xml.hpp
 * Simple wrapper for libxml2 code.
 * This is not meant to be a complete wrapper package. Instead it
 * wraps only the few bits that the codex project needs.
 */
 
#ifndef XML_HPP
#define XML_HPP

#include <cstring>
#include <string>

extern "C"
{
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/relaxng.h>
}

/// All the XML code resides in this namespace.
namespace xml
{
  class doc;

  /// Wrapper class for relax-ng globals.
  /// This is a singleton class, although it makes no attempt to enforce this restriction.
  class relax_ng
  {
  public:
    /// Construct the singleton object, to initialize libxml2's relax-ng code.
    relax_ng();
    /// Destroy the singleton object, to cleanup libxml2's relax-ng code.
    ~relax_ng();
  };

  /// Wrapper class for parser globals.
  /// This is a singleton class, although it makes no attempt to enforce this restriction.
  class parser
  {
  public:
    /// Construct the singleton object, to initialize libxml2's parser code.
    parser();
    /// Destroy the singleton object, to cleanup libxml2's parser code.
    ~parser();
  };

  /// Wrapper class for libxml2 relax-ng parser context.
  class rng_parser_context
  {
  public:
    /// Construct an empty, invalid parser context.
    rng_parser_context();
    /// Construct a parser context by parsing a schema in an external file.
    /// @param schema_file the file name of the schema file
    rng_parser_context(char const* schema_file);
    /// Construct a parser context by parsing an in-memory schema.
    /// @param buffer a pointer to the in-memory schema
    /// @param size the number of bytes that @p buffer points to
    rng_parser_context(char const* buffer, int size);
    /// Destroy the parser context and clean up.
    ~rng_parser_context();

    /// Open an external file and parse its schema.
    /// @param schema_file the file name of the schema file
    /// @returns true for success or false if the file cannot be opened or parsed
    bool open(char const* schema_file);
    /// Parse an in-memory schema.
    /// @param buffer a pointer to the in-memory schema
    /// @param size the number of bytes that @p buffer points to
    /// @returns true for success or false if the schema cannot be parsed
    bool initialize(char const* buffer, int size);
    /// Clean up the parser context.
    void close();
    /// Indicates status of the parser context.
    /// @returns 0 if no parser context or it is invalid; non-0 for valid parser context
    operator void*() const { return context_; }
  private:
    friend class rng_schema;
    rng_parser_context(rng_parser_context&); ///< do not implement to avoid problems with copying @c context_
    void operator=(rng_parser_context&);     ///< do not implement to avoid problems with copying @c context_
    _xmlRelaxNGParserCtxt* context_;         ///< the libxml2 parser context
  };

  /// Wrapper class for libxml2 relax-ng schema.
  class rng_schema
  {
  public:
    /// Construct an empty schema
    rng_schema();
    /// Construct a schema from a parser context
    /// @param context a valid parser context
    rng_schema(rng_parser_context& context);
    /// Destroy and cleanup the schema
    ~rng_schema();

    /// Construct a schema from a parser context
    /// @param context a valid parser context
    /// @returns true for success
    bool parse(rng_parser_context& context);
    /// Clean up the schema
    void free();
    /// Indicate whether the schema is valid.
    /// @returns 0 for invalid or empty schema, non-0 for valid schema
    operator void*() const { return schema_; }
  private:
    friend class rng_validation_context;
    rng_schema(rng_schema&);      ///< do not implement so object cannot be copied
    void operator=(rng_schema&);  ///< do not implement so object cannot be assigned
    _xmlRelaxNG* schema_;         ///< the libxml2 schema pointer
  };

  /// Wrapper class for libxml2 relax-ng validation context.
  class rng_validation_context
  {
  public:
    /// Construct an empty validation context
    rng_validation_context();
    /// Construct a validation context from a schema.
    /// @param schema a valid schema
    rng_validation_context(rng_schema& schema);
    /// Destroy the validation context
    ~rng_validation_context();

    /// Initialize the validation context from a schema.
    /// @param schema a valid schema
    /// @returns true for success
    bool initialize(rng_schema& schema);
    /// Cleanup the validation schema.
    void free();

    /// Validate a document.
    /// @param doc an XML document to validate against the schema
    /// @returns true if the document is valid according to the schema; false for invalid
    bool is_valid(doc& doc);

    /// Indicate whether the validation context is ready.
    /// @returns 0 if there is no validation context or it is not ready, non-0 if it is okay
    operator void*() const { return context_; }
  private:
    rng_validation_context(rng_validation_context&); ///< do not implement
    void operator=(rng_validation_context&);         ///< do not implement
    _xmlRelaxNGValidCtxt* context_;                  ///< the libxml2 validation context
  };

  class push_parser_context
  {
  public:
    push_parser_context(xmlSAXHandlerPtr handler, void* data, char const* filename);
    ~push_parser_context();

    int parse(char* buffer, int size);
  private:
    xmlParserCtxtPtr context_;
    char const* const filename_;
    xmlSAXHandlerPtr const handler_;
    void* const data_;
  };

  /// Wrapper class for SAX2.
  /// Declare a subclass that overrides any callbacks that
  /// you are interested in. The subclass can carry any state it needs.
  class sax
  {
  public:
    sax();
    virtual ~sax() {}

    int parse_file(char const* filename);
    int parse_file(std::string const& filename);
    int parse_memory(char const* buffer, std::size_t size);
    int parse_memory(unsigned char const* buffer, std::size_t size);
    int parse_memory(std::string const& buffer);

    /// Any callback can call @c abort_parsing to abort the parse.
    /// The parse function will return @p error.
    /// This function does not return.
    void abort_parsing(int error = 0);

  protected:
      /// Benign exception class to abort parsing early.
    struct sax_abort
    {
      sax_abort(int error) : error_(error) {}
      int const error_;
    };

    // Override any or all of these functions in a derived class.
    // The SAX parser will call the functions as it parses the XML stream.
    // The base class version of each function does nothing.
    virtual void start_element(xmlChar const* name, xmlChar const **attrs);
    virtual void end_element(xmlChar const* name);
    virtual void characters(xmlChar const *ch, int len);
    virtual xmlEntity* get_entity(xmlChar const *name);

  private:
    // Callbacks that are used to fill an xmlSaxHandler structure. Each callback
    // interprets the ctx argument as a sax pointer, casts it, and calls
    // the corresponding virtual function.
    static void sax_start_element(void *ctx, xmlChar const* name, xmlChar const **attrs);
    static void sax_end_element(void *ctx, xmlChar const *name);
    static void sax_characters(void *ctx, xmlChar const *ch, int len);
    static xmlEntity* sax_get_entity(void *ctx, xmlChar const *name);

    xmlSAXHandler callbacks_;
  };

  /// Wrapper class for libxml2 XML document.
  class doc
  {
  public:
    /// Construct an empty document
    doc();
    /// Release the document
    ~doc();

    /// Return the root element node, ignoring comment nodes.
    /// @returns a pointer to the root element node.
    _xmlNode* get_root_element() const;

    /// Close the document and release memory.
    void close();

    /// Parse an in-memory XML document.
    /// @param buffer a pointer to an in-memory XML document. The string is NUL-terminated UTF-8 characters.
    /// @returns true for success or false if the XML cannot be parsed
    bool parse(char const* buffer);
    /// Parse an in-memory XML document.
    /// @param buffer a pointer to an in-memory XML document. The string is NUL-terminated UTF-8 characters.
    /// @returns true for success or false if the XML cannot be parsed
    bool parse(unsigned char const* buffer);
    /// Parse an in-memory XML document.
    /// @param buffer a pointer to an in-memory XML document. The string contains UTF-8 characters.
    /// @returns true for success or false if the XML cannot be parsed
    bool parse(std::string const& buffer);

    /// Parse an XML document in an external file.
    /// @param filename the path to the file that contains the XML document
    /// @returns true for success or false if the file cannot be opened or the XML cannot be parsed
    bool parse_entity(char const* filename);
    /// Parse an XML document in an external file.
    /// @param filename the path to the file that contains the XML document
    /// @returns true for success or false if the file cannot be opened or the XML cannot be parsed
    bool parse_entity(std::string const& filename);

    /// Dump the contents of the document to a string.
    /// @return the entire tree contents as formatted XML
    std::string dump() const;

    /// Delete the old root element and replace it with root.
    /// @param root the new root of the tree
    void replace_root(xmlNode* root);

  private:
    friend class rng_validation_context;
    doc(doc&);                ///< do not implement
    void operator=(doc&);     ///< do not implement
    xmlDoc* doc_;             ///< the libxml2 document pointer
  };

  /** Convert @c xmlChar pointer to @c char pointer.
   * libxml uses unsigned char (xmlChar) throughout the library for utf-8 characters.
   * std::string doesn't care about utf-8, but it uses plain char. Use xml::string
   * to convert an xmlChar* to an std::string or charptr() to convert to plain char*.
   * @param ucharptr pointer to @c xmlChar string
   * @returns the same pointer, but cast to <tt>char*</tt>.
   */
  inline char const* charptr(unsigned char const* ucharptr)
  {
    return reinterpret_cast<char const*>(ucharptr);
  }

  /** Convert @c const @c xmlChar pointer to @c const @c char pointer.
   * libxml uses unsigned char (xmlChar) throughout the library for utf-8 characters.
   * std::string doesn't care about utf-8, but it uses plain char. Use xml::string
   * to convert an xmlChar* to an std::string or charptr() to convert to plain char*.
   * @param ucharptr pointer to @c xmlChar string
   * @returns the same pointer, but cast to <tt>char*</tt>.
   */
  inline char* charptr(unsigned char* ucharptr)
  {
    return reinterpret_cast<char*>(ucharptr);
  }

  /** Convert @c xmlChar pointer to a string.
   * libxml uses unsigned char (xmlChar) throughout the library for utf-8 characters.
   * std::string doesn't care about utf-8, but it uses plain char. Use xml::string
   * to convert an xmlChar* to an std::string or charptr() to convert to plain char*.
   * @param text pointer to @c xmlChar string
   * @returns the character string converted to <tt>std::string</tt>.
   */
  inline std::string string(unsigned char const* text)
  {
    return std::string(charptr(text));
  }

  /** Convert @c xmlChar pointer to a string.
   * libxml uses unsigned char (xmlChar) throughout the library for utf-8 characters.
   * std::string doesn't care about utf-8, but it uses plain char. Use xml::string
   * to convert an xmlChar* to an std::string or charptr() to convert to plain char*.
   * @param text pointer to @c xmlChar string
   * @param size number of bytes in @p text
   * @returns the character string converted to <tt>std::string</tt>.
   */
  inline std::string string(unsigned char const* text, std::size_t size)
  {
    return std::string(charptr(text), size);
  }

  /** Convert @c const @c char pointer to @c const @c xmlChar pointer.
   * libxml uses unsigned char (xmlChar) throughout the library for utf-8 characters.
   * std::string doesn't care about utf-8, but it uses plain char.
   * @param ptr pointer to @c char string
   * @returns the same pointer, but cast to <tt>xmlChar*</tt>.
   */
  inline unsigned char const* ucharptr(char const* ptr)
  {
    return reinterpret_cast<unsigned char const*>(ptr);
  }

  /** Convert @c const @c char pointer to @c const @c xmlChar pointer.
   * libxml uses unsigned char (xmlChar) throughout the library for utf-8 characters.
   * std::string doesn't care about utf-8, but it uses plain char.
   * @param ptr pointer to @c char string
   * @returns the same pointer, but cast to <tt>xmlChar*</tt>.
   */
  inline unsigned char const* ucharptr(std::string const& str)
  {
    return ucharptr(str.c_str());
  }

  /** Compare two strings for equality
   * @param a a string
   * @param b another string
   * @return true if the strings are equal
   */
  inline bool text_is(char const* a, char const* b)
  {
    return std::strcmp(a, b) == 0;
  }

  /** Test a node name.
   * @param node the node to test
   * @param name the node name
   * @return true if the name of @p node is equal to @p name
   */
  bool node_is(xmlNode* node, char const* name);
  /** Test a node name.
   * @param node the node to test
   * @param name the node name
   * @return true if the name of @p node is equal to @p name
   */
  inline bool node_is(xmlNode* node, std::string const& name)
  {
    return node_is(node, name.c_str());
  }
  /** Test an attribute name.
   * @param attr the attribute to test
   * @param name the attribute name
   * @return true if the name of @p attr is equal to @p name
   */
  bool attr_is(xmlAttr* attr, char const* name);
  /** Test an attribute name.
   * @param attr the attribute to test
   * @param name the attribute name
   * @return true if the name of @p attr is equal to @p name
   */
  inline bool attr_is(xmlAttr* attr, std::string const& name)
  {
    return attr_is(attr, name.c_str());
  }

  /// Get text content from a node. Call xmlNodeGetContent to obtain
  /// all the text content of the node's children, and convert the @c xmlChar
  /// string to <tt>std::string</tt>.
  /// @param node the node pointer
  /// @returns the node's text value as a string
  std::string get_content(xmlNode* node);

  /// Get an attribute value.
  /// @param node the node pointer
  /// @param name the name of the attribute
  /// @returns the attribute value as a string
  std::string get_attr_value(xmlNode* node, char const* name);
  /// Get an attribute value.
  /// @param node the node pointer
  /// @param name the name of the attribute
  /// @param ns_prefix the namespace prefix of the attribute
  /// @returns the attribute value as a string
  std::string get_attr_value(xmlNode* node, char const* name, char const* ns_prefix);
  /// Get an attribute value.
  /// @param node the node pointer
  /// @param name the name of the attribute
  /// @returns the attribute value as a string
  std::string get_attr_value(xmlNode* node, std::string const& name);
  /// Get an attribute value.
  /// @param node the node pointer
  /// @param name the name of the attribute
  /// @param ns_prefix the namespace prefix of the attribute
  /// @returns the attribute value as a string
  std::string get_attr_value(xmlNode* node, std::string const& name, std::string const& ns_prefix);
}
#endif
