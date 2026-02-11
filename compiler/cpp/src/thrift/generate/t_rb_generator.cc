/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 * Contains some contributions under the Thrift Software License.
 * Please see doc/old-thrift-license.txt in the Thrift distribution for
 * details.
 */

#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sstream>

#include "thrift/platform.h"
#include "thrift/version.h"
#include "thrift/generate/t_oop_generator.h"

using std::map;
using std::string;
using std::vector;

/**
 * A subclass of std::ofstream that includes indenting functionality.
 */
class t_rb_ofstream : public std::ofstream {
private:
  int indent_;

public:
  t_rb_ofstream() : std::ofstream(), indent_(0) {}
  explicit t_rb_ofstream(const char* filename,
                         ios_base::openmode mode = ios_base::out,
                         int indent = 0)
    : std::ofstream(filename, mode), indent_(indent) {}

  t_rb_ofstream& indent() {
    for (int i = 0; i < indent_; ++i) {
      *this << "  ";
    }
    return *this;
  }

  void indent_up() { indent_++; }
  void indent_down() { indent_--; }
};

/**
 * Ruby code generator.
 *
 */
class t_rb_generator : public t_oop_generator {
public:
  t_rb_generator(t_program* program,
                 const std::map<std::string, std::string>& parsed_options,
                 const std::string& /* option_string */)
    : t_oop_generator(program) {
    std::map<std::string, std::string>::const_iterator iter;

    require_rubygems_ = false;
    namespaced_ = false;
    zeitwerk_ = false;

    for( iter = parsed_options.begin(); iter != parsed_options.end(); ++iter) {
      if( iter->first.compare("rubygems") == 0) {
        require_rubygems_ = true;
      } else if( iter->first.compare("namespaced") == 0) {
        namespaced_ = true;
      } else if (iter->first.compare("zeitwerk") == 0) {
        zeitwerk_ = true;
      } else {
        throw "unknown option ruby:" + iter->first;
      }
    }

    if (zeitwerk_ && namespaced_) {
      throw "ruby options zeitwerk and namespaced are mutually exclusive";
    }

    out_dir_base_ = "gen-rb";
  }

  /**
   * Init and close methods
   */

  void init_generator() override;
  void close_generator() override;
  std::string display_name() const override;

  /**
   * Program-level generation functions
   */

  void generate_typedef(t_typedef* ttypedef) override;
  void generate_enum(t_enum* tenum) override;
  void generate_const(t_const* tconst) override;
  void generate_struct(t_struct* tstruct) override;
  void generate_forward_declaration(t_struct* tstruct) override;
  void generate_union(t_struct* tunion);
  void generate_xception(t_struct* txception) override;
  void generate_service(t_service* tservice) override;

  t_rb_ofstream& render_const_value(t_rb_ofstream& out, t_type* type, t_const_value* value);

  /**
   * Struct generation code
   */

  void generate_rb_struct_declaration(t_rb_ofstream& out, t_struct* tstruct, bool is_exception);
  void generate_rb_struct(t_rb_ofstream& out, t_struct* tstruct, bool is_exception);
  void generate_rb_struct_required_validator(t_rb_ofstream& out, t_struct* tstruct);
  void generate_rb_union(t_rb_ofstream& out, t_struct* tstruct, bool is_exception);
  void generate_rb_union_validator(t_rb_ofstream& out, t_struct* tstruct);
  void generate_rb_function_helpers(t_rb_ofstream& out, t_function* tfunction);
  void generate_rb_simple_constructor(t_rb_ofstream& out, t_struct* tstruct);
  void generate_rb_simple_exception_constructor(t_rb_ofstream& out, t_struct* tstruct);
  void generate_field_constants(t_rb_ofstream& out, t_struct* tstruct);
  void generate_field_constructors(t_rb_ofstream& out, t_struct* tstruct);
  void generate_field_defns(t_rb_ofstream& out, t_struct* tstruct);
  void generate_rb_enum_values(t_rb_ofstream& out, t_enum* tenum);
  void generate_field_data(t_rb_ofstream& out,
                           t_type* field_type,
                           const std::string& field_name,
                           t_const_value* field_value,
                           bool optional);

  /**
   * Service-level generation functions
   */

  void generate_service_helpers(t_rb_ofstream& out, t_service* tservice);
  void generate_service_interface(t_service* tservice);
  void generate_service_client(t_rb_ofstream& out, t_service* tservice);
  void generate_service_server(t_rb_ofstream& out, t_service* tservice);
  void generate_process_function(t_rb_ofstream& out, t_function* tfunction);

  /**
   * Serialization constructs
   */

  void generate_deserialize_field(t_rb_ofstream& out,
                                  t_field* tfield,
                                  std::string prefix = "",
                                  bool inclass = false);

  void generate_deserialize_struct(t_rb_ofstream& out, t_struct* tstruct, std::string prefix = "");

  void generate_deserialize_container(t_rb_ofstream& out, t_type* ttype, std::string prefix = "");

  void generate_deserialize_set_element(t_rb_ofstream& out, t_set* tset, std::string prefix = "");

  void generate_deserialize_map_element(t_rb_ofstream& out, t_map* tmap, std::string prefix = "");

  void generate_deserialize_list_element(t_rb_ofstream& out,
                                         t_list* tlist,
                                         std::string prefix = "");

  void generate_serialize_field(t_rb_ofstream& out, t_field* tfield, std::string prefix = "");

  void generate_serialize_struct(t_rb_ofstream& out, t_struct* tstruct, std::string prefix = "");

  void generate_serialize_container(t_rb_ofstream& out, t_type* ttype, std::string prefix = "");

  void generate_serialize_map_element(t_rb_ofstream& out,
                                      t_map* tmap,
                                      std::string kiter,
                                      std::string viter);

  void generate_serialize_set_element(t_rb_ofstream& out, t_set* tmap, std::string iter);

  void generate_serialize_list_element(t_rb_ofstream& out, t_list* tlist, std::string iter);

  void generate_rdoc(t_rb_ofstream& out, t_doc* tdoc);

  /**
   * Helper rendering functions
   */

