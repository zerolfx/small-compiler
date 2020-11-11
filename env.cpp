#include "env.h"
#include <fmt/format.h>
#include <exception>

int Env::get_identifier(const Identifier *identifier) {
  if (!sym_table.contains(identifier->name)) register_identifier(identifier);
  return sym_table[identifier->name];
//  auto it = sym_table.find(identifier->name);
//  if (it == sym_table.end())
//    throw std::runtime_error(fmt::format("Reference to undefined variable {}.", identifier->name));
//  return it->second;
}

void Env::register_identifier(const Identifier* identifier) {
  if (sym_table.contains(identifier->name))
    throw std::runtime_error(fmt::format("Variable {} is already defined.", identifier->name));
  sym_table[identifier->name] = sym_table.size();
}