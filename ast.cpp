#include "ast.h"
#include <cassert>

std::string gen_label(const std::string& prefix) {
  static int cnt = 0;
  return prefix + std::to_string(cnt++);
}

std::string Identifier::gen(Env& env) const {
  return fmt::format("lod i 0 {}\n", env.get_identifier(this));
}

std::string UnaryOp::gen(Env &env) const {
  if (op == "++" || op == "--") {
    auto id = dynamic_cast<Identifier*>(expr);
    if (id == nullptr) throw std::runtime_error(op + " should only used on variable.");
    return AssignStmt(id, new BinaryOp(id, op.substr(1), new Num(1))).gen(env) + id->gen(env);
  } else if (op == "not") {
    return expr->gen(env) + "not\n";
  } else if (op == "odd") {
    return BinaryOp(new BinaryOp(expr, "mod", new Num(2)), "==", new Num(1)).gen(env);
  }
  assert(0);
}

std::string AssignStmt::gen(Env& env) const {
  env.register_identifier(id);
  int addr = env.get_identifier(id);
  return expr->gen(env) + fmt::format("str i 0 {}\n", addr);
}

std::string StmtSequence::gen(Env& env) const {
  auto v = std::views::transform(stmts, [&env](Stmt* s){ return s->gen(env); });
  return std::accumulate(v.begin(), v.end(), std::string());
}

std::string ForStmt::gen(Env& env) const {
  env.open_loop();
  auto continue_label = env.get_loop_start();
  auto end_label = env.get_loop_end();
  auto start_label = gen_label("if");
  auto res = s1->gen(env) + fmt::format("{}:\n", start_label) +
             s2->gen(env) + fmt::format("fjp {}\n", end_label) +
             s4->gen(env) + fmt::format("{}:\n", continue_label) +
             s3->gen(env) + fmt::format("ujp {}\n", start_label) +
             fmt::format("{}:\n", end_label);
  env.close_loop();
  return res;
}

std::string BreakStmt::gen(Env &env) const {
  return fmt::format("ujp {}\n", env.get_loop_end());
}

std::string ContinueStmt::gen(Env &env) const {
  return fmt::format("ujp {}\n", env.get_loop_start());
}