  std::string rb_autogen_comment();
  std::string render_require_thrift();
  std::string render_includes();
  std::string declare_field(t_field* tfield);
  std::string type_name(const t_type* ttype);
  std::string full_type_name(const t_type* ttype);
  std::string function_signature(t_function* tfunction, std::string prefix = "");
  std::string argument_list(t_struct* tstruct);
  std::string type_to_enum(t_type* ttype);
  const std::string& rb_namespace_constant_prefix(const t_program* p);
  std::string rb_constant_name(const std::string& name);
  std::string rb_file_name(const std::string& name);
  const std::string& rb_namespace_path(const t_program* p);
  std::string rb_symbol_path(const t_program* p, const std::string& constant_name);
  std::string normalize_zeitwerk_name(const std::string& in);
  void open_zeitwerk_service_unit(const t_service* service,
                                  const std::string& idl_name,
                                  const std::string& ruby_name,
                                  const std::string& relative_path,
                                  t_rb_ofstream& out);
  void close_zeitwerk_service_unit(const t_service* service, t_rb_ofstream& out);
  void open_zeitwerk_unit(const t_program* program,
                          const std::string& idl_name,
                          const std::string& ruby_name,
                          const std::string& relative_path,
                          t_rb_ofstream& out);
  void close_zeitwerk_unit(const t_program* program, t_rb_ofstream& out);

  const std::vector<std::string>& ruby_modules(const t_program* p);
  void begin_namespace(t_rb_ofstream&, const std::vector<std::string>&);
  void end_namespace(t_rb_ofstream&, const std::vector<std::string>&);

private:
  /**
   * File streams
   */

  t_rb_ofstream f_types_;
  t_rb_ofstream f_consts_;

  std::string namespace_dir_;
  std::string require_prefix_;

  /** If true, add a "require 'rubygems'" line to the top of each gen-rb file. */
  bool require_rubygems_;

  /** If true, generate files in idiomatic namespaced directories. */
  bool namespaced_;

  /** If true, generate strict Zeitwerk-compatible files. */
  bool zeitwerk_;

  struct rb_namespace_cache {
    std::vector<std::string> modules;
    std::string path_prefix;
    std::string constant_prefix;
  };

  const rb_namespace_cache& get_rb_namespace_cache(const t_program* p);
  std::map<const t_program*, rb_namespace_cache> rb_namespace_cache_;

  std::map<std::string, std::string> zeitwerk_symbols_;
  std::map<std::string, std::string> zeitwerk_paths_;
};

/**
 * Prepares for file generation by opening up the necessary file output
 * streams.
 *
 * @param tprogram The program to generate
 */
void t_rb_generator::init_generator() {
  string subdir = get_out_dir();

  // Make output directory
  MKDIR(subdir.c_str());

  if (zeitwerk_) {
    namespace_dir_ = subdir;
    require_prefix_.clear();
    return;
  }

  if (namespaced_) {
    require_prefix_ = rb_namespace_path(program_);

    string dir = require_prefix_;
    string::size_type loc;

    while ((loc = dir.find("/")) != string::npos) {
      subdir = subdir + dir.substr(0, loc) + "/";
      MKDIR(subdir.c_str());
      dir = dir.substr(loc + 1);
    }
  }

  namespace_dir_ = subdir;

  // Make output file
  string f_types_name = namespace_dir_ + underscore(program_name_) + "_types.rb";
  f_types_.open(f_types_name.c_str());

  string f_consts_name = namespace_dir_ + underscore(program_name_) + "_constants.rb";
  f_consts_.open(f_consts_name.c_str());

  // Print header
  f_types_ << rb_autogen_comment() << '\n' << render_require_thrift() << render_includes() << '\n';
  const auto& program_modules = ruby_modules(program_);
  begin_namespace(f_types_, program_modules);

  f_consts_ << rb_autogen_comment() << '\n' << render_require_thrift() << "require '"
            << require_prefix_ << underscore(program_name_) << "_types'" << '\n' << '\n';
  begin_namespace(f_consts_, program_modules);
}

/**
 * Renders the require of thrift itself, and possibly of the rubygems dependency.
 */
string t_rb_generator::render_require_thrift() {
  if (require_rubygems_) {
    return "require 'rubygems'\nrequire 'thrift'\n";
  } else {
    return "require 'thrift'\n";
  }
}

/**
 * Renders all the imports necessary for including another Thrift program
 */
string t_rb_generator::render_includes() {
  const vector<t_program*>& includes = program_->get_includes();
  string result = "";
  for (auto include : includes) {
    if (namespaced_) {
      t_program* included = include;
      std::string included_require_prefix = rb_namespace_path(included);
      std::string included_name = included->get_name();
      result += "require '" + included_require_prefix + underscore(included_name) + "_types'\n";
    } else {
      result += "require '" + underscore(include->get_name()) + "_types'\n";
    }
  }
  if (includes.size() > 0) {
    result += "\n";
  }
  return result;
}

/**
 * Autogen'd comment
 */
string t_rb_generator::rb_autogen_comment() {
  return std::string("#\n") + "# Autogenerated by Thrift Compiler (" + THRIFT_VERSION + ")\n"
         + "#\n" + "# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING\n" + "#\n";
}

/**
 * Closes the type files
 */
void t_rb_generator::close_generator() {
  if (zeitwerk_) {
    return;
  }

  // Close types file
  const auto& program_modules = ruby_modules(program_);
  end_namespace(f_types_, program_modules);
  end_namespace(f_consts_, program_modules);
  f_types_.close();
  f_consts_.close();
}

string t_rb_generator::normalize_zeitwerk_name(const std::string& in) {
  string out;
  bool upper_next = true;
  for (char ch : in) {
    if (ch == '_') {
      upper_next = true;
      continue;
    }
    if (upper_next) {
      out.push_back((char)toupper(ch));
      upper_next = false;
    } else {
      out.push_back(ch);
    }
  }
  if (out.empty()) {
    throw "cannot normalize empty ruby identifier";
  }
  return out;
}

std::string t_rb_generator::rb_constant_name(const std::string& name) {
  if (zeitwerk_) {
    return normalize_zeitwerk_name(name);
  }
  return capitalize(name);
}

std::string t_rb_generator::rb_file_name(const std::string& name) {
  return underscore(name) + ".rb";
}

