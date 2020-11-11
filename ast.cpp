#include "ast.h"

std::string gen_label(const std::string& prefix) {
  static int cnt = 0;
  return prefix + std::to_string(cnt++);
}

std::string Identifier::gen(Env& env) const {
  return fmt::format("lod i 0 {}\n", env.get_identifier(this));
}

std::string AssignStmt::gen(Env& env) const {
  int addr = env.get_identifier(id);
  return expr->gen(env) + fmt::format("str i 0 {}\n", addr);
}

std::string StmtSequence::gen(Env& env) const {
  auto v = std::views::transform(stmts, [&env](Stmt* s){ return s->gen(env); });
//  auto v = stmts | std::views::transform();
  return std::accumulate(v.begin(), v.end(), std::string());
}