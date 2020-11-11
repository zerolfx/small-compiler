#ifndef ZPC_ENV_H
#define ZPC_ENV_H
#include "ast.h"
#include <map>
#include <string>

struct Identifier;

class Env {
public:
  void register_identifier(const Identifier* identifier);
  int get_identifier(const Identifier* identifier);
  int get_allocated() { return sym_table.size(); }
private:
  std::map<std::string, int> sym_table;
};


#endif //ZPC_ENV_H