const t_rb_generator::rb_namespace_cache&
t_rb_generator::get_rb_namespace_cache(const t_program* p) {
  auto iter = rb_namespace_cache_.find(p);
  if (iter != rb_namespace_cache_.end()) {
    return iter->second;
  }

  rb_namespace_cache cached;
  std::string ns = p->get_namespace("rb");
  if (!ns.empty()) {
    std::string::iterator pos = ns.begin();
    while (true) {
      std::string::iterator delim = std::find(pos, ns.end(), '.');
      string module = rb_constant_name(std::string(pos, delim));
      cached.modules.push_back(module);
      cached.path_prefix += underscore(module) + "/";
      cached.constant_prefix += module + "::";
      pos = delim;
      if (pos == ns.end()) {
        break;
      }
      ++pos;
    }
  }

  auto inserted = rb_namespace_cache_.insert(std::make_pair(p, cached));
  return inserted.first->second;
}

const std::vector<std::string>& t_rb_generator::ruby_modules(const t_program* p) {
  return get_rb_namespace_cache(p).modules;
}

const std::string& t_rb_generator::rb_namespace_path(const t_program* p) {
  return get_rb_namespace_cache(p).path_prefix;
}

const std::string& t_rb_generator::rb_namespace_constant_prefix(const t_program* p) {
  return get_rb_namespace_cache(p).constant_prefix;
}

std::string t_rb_generator::rb_symbol_path(const t_program* p, const std::string& constant_name) {
  return rb_namespace_path(p) + rb_file_name(constant_name);
}

void t_rb_generator::open_zeitwerk_service_unit(const t_service* service,
                                                const std::string& idl_name,
                                                const std::string& ruby_name,
                                                const std::string& relative_path,
                                                t_rb_ofstream& out) {
  open_zeitwerk_unit(service->get_program(), idl_name, ruby_name, relative_path, out);
  out.indent() << "module " << rb_constant_name(service->get_name()) << '\n';
  out.indent_up();
}

void t_rb_generator::close_zeitwerk_service_unit(const t_service* service, t_rb_ofstream& out) {
  out.indent_down();
  out.indent() << "end" << '\n' << '\n';
  close_zeitwerk_unit(service->get_program(), out);
}

void t_rb_generator::open_zeitwerk_unit(const t_program* program,
                                        const std::string& idl_name,
                                        const std::string& ruby_name,
                                        const std::string& relative_path,
                                        t_rb_ofstream& out) {
  auto symbol_iter = zeitwerk_symbols_.find(ruby_name);
  if (symbol_iter != zeitwerk_symbols_.end() && symbol_iter->second != idl_name) {
    throw "zeitwerk symbol collision for " + ruby_name + ": " + symbol_iter->second + " vs " + idl_name;
  }
  auto path_iter = zeitwerk_paths_.find(relative_path);
  if (path_iter != zeitwerk_paths_.end() && path_iter->second != ruby_name) {
    throw "zeitwerk path collision for " + relative_path + ": " + path_iter->second + " vs " + ruby_name;
  }
  zeitwerk_symbols_[ruby_name] = idl_name;
  zeitwerk_paths_[relative_path] = ruby_name;

  string::size_type slash = relative_path.find('/');
  while (slash != string::npos) {
    MKDIR((namespace_dir_ + relative_path.substr(0, slash + 1)).c_str());
    slash = relative_path.find('/', slash + 1);
  }
  out.open((namespace_dir_ + relative_path).c_str());
  out << rb_autogen_comment() << '\n' << render_require_thrift() << '\n';
  begin_namespace(out, ruby_modules(program));
}

void t_rb_generator::close_zeitwerk_unit(const t_program* program, t_rb_ofstream& out) {
  end_namespace(out, ruby_modules(program));
  out.close();
}

/**
 * Generates a typedef. This is not done in Ruby, types are all implicit.
 *
 * @param ttypedef The type definition
 */
void t_rb_generator::generate_typedef(t_typedef* ttypedef) {
  (void)ttypedef;
}

void t_rb_generator::generate_rb_enum_values(t_rb_ofstream& out, t_enum* tenum) {
  vector<t_enum_value*> constants = tenum->get_constants();
  for (auto* constant : constants) {
    int value = constant->get_value();
    string name = capitalize(constant->get_name());

    generate_rdoc(out, constant);
    out.indent() << name << " = " << value << '\n';
  }

  out.indent() << "VALUE_MAP = {";
  bool first = true;
  for (auto* constant : constants) {
    int value = constant->get_value();
    if (!first) {
      out << ", ";
    }
    first = false;
    out << value << " => \"" << capitalize(constant->get_name()) << "\"";
  }
  out << "}" << '\n';

  out.indent() << "VALID_VALUES = Set.new([";
  first = true;
  for (auto* constant : constants) {
    if (!first) {
      out << ", ";
    }
    first = false;
    out << capitalize(constant->get_name());
  }
  out << "]).freeze" << '\n';
}

/**
 * Generates code for an enumerated type. Done using a class to scope
 * the values.
 *
 * @param tenum The enumeration
 */
void t_rb_generator::generate_enum(t_enum* tenum) {
  if (zeitwerk_) {
    string enum_name = type_name(tenum);
    t_rb_ofstream out;
    open_zeitwerk_unit(program_,
                       tenum->get_name(),
                       rb_namespace_constant_prefix(program_) + enum_name,
                       rb_symbol_path(program_, enum_name),
                       out);
    out.indent() << "module " << enum_name << '\n';
    out.indent_up();
    generate_rb_enum_values(out, tenum);
    out.indent_down();
    out.indent() << "end" << '\n' << '\n';
    close_zeitwerk_unit(program_, out);
    return;
  }

  f_types_.indent() << "module " << capitalize(tenum->get_name()) << '\n';
  f_types_.indent_up();
  generate_rb_enum_values(f_types_, tenum);
  f_types_.indent_down();
  f_types_.indent() << "end" << '\n' << '\n';
}

/**
 * Generate a constant value
 */
