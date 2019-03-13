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

/// @file xml.cpp
/// Implement the XML wrapper classes.

#include "xml.hpp"
#include <climits>

extern "C"
{
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/relaxng.h>
}

namespace xml
{

  relax_ng::relax_ng()
  {
    xmlRelaxNGInitTypes();
  }

  relax_ng::~relax_ng()
  {
    xmlRelaxNGCleanupTypes();
  }

  parser::parser()
  {
    xmlInitParser();
  }

  parser::~parser()
  {
    xmlCleanupParser();
  }

  doc::doc()
  : doc_(0)
  {}

  doc::~doc()
  {
    close();
  }

  void doc::close()
  {
    if (doc_ != 0)
      xmlFreeDoc(doc_);
  }

  xmlNode* doc::get_root_element()
  const
  {
    return xmlDocGetRootElement(doc_);
  }

  bool doc::parse(char const* buffer)
  {
    return parse(reinterpret_cast<unsigned char const*>(buffer));
  }

  bool doc::parse(std::string const& buffer)
  {
    return parse(buffer.c_str());
  }

  bool doc::parse(unsigned char const* buffer)
  {
    close();
    doc_ = xmlParseDoc(buffer);
    return doc_ != 0;
  }

  bool doc::parse_entity(char const* filename)
  {
    close();
    doc_ = xmlParseEntity(filename);
    return doc_ != 0;
  }
  bool doc::parse_entity(std::string const& filename)
  {
    return parse_entity(filename.c_str());
  }
  std::string doc::dump()
  const
  {
    xmlChar* buffer(0);
    int size(0);
    xmlDocDumpMemory(doc_, &buffer, &size);
    std::string result(charptr(buffer), size);
    xmlFree(buffer);
    return result;
  }

  void doc::replace_root(_xmlNode* root)
  {
    for (xmlNode* node = doc_->children, *prev(0); node != 0; prev = node, node = node->next)
      if (node == get_root_element())
      {
        xmlUnlinkNode(node);
        xmlFreeNode(node);
        if (prev != 0)
          xmlAddSibling(prev, root);
      }
  }

  rng_parser_context::rng_parser_context() : context_(0) {}
  rng_parser_context::rng_parser_context(char const* schema_file) : context_(0)
  {
    open(schema_file);
  }
  rng_parser_context::rng_parser_context(char const* buffer, int size) : context_(0)
  {
    initialize(buffer, size);
  }
  rng_parser_context::~rng_parser_context()
  {
    close();
  }

  bool rng_parser_context::initialize(char const* buffer, int size)
  {
    close();
    context_ = xmlRelaxNGNewMemParserCtxt(buffer, size);
    return context_ != 0;
  }

  bool rng_parser_context::open(char const* schema_file)
  {
    close();
    context_ = xmlRelaxNGNewParserCtxt(schema_file);
    return context_ != 0;
  }

  void rng_parser_context::close()
  {
    if (context_ != 0)
    {
      xmlRelaxNGFreeParserCtxt(context_);
      context_ = 0;
    }
  }

  rng_schema::rng_schema() : schema_(0) {}
  rng_schema::rng_schema(rng_parser_context& context) : schema_(0)
  {
    parse(context);
  }
  rng_schema::~rng_schema()
  {
    free();
  }

  bool rng_schema::parse(rng_parser_context& context)
  {
    free();
    schema_ = xmlRelaxNGParse(context.context_);
    return schema_ != 0;
  }
  void rng_schema::free()
  {
    if (schema_ != 0)
    {
      xmlRelaxNGFree(schema_);
      schema_ = 0;
    }
  }

  rng_validation_context::rng_validation_context() : context_(0) {}
  rng_validation_context::rng_validation_context(rng_schema& schema)
  : context_(0)
  {
    initialize(schema);
  }
  rng_validation_context::~rng_validation_context()
  {
    free();
  }

  bool rng_validation_context::initialize(rng_schema& schema)
  {
    free();
    context_ = xmlRelaxNGNewValidCtxt(schema.schema_);
    return context_ != 0;
  }
  void rng_validation_context::free()
  {
    if (context_ != 0)
    {
      xmlRelaxNGFreeValidCtxt(context_);
      context_ = 0;
    }
  }

  bool rng_validation_context::is_valid(doc& doc)
  {
    return xmlRelaxNGValidateDoc(context_, doc.doc_) == 0;
  }

  bool node_is(_xmlNode* node, char const* name)
  {
    return text_is(charptr(node->name), name);
  }

