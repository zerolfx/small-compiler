#ifndef ZPC_ENV_H
#define ZPC_ENV_H
#include "ast.h"
#include <map>
#include <string>
#include <stack>

struct Identifier;

class Env {
public:
  void register_identifier(const Identifier* identifier);
  int get_identifier(const Identifier* identifier);
  int get_allocated() { return sym_table.size(); }
  void open_loop();
  void close_loop() {
    loop_st.pop();
  }
  std::string get_loop_start() {
    return loop_st.top().first;
  }
  std::string get_loop_end() {
    return loop_st.top().second;
  }
private:
  std::stack<std::pair<std::string, std::string>> loop_st;
  std::map<std::string, int> sym_table;
};


#endif //ZPC_ENV_H