void t_rb_generator::generate_const(t_const* tconst) {
  t_type* type = tconst->get_type();
  string name = rb_constant_name(tconst->get_name());
  t_const_value* value = tconst->get_value();

  if (zeitwerk_) {
    t_rb_ofstream out;
    open_zeitwerk_unit(program_,
                       tconst->get_name(),
                       rb_namespace_constant_prefix(program_) + name,
                       rb_symbol_path(program_, name),
                       out);
    out.indent() << name << " = ";
    render_const_value(out, type, value) << '\n' << '\n';
    close_zeitwerk_unit(program_, out);
    return;
  }

  f_consts_.indent() << name << " = ";
  render_const_value(f_consts_, type, value) << '\n' << '\n';
}

/**
 * Prints the value of a constant with the given type. Note that type checking
 * is NOT performed in this function as it is always run beforehand using the
 * validate_types method in main.cc
 */
t_rb_ofstream& t_rb_generator::render_const_value(t_rb_ofstream& out,
                                                  t_type* type,
                                                  t_const_value* value) {
  type = get_true_type(type);
  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_STRING:
      out << "%q\"" << get_escaped_string(value) << '"';
      break;
    case t_base_type::TYPE_UUID:
      out << "%q\"" << get_escaped_string(value) << '"';
      break;
    case t_base_type::TYPE_BOOL:
      out << (value->get_integer() > 0 ? "true" : "false");
      break;
    case t_base_type::TYPE_I8:
    case t_base_type::TYPE_I16:
    case t_base_type::TYPE_I32:
    case t_base_type::TYPE_I64:
      out << value->get_integer();
      break;
    case t_base_type::TYPE_DOUBLE:
      if (value->get_type() == t_const_value::CV_INTEGER) {
        out << value->get_integer();
      } else {
        out << value->get_double();
      }
      break;
    default:
      throw "compiler error: no const of base type " + t_base_type::t_base_name(tbase);
    }
  } else if (type->is_enum()) {
    out.indent() << value->get_integer();
  } else if (type->is_struct() || type->is_xception()) {
    out << full_type_name(type) << ".new({" << '\n';
    out.indent_up();
    const vector<t_field*>& fields = ((t_struct*)type)->get_members();
    vector<t_field*>::const_iterator f_iter;
    const map<t_const_value*, t_const_value*, t_const_value::value_compare>& val = value->get_map();
    map<t_const_value*, t_const_value*, t_const_value::value_compare>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      t_type* field_type = nullptr;
      for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
        if ((*f_iter)->get_name() == v_iter->first->get_string()) {
          field_type = (*f_iter)->get_type();
        }
      }
      if (field_type == nullptr) {
        throw "type error: " + type->get_name() + " has no field " + v_iter->first->get_string();
      }
      out.indent();
      render_const_value(out, g_type_string, v_iter->first) << " => ";
      render_const_value(out, field_type, v_iter->second) << "," << '\n';
    }
    out.indent_down();
    out.indent() << "})";
  } else if (type->is_map()) {
    t_type* ktype = ((t_map*)type)->get_key_type();
    t_type* vtype = ((t_map*)type)->get_val_type();
    out << "{" << '\n';
    out.indent_up();
    const map<t_const_value*, t_const_value*, t_const_value::value_compare>& val = value->get_map();
    map<t_const_value*, t_const_value*, t_const_value::value_compare>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      out.indent();
      render_const_value(out, ktype, v_iter->first) << " => ";
      render_const_value(out, vtype, v_iter->second) << "," << '\n';
    }
    out.indent_down();
    out.indent() << "}";
  } else if (type->is_list() || type->is_set()) {
    t_type* etype;
    if (type->is_list()) {
      etype = ((t_list*)type)->get_elem_type();
    } else {
      etype = ((t_set*)type)->get_elem_type();
    }
    if (type->is_set()) {
      out << "Set.new([" << '\n';
    } else {
      out << "[" << '\n';
    }
    out.indent_up();
    const vector<t_const_value*>& val = value->get_list();
    vector<t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      out.indent();
      render_const_value(out, etype, *v_iter) << "," << '\n';
    }
    out.indent_down();
    if (type->is_set()) {
      out.indent() << "])";
    } else {
      out.indent() << "]";
    }
  } else {
    throw "CANNOT GENERATE CONSTANT FOR TYPE: " + type->get_name();
  }
  return out;
}

/**
 * Generates a ruby struct
 */
void t_rb_generator::generate_struct(t_struct* tstruct) {
  if (zeitwerk_) {
    string struct_name = type_name(tstruct);
    t_rb_ofstream out;
    open_zeitwerk_unit(program_,
                       tstruct->get_name(),
                       rb_namespace_constant_prefix(program_) + struct_name,
                       rb_symbol_path(program_, struct_name),
                       out);
    if (tstruct->is_union()) {
      generate_rb_union(out, tstruct, false);
    } else {
      generate_rb_struct(out, tstruct, false);
    }
    close_zeitwerk_unit(program_, out);
    return;
  }

  if (tstruct->is_union()) {
    generate_rb_union(f_types_, tstruct, false);
  } else {
    generate_rb_struct(f_types_, tstruct, false);
  }
}


/**
 * Generates the "forward declarations" for ruby structs.
 * These are simply a declaration of each class with proper inheritance.
 * The rest of the struct is still generated in generate_struct as has
 * always been the case. These declarations allow thrift to generate valid
 * ruby in cases where thrift structs rely on recursive definitions.
 */
void t_rb_generator::generate_forward_declaration(t_struct* tstruct) {
  if (zeitwerk_) {
    (void)tstruct;
    return;
  }
  generate_rb_struct_declaration(f_types_, tstruct, tstruct->is_xception());
}

void t_rb_generator::generate_rb_struct_declaration(t_rb_ofstream& out, t_struct* tstruct, bool is_exception) {
  out.indent() << "class " << type_name(tstruct);
  if (tstruct->is_union()) {
    out << " < ::Thrift::Union";
  }
  if (is_exception) {
    out << " < ::Thrift::Exception";
  }
  out << "; end" << '\n' << '\n';
}

/**
 * Generates a struct definition for a thrift exception. Basically the same
 * as a struct but extends the Exception class.
 *
 * @param txception The struct definition
 */