  bool attr_is(_xmlAttr* attr, char const* name)
  {
    return text_is(charptr(attr->name), name);
  }

  std::string get_content(_xmlNode* node)
  {
    xmlChar* text = xmlNodeGetContent(node);
    try {
      std::string result(string(text));
      xmlFree(text);
      return result;
    } catch(...) {
      xmlFree(text);
      throw;
    }
  }

  std::string get_attr_value(_xmlNode* node, char const* name)
  {
    xmlChar* value = xmlGetProp(node, ucharptr(name));
    if (value == 0)
      return std::string();
    else
      return xml::string(value);
  }
  std::string get_attr_value(_xmlNode* node, char const* name, char const* ns_prefix)
  {
    xmlChar* value = xmlGetNsProp(node, ucharptr(name), ucharptr(ns_prefix));
    if (value == 0)
      return std::string();
    else
      return xml::string(value);
  }
  std::string get_attr_value(_xmlNode* node, std::string const& name)
  {
    return get_attr_value(node, name.c_str());
  }
  std::string get_attr_value(_xmlNode* node, std::string const& name, std::string const& ns_prefix)
  {
    return get_attr_value(node, name.c_str(), ns_prefix.c_str());
  }


  push_parser_context::push_parser_context(xmlSAXHandlerPtr handler, void* data, char const* filename)
  : context_(0), filename_(filename), handler_(handler), data_(data)
  {}
  push_parser_context::~push_parser_context()
  {
    xmlFreeParserCtxt(context_);
  }

  int push_parser_context::parse(char* buffer, int size)
  {
    if (context_ == 0)
    {
      context_ = xmlCreatePushParserCtxt(handler_, data_, buffer, size, filename_);
      assert(context_ != 0);
      return 0;
    }
    else
      return xmlParseChunk(context_, buffer, size, size == 0);
  }


  sax::sax()
  {
    std::memset(static_cast<void*>(&callbacks_), sizeof(callbacks_), 0);
    callbacks_.startElement = sax_start_element;
    callbacks_.endElement = sax_end_element;
    callbacks_.characters = sax_characters;
  }

  int sax::parse_file(char const* filename)
  {
    try
    {
      return xmlSAXUserParseFile(&callbacks_, this, filename);
    }
    catch (sax_abort& sa)
    {
      return sa.error_;
    }
    catch (...)
    {
      throw;
    }
  }
  int sax::parse_file(std::string const& filename)
  {
    return parse_file(filename.c_str());
  }
  int sax::parse_memory(char const* buffer, std::size_t size)
  {
    assert(size <= INT_MAX);
    try
    {
      return xmlSAXUserParseMemory(&callbacks_, this, buffer, static_cast<int>(size));
    }
    catch (sax_abort& sa)
    {
      return sa.error_;
    }
    catch (...)
    {
      throw;
    }
  }
  int sax::parse_memory(unsigned char const* buffer, std::size_t size)
  {
    return parse_memory(reinterpret_cast<char const*>(buffer), size);
  }
  int sax::parse_memory(std::string const& buffer)
  {
    return parse_memory(buffer.data(), buffer.size());
  }
  void sax::abort_parsing(int error)
  {
    throw sax_abort(error);
  }
  // Override any or all of these functions in a derived class.
  // The SAX parser will call the functions as it parses the XML stream.
  // The base class version of most functions does nothing.
  void sax::start_element(xmlChar const* name, xmlChar const **attrs) {}
  void sax::end_element(xmlChar const* name)                          {}
  void sax::characters(xmlChar const *ch, int len)                    {}
  xmlEntityPtr sax::get_entity(xmlChar const *name)
  {
    return xmlGetPredefinedEntity(name);
  }



  // Callbacks that are used to fill an xmlSaxHandler structure. Each callback
  // interprets the ctx argument as a sax pointer, casts it, and calls
  // the corresponding virtual function.
  void sax::sax_start_element(void *ctx, xmlChar const* name, xmlChar const **attrs)
  {
    static_cast<sax*>(ctx)->start_element(name, attrs);
  }
  void sax::sax_end_element(void *ctx, xmlChar const *name)
  {
    static_cast<sax*>(ctx)->end_element(name);
  }
  void sax::sax_characters(void *ctx, xmlChar const *ch, int len)
  {
    static_cast<sax*>(ctx)->characters(ch, len);
  }
  xmlEntityPtr sax::sax_get_entity(void *ctx, xmlChar const *name)
  {
    static_cast<sax*>(ctx)->get_entity(name);
  }



}
