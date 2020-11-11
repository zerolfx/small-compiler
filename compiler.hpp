#ifndef ZPC_COMPILER_HPP
#define ZPC_COMPILER_HPP

#include "zpc/helper.hpp"
#include "zpc/scanner.hpp"
#include "zpc/printer.hpp"
#include <fmt/format.h>
#include <algorithm>
#include <functional>
#include <numeric>
#include <utility>
#include <map>
#include <stack>
#include <set>

#include "ast.h"

std::ostream& operator << (std::ostream& os, const Node* n) {
  return os << n->to_string();
}


Parser<Expr*> build_binary_parser(const Parser<Expr*>& p, const Parser<std::string>& op) {
  return seq(p, many(seq(op, p))) %= [](Expr* e, const std::vector<std::tuple<std::string, Expr*>>& rest) {
    return std::accumulate(rest.begin(), rest.end(), e, [](Expr* e, const std::tuple<std::string, Expr*>& t)->Expr* {
      return new BinaryOp(e, std::get<std::string>(t), std::get<Expr*>(t));
    });
  };
}

std::set<std::string> kw_set;

auto kw(const std::string& s) {
  kw_set.insert(s);
  return lit(s).atom();
}

auto build_parser() {
  auto letter = alt(ch_range('a', 'z'), ch_range('A', 'Z'));
  auto digit = ch_range('0', '9');
  Parser<Expr*> identifier = raw(seq(letter, many(alt(digit, letter)))).atom()
      % [](auto&& s){ return new Identifier(s); }
      /= [](Identifier* id){ return !kw_set.contains(id->name); };
  Parser<Expr*> number = raw(many1(digit)).atom() % [](auto&& s){ return new Num(std::stoi(s)); };

  static Parser<Expr*> expr;
  Parser<Expr*> factor = alt(number, identifier,
                             seq(lit("("), lazy(expr), lit(")")) % RESOLVE_OVERLOAD(std::get<1>));
  Parser<Expr*> term = build_binary_parser(factor, alt(lit("*"), lit("/")));
  Parser<Expr*> simple_exp = build_binary_parser(term, alt(lit("+"), lit("-")));
  expr = build_binary_parser(simple_exp, alt(lit("<="), lit(">="), lit("=="), lit(">"), lit("<")));

  static Parser<Stmt*> statement;
  Parser<Stmt*> stmt_sequence = sep_by(lazy(statement), lit(";")) % [](auto&& stmts){ return new StmtSequence(stmts); };
  Parser<Stmt*> read_stmt = seq(kw("read"), identifier) %= [](auto&& _, auto&& id){ return new ReadStmt(id); };
  Parser<Stmt*> write_stmt = seq(kw("write"), expr) %= [](auto&& _, auto&& e){ return new WriteStmt(e); };
  Parser<Stmt*> assign_stmt = seq(identifier, lit(":="), expr) %= [](auto&& id, auto&& _, auto&& e){ return new AssignStmt(id, e); };
  Parser<Stmt*> if_stmt = seq(
      kw("if"),
      expr,
      kw("then"), stmt_sequence,
      opt(seq(kw("else"), stmt_sequence)) % [](const optional<std::tuple<std::string, Stmt*>>& x)->Stmt* {
        if (x.has_value()) return std::get<1>(x.value());
        else return empty_stmt;
      },
      kw("end")
  ) %= [](auto&& _1, auto&& e, auto&& _2, auto&& s1, auto&& s2, auto&& _3){
    return new IfStmt(e, s1, s2);
  };
  statement = alt(read_stmt, write_stmt, assign_stmt, if_stmt);

  return eof(stmt_sequence);
}


std::string compile(const std::string& in) {
  static auto parser = build_parser();
  auto scanner = Scanner(in);
  auto res = parser(scanner);
  if (!res) {
    std::cerr << "Compile Error" << std::endl;
    std::cerr << "Remain: " << scanner << std::endl;
    throw std::runtime_error("Compile Error");
  } else {
    std::cout << "Ast:\n" << res.value() << std::endl;
    Env env;
    auto body = res.value()->gen(env);
    return fmt::format("ssp {}\n", env.get_allocated()) + body + "hlt\n";
  }
}

#endif //ZPC_COMPILER_HPP