void t_rb_generator::generate_xception(t_struct* txception) {
  if (zeitwerk_) {
    string xception_name = type_name(txception);
    t_rb_ofstream out;
    open_zeitwerk_unit(program_,
                       txception->get_name(),
                       rb_namespace_constant_prefix(program_) + xception_name,
                       rb_symbol_path(program_, xception_name),
                       out);
    generate_rb_struct(out, txception, true);
    close_zeitwerk_unit(program_, out);
    return;
  }
  generate_rb_struct(f_types_, txception, true);
}

/**
 * Generates a ruby struct
 */
void t_rb_generator::generate_rb_struct(t_rb_ofstream& out,
                                        t_struct* tstruct,
                                        bool is_exception = false) {
  generate_rdoc(out, tstruct);
  out.indent() << "class " << type_name(tstruct);
  if (is_exception) {
    out << " < ::Thrift::Exception";
  }
  out << '\n';

  out.indent_up();
  out.indent() << "include ::Thrift::Struct, ::Thrift::Struct_Union" << '\n';

  if (is_exception) {
    generate_rb_simple_exception_constructor(out, tstruct);
  }

  generate_field_constants(out, tstruct);
  generate_field_defns(out, tstruct);
  generate_rb_struct_required_validator(out, tstruct);

  out.indent() << "::Thrift::Struct.generate_accessors self" << '\n';

  out.indent_down();
  out.indent() << "end" << '\n' << '\n';
}

/**
 * Generates a ruby union
 */
void t_rb_generator::generate_rb_union(t_rb_ofstream& out,
                                       t_struct* tstruct,
                                       bool is_exception = false) {
  (void)is_exception;
  generate_rdoc(out, tstruct);
  out.indent() << "class " << type_name(tstruct) << " < ::Thrift::Union" << '\n';

  out.indent_up();
  out.indent() << "include ::Thrift::Struct_Union" << '\n';

  generate_field_constructors(out, tstruct);

  generate_field_constants(out, tstruct);
  generate_field_defns(out, tstruct);
  generate_rb_union_validator(out, tstruct);

  out.indent() << "::Thrift::Union.generate_accessors self" << '\n';

  out.indent_down();
  out.indent() << "end" << '\n' << '\n';
}

void t_rb_generator::generate_field_constructors(t_rb_ofstream& out, t_struct* tstruct) {

  out.indent() << "class << self" << '\n';
  out.indent_up();

  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;

  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if (f_iter != fields.begin()) {
      out << '\n';
    }
    std::string field_name = (*f_iter)->get_name();

    out.indent() << "def " << field_name << "(val)" << '\n';
    out.indent() << "  " << tstruct->get_name() << ".new(:" << field_name << ", val)" << '\n';
    out.indent() << "end" << '\n';
  }

  out.indent_down();
  out.indent() << "end" << '\n';

  out << '\n';
}

void t_rb_generator::generate_rb_simple_exception_constructor(t_rb_ofstream& out,
                                                              t_struct* tstruct) {
  const vector<t_field*>& members = tstruct->get_members();

  if (members.size() == 1) {
    vector<t_field*>::const_iterator m_iter = members.begin();

    if ((*m_iter)->get_type()->is_string()) {
      string name = (*m_iter)->get_name();

      out.indent() << "def initialize(message=nil)" << '\n';
      out.indent_up();
      out.indent() << "super()" << '\n';
      out.indent() << "self." << name << " = message" << '\n';
      out.indent_down();
      out.indent() << "end" << '\n' << '\n';

      if (name != "message") {
        out.indent() << "def message; " << name << " end" << '\n' << '\n';
      }
    }
  }
}

void t_rb_generator::generate_field_constants(t_rb_ofstream& out, t_struct* tstruct) {
  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;

  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    std::string field_name = (*f_iter)->get_name();
    std::string cap_field_name = upcase_string(field_name);

    out.indent() << cap_field_name << " = " << (*f_iter)->get_key() << '\n';
  }
  out << '\n';
}

void t_rb_generator::generate_field_defns(t_rb_ofstream& out, t_struct* tstruct) {
  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;

  out.indent() << "FIELDS = {" << '\n';
  out.indent_up();
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if (f_iter != fields.begin()) {
      out << "," << '\n';
    }

    // generate the field docstrings within the FIELDS constant. no real better place...
    generate_rdoc(out, *f_iter);

    out.indent() << upcase_string((*f_iter)->get_name()) << " => ";

    generate_field_data(out,
                        (*f_iter)->get_type(),
                        (*f_iter)->get_name(),
                        (*f_iter)->get_value(),
                        (*f_iter)->get_req() == t_field::T_OPTIONAL);
  }
  out.indent_down();
  out << '\n';
  out.indent() << "}" << '\n' << '\n';

  out.indent() << "def struct_fields; FIELDS; end" << '\n' << '\n';
}

void t_rb_generator::generate_field_data(t_rb_ofstream& out,
                                         t_type* field_type,
                                         const std::string& field_name = "",
                                         t_const_value* field_value = nullptr,
                                         bool optional = false) {
  field_type = get_true_type(field_type);

  // Begin this field's defn
  out << "{:type => " << type_to_enum(field_type);

  if (!field_name.empty()) {
    out << ", :name => '" << field_name << "'";
  }

  if (field_value != nullptr) {
    out << ", :default => ";
    render_const_value(out, field_type, field_value);
  }

  if (!field_type->is_base_type()) {
    if (field_type->is_struct() || field_type->is_xception()) {
      if (zeitwerk_) {
        out << ", :class_name => '" << full_type_name((t_struct*)field_type) << "'";
      } else {
        out << ", :class => " << full_type_name((t_struct*)field_type);
      }
    } else if (field_type->is_list()) {
      out << ", :element => ";
      generate_field_data(out, ((t_list*)field_type)->get_elem_type());
    } else if (field_type->is_map()) {
      out << ", :key => ";
      generate_field_data(out, ((t_map*)field_type)->get_key_type());
      out << ", :value => ";
      generate_field_data(out, ((t_map*)field_type)->get_val_type());
    } else if (field_type->is_set()) {
      out << ", :element => ";
      generate_field_data(out, ((t_set*)field_type)->get_elem_type());
    }
  } else {
    if (((t_base_type*)field_type)->is_binary()) {
      out << ", :binary => true";
    }
  }

  if (optional) {
    out << ", :optional => true";
  }

  if (field_type->is_enum()) {
    if (zeitwerk_) {
      out << ", :enum_name => '" << full_type_name(field_type) << "'";
    } else {
      out << ", :enum_class => " << full_type_name(field_type);
    }
  }

  // End of this field's defn
  out << "}";
}

void t_rb_generator::begin_namespace(t_rb_ofstream& out, const vector<std::string>& modules) {
  for (const auto& module : modules) {
    out.indent() << "module " << module << '\n';
    out.indent_up();
  }
}

void t_rb_generator::end_namespace(t_rb_ofstream& out, const vector<std::string>& modules) {
  for (vector<std::string>::const_reverse_iterator m_iter = modules.rbegin();
       m_iter != modules.rend();
       ++m_iter) {
    out.indent_down();
    out.indent() << "end" << '\n';
  }
}

/**
 * Generates a thrift service.
 *
 * @param tservice The service definition
 */
void t_rb_generator::generate_service(t_service* tservice) {
  if (zeitwerk_) {
    string service_name = rb_constant_name(tservice->get_name());
    const t_program* service_program = tservice->get_program();
    string ruby_service_name = rb_namespace_constant_prefix(service_program) + service_name;
    string service_base_path = rb_namespace_path(service_program) + underscore(service_name) + "/";

    {
      t_rb_ofstream out;
      open_zeitwerk_service_unit(tservice,
                                 tservice->get_name(),
                                 ruby_service_name,
                                 rb_symbol_path(service_program, service_name),
                                 out);
      close_zeitwerk_service_unit(tservice, out);
    }

    {
      t_rb_ofstream out;
      open_zeitwerk_service_unit(tservice,
                                 tservice->get_name() + ".Client",
                                 ruby_service_name + "::Client",
                                 service_base_path + "client.rb",
                                 out);
      generate_service_client(out, tservice);
      close_zeitwerk_service_unit(tservice, out);
    }

    {
      t_rb_ofstream out;
      open_zeitwerk_service_unit(tservice,
                                 tservice->get_name() + ".Processor",
                                 ruby_service_name + "::Processor",
                                 service_base_path + "processor.rb",
                                 out);
      generate_service_server(out, tservice);
      close_zeitwerk_service_unit(tservice, out);
    }

    const auto& functions = tservice->get_functions();
    for (auto* function : functions) {
      t_struct* args = function->get_arglist();
      string args_name = rb_constant_name(function->get_name() + "_args");
      {
        t_rb_ofstream out;
        open_zeitwerk_service_unit(tservice,
                                   function->get_name() + "_args",
                                   ruby_service_name + "::" + args_name,
                                   service_base_path + rb_file_name(args_name),
                                   out);
        generate_rb_struct(out, args, false);
        close_zeitwerk_service_unit(tservice, out);
      }

      string result_name = rb_constant_name(function->get_name() + "_result");
      {
        t_rb_ofstream out;
        open_zeitwerk_service_unit(tservice,
                                   function->get_name() + "_result",
                                   ruby_service_name + "::" + result_name,
                                   service_base_path + rb_file_name(result_name),
                                   out);
        generate_rb_function_helpers(out, function);
        close_zeitwerk_service_unit(tservice, out);
      }
    }
    return;
  }

  string f_service_name = namespace_dir_ + underscore(service_name_) + ".rb";
  t_rb_ofstream service_out;
  service_out.open(f_service_name.c_str());

  service_out << rb_autogen_comment() << '\n' << render_require_thrift();

  if (tservice->get_extends() != nullptr) {
    if (namespaced_) {
      service_out << "require '" << rb_namespace_path(tservice->get_extends()->get_program())
                  << underscore(tservice->get_extends()->get_name()) << "'" << '\n';
    } else {
      service_out << "require '" << require_prefix_
                  << underscore(tservice->get_extends()->get_name()) << "'" << '\n';
    }
  }

  service_out << "require '" << require_prefix_ << underscore(program_name_) << "_types'" << '\n'
              << '\n';

  const auto& service_modules = ruby_modules(tservice->get_program());
  begin_namespace(service_out, service_modules);

  service_out.indent() << "module " << rb_constant_name(tservice->get_name()) << '\n';
  service_out.indent_up();

  // Generate the three main parts of the service (well, two for now in PHP)
  generate_service_client(service_out, tservice);
  generate_service_server(service_out, tservice);
  generate_service_helpers(service_out, tservice);

  service_out.indent_down();
  service_out.indent() << "end" << '\n' << '\n';

  end_namespace(service_out, service_modules);

  // Close service file
  service_out.close();
}

/**
 * Generates helper functions for a service.
 *
 * @param tservice The service to generate a header definition for
 */
void t_rb_generator::generate_service_helpers(t_rb_ofstream& out, t_service* tservice) {
  const auto& functions = tservice->get_functions();

  out.indent() << "# HELPER FUNCTIONS AND STRUCTURES" << '\n' << '\n';

  for (auto* function : functions) {
    t_struct* ts = function->get_arglist();
    generate_rb_struct(out, ts, false);
    generate_rb_function_helpers(out, function);
  }
}

/**
 * Generates a struct and helpers for a function.
 *
 * @param tfunction The function
 */
void t_rb_generator::generate_rb_function_helpers(t_rb_ofstream& out, t_function* tfunction) {
  t_struct result(program_, tfunction->get_name() + "_result");
  t_field success(tfunction->get_returntype(), "success", 0);
  if (!tfunction->get_returntype()->is_void()) {
    result.append(&success);
  }

  t_struct* xs = tfunction->get_xceptions();
  const vector<t_field*>& fields = xs->get_members();
  for (auto* field : fields) {
    result.append(field);
  }
  generate_rb_struct(out, &result, false);
}

/**
 * Generates a service client definition.
 *
 * @param tservice The service to generate a server for.
 */
void t_rb_generator::generate_service_client(t_rb_ofstream& out, t_service* tservice) {
  string extends_client = "";
  if (tservice->get_extends() != nullptr) {
    extends_client = " < " + full_type_name(tservice->get_extends()) + "::Client ";
  }

  out.indent() << "class Client" << extends_client << '\n';
  out.indent_up();

  out.indent() << "include ::Thrift::Client" << '\n' << '\n';

  // Generate client method implementations
  const auto& functions = tservice->get_functions();
  for (auto* function : functions) {
    t_struct* arg_struct = function->get_arglist();
    const vector<t_field*>& fields = arg_struct->get_members();
    string funname = function->get_name();

    // Open function
    out.indent() << "def " << function_signature(function) << '\n';
    out.indent_up();
    out.indent() << "send_" << funname << "(";

    bool first = true;
    for (const auto* field : fields) {
      if (first) {
        first = false;
      } else {
        out << ", ";
      }
      out << field->get_name();
    }
    out << ")" << '\n';

    if (!function->is_oneway()) {
      out.indent();
      if (!function->get_returntype()->is_void()) {
        out << "return ";
      }
      out << "recv_" << funname << "()" << '\n';
    }
    out.indent_down();
    out.indent() << "end" << '\n';
    out << '\n';

    out.indent() << "def send_" << function_signature(function) << '\n';
    out.indent_up();

    std::string argsname = rb_constant_name(function->get_name() + "_args");
    std::string messageSendProc = function->is_oneway() ? "send_oneway_message" : "send_message";

    out.indent() << messageSendProc << "('" << funname << "', " << argsname;

    for (const auto* field : fields) {
      out << ", :" << field->get_name() << " => " << field->get_name();
    }

    out << ")" << '\n';

    out.indent_down();
    out.indent() << "end" << '\n';

    if (!function->is_oneway()) {
      std::string resultname = rb_constant_name(function->get_name() + "_result");
      t_struct noargs(program_);

      t_function recv_function(function->get_returntype(),
                               string("recv_") + function->get_name(),
                               &noargs);
      // Open function
      out << '\n';
      out.indent() << "def " << function_signature(&recv_function) << '\n';
      out.indent_up();

      out.indent() << "fname, mtype, rseqid = receive_message_begin()" << '\n';
      out.indent() << "handle_exception(mtype)" << '\n';

      out.indent() << "if reply_seqid(rseqid)==false" << '\n';
      out.indent() << "  raise \"seqid reply faild\"" << '\n';
      out.indent() << "end" << '\n';

      out.indent() << "result = receive_message(" << resultname << ")" << '\n';

      // Careful, only return _result if not a void function
      if (!function->get_returntype()->is_void()) {
        out.indent() << "return result.success unless result.success.nil?" << '\n';
      }

      t_struct* xs = function->get_xceptions();
      const std::vector<t_field*>& xceptions = xs->get_members();
      for (const auto* xception : xceptions) {
        out.indent() << "raise result." << xception->get_name() << " unless result."
                            << xception->get_name() << ".nil?" << '\n';
      }

      // Careful, only return _result if not a void function
      if (function->get_returntype()->is_void()) {
        out.indent() << "return" << '\n';
      } else {
        out.indent() << "raise "
                               "::Thrift::ApplicationException.new(::Thrift::ApplicationException::"
                               "MISSING_RESULT, '" << function->get_name()
                            << " failed: unknown result')" << '\n';
      }

      // Close function
      out.indent_down();
      out.indent() << "end" << '\n' << '\n';
    }
  }

  out.indent_down();
  out.indent() << "end" << '\n' << '\n';
}

/**
 * Generates a service server definition.
 *
 * @param tservice The service to generate a server for.
 */
void t_rb_generator::generate_service_server(t_rb_ofstream& out, t_service* tservice) {
  // Generate the dispatch methods
  const auto& functions = tservice->get_functions();

  string extends_processor = "";
  if (tservice->get_extends() != nullptr) {
    extends_processor = " < " + full_type_name(tservice->get_extends()) + "::Processor ";
  }

  // Generate the header portion
  out.indent() << "class Processor" << extends_processor << '\n';
  out.indent_up();

  out.indent() << "include ::Thrift::Processor" << '\n' << '\n';

  // Generate the process subfunctions
  for (auto* function : functions) {
    generate_process_function(out, function);
  }

  out.indent_down();
  out.indent() << "end" << '\n' << '\n';
}

/**
 * Generates a process function definition.
 *
 * @param tfunction The function to write a dispatcher for
 */
void t_rb_generator::generate_process_function(t_rb_ofstream& out, t_function* tfunction) {
  // Open function
  out.indent() << "def process_" << tfunction->get_name() << "(seqid, iprot, oprot)" << '\n';
  out.indent_up();

  string argsname = rb_constant_name(tfunction->get_name() + "_args");
  string resultname = rb_constant_name(tfunction->get_name() + "_result");

  out.indent() << "args = read_args(iprot, " << argsname << ")" << '\n';

  t_struct* xs = tfunction->get_xceptions();
  const std::vector<t_field*>& xceptions = xs->get_members();

  // Declare result for non oneway function
  if (!tfunction->is_oneway()) {
    out.indent() << "result = " << resultname << ".new()" << '\n';
  }

  // Try block for a function with exceptions
  if (xceptions.size() > 0) {
    out.indent() << "begin" << '\n';
    out.indent_up();
  }

  // Generate the function call
  t_struct* arg_struct = tfunction->get_arglist();
  const std::vector<t_field*>& fields = arg_struct->get_members();

  out.indent();
  if (!tfunction->is_oneway() && !tfunction->get_returntype()->is_void()) {
    out << "result.success = ";
  }
  out << "@handler." << tfunction->get_name() << "(";
  bool first = true;
  for (const auto* field : fields) {
    if (first) {
      first = false;
    } else {
      out << ", ";
    }
    out << "args." << field->get_name();
  }
  out << ")" << '\n';

  if (!tfunction->is_oneway() && xceptions.size() > 0) {
    out.indent_down();
    for (const auto* xception : xceptions) {
      out.indent() << "rescue " << full_type_name(xception->get_type()) << " => "
                          << xception->get_name() << '\n';
      if (!tfunction->is_oneway()) {
        out.indent_up();
        out.indent() << "result." << xception->get_name() << " = " << xception->get_name()
                            << '\n';
        out.indent_down();
      }
    }
    out.indent() << "end" << '\n';
  }

  // Shortcut out here for oneway functions
  if (tfunction->is_oneway()) {
    out.indent() << "return" << '\n';
    out.indent_down();
    out.indent() << "end" << '\n' << '\n';
    return;
  }

  out.indent() << "write_result(result, oprot, '" << tfunction->get_name() << "', seqid)"
                      << '\n';

  // Close function
  out.indent_down();
  out.indent() << "end" << '\n' << '\n';
}

/**
 * Renders a function signature of the form 'type name(args)'
 *
 * @param tfunction Function definition
 * @return String of rendered function definition
 */
string t_rb_generator::function_signature(t_function* tfunction, string prefix) {
  // TODO(mcslee): Nitpicky, no ',' if argument_list is empty
  return prefix + tfunction->get_name() + "(" + argument_list(tfunction->get_arglist()) + ")";
}

/**
 * Renders a field list
 */
string t_rb_generator::argument_list(t_struct* tstruct) {
  string result = "";

  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;
  bool first = true;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if (first) {
      first = false;
    } else {
      result += ", ";
    }
    result += (*f_iter)->get_name();
  }
  return result;
}

string t_rb_generator::type_name(const t_type* ttype) {
  string name = ttype->get_name();
  if (ttype->is_struct() || ttype->is_xception() || ttype->is_enum() || ttype->is_service()) {
    name = rb_constant_name(ttype->get_name());
  }

  return name;
}

string t_rb_generator::full_type_name(const t_type* ttype) {
  return "::" + rb_namespace_constant_prefix(ttype->get_program()) + type_name(ttype);
}

/**
 * Converts the parse type to a Ruby tyoe
 */
string t_rb_generator::type_to_enum(t_type* type) {
  type = get_true_type(type);

  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_VOID:
      throw "NO T_VOID CONSTRUCT";
    case t_base_type::TYPE_STRING:
      return "::Thrift::Types::STRING";
    case t_base_type::TYPE_BOOL:
      return "::Thrift::Types::BOOL";
    case t_base_type::TYPE_I8:
      return "::Thrift::Types::BYTE";
    case t_base_type::TYPE_I16:
      return "::Thrift::Types::I16";
    case t_base_type::TYPE_I32:
      return "::Thrift::Types::I32";
    case t_base_type::TYPE_I64:
      return "::Thrift::Types::I64";
    case t_base_type::TYPE_DOUBLE:
      return "::Thrift::Types::DOUBLE";
    case t_base_type::TYPE_UUID:
      return "::Thrift::Types::UUID";
    default:
      throw "compiler error: unhandled type";
    }
  } else if (type->is_enum()) {
    return "::Thrift::Types::I32";
  } else if (type->is_struct() || type->is_xception()) {
    return "::Thrift::Types::STRUCT";
  } else if (type->is_map()) {
    return "::Thrift::Types::MAP";
  } else if (type->is_set()) {
    return "::Thrift::Types::SET";
  } else if (type->is_list()) {
    return "::Thrift::Types::LIST";
  }

  throw "INVALID TYPE IN type_to_enum: " + type->get_name();
}

void t_rb_generator::generate_rdoc(t_rb_ofstream& out, t_doc* tdoc) {
  if (tdoc->has_doc()) {
    out.indent();
    generate_docstring_comment(out, "", "# ", tdoc->get_doc(), "");
  }
}

void t_rb_generator::generate_rb_struct_required_validator(t_rb_ofstream& out, t_struct* tstruct) {
  out.indent() << "def validate" << '\n';
  out.indent_up();

  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;

  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    t_field* field = (*f_iter);
    if (field->get_req() == t_field::T_REQUIRED) {
      out.indent() << "raise ::Thrift::ProtocolException.new(::Thrift::ProtocolException::INVALID_DATA, "
                      "'Required field " << field->get_name() << " is unset!')";
      if (field->get_type()->is_bool()) {
        out << " if @" << field->get_name() << ".nil?";
      } else {
        out << " unless @" << field->get_name();
      }
      out << '\n';
    }
  }

  // if field is an enum, check that its value is valid
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    t_field* field = (*f_iter);

    if (field->get_type()->is_enum()) {
      out.indent() << "unless @" << field->get_name() << ".nil? || "
                   << full_type_name(field->get_type()) << "::VALID_VALUES.include?(@"
                   << field->get_name() << ")" << '\n';
      out.indent_up();
      out.indent() << "raise ::Thrift::ProtocolException.new(::Thrift::ProtocolException::INVALID_DATA, "
                      "'Invalid value of field " << field->get_name() << "!')" << '\n';
      out.indent_down();
      out.indent() << "end" << '\n';
    }
  }

  out.indent_down();
  out.indent() << "end" << '\n' << '\n';
}

void t_rb_generator::generate_rb_union_validator(t_rb_ofstream& out, t_struct* tstruct) {
  out.indent() << "def validate" << '\n';
  out.indent_up();

  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;

  out.indent()
      << "raise ::Thrift::ProtocolException.new(::Thrift::ProtocolException::INVALID_DATA, "
         "'Union fields are not set.') if get_set_field.nil? || get_value.nil?"
      << '\n';

  // if field is an enum, check that its value is valid
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    const t_field* field = (*f_iter);

    if (field->get_type()->is_enum()) {
      out.indent() << "if get_set_field == :" << field->get_name() << '\n';
      out.indent() << "  raise "
                      "::Thrift::ProtocolException.new(::Thrift::ProtocolException::INVALID_DATA, "
                      "'Invalid value of field " << field->get_name() << "!') unless "
                   << full_type_name(field->get_type()) << "::VALID_VALUES.include?(get_value)"
                   << '\n';
      out.indent() << "end" << '\n';
    }
  }

  out.indent_down();
  out.indent() << "end" << '\n' << '\n';
}

std::string t_rb_generator::display_name() const {
  return "Ruby";
}


THRIFT_REGISTER_GENERATOR(
    rb,
    "Ruby",
    "    rubygems:        Add a \"require 'rubygems'\" line to the top of each generated file.\n"
    "    namespaced:      Generate files in idiomatic namespaced directories.\n"
    "    zeitwerk:        Generate strict Zeitwerk-compatible output.\n")
