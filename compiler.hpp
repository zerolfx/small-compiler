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

auto build_parser() {
  auto letter = alt(ch_range('a', 'z'), ch_range('A', 'Z'));
  auto digit = ch_range('0', '9');

  auto kw = [&](const std::string& s) {
    kw_set.insert(s);
    return seq(lit(s), not_predicate(alt(digit, letter))).atom() % RESOLVE_OVERLOAD(std::get<0>);
  };

  Parser<Expr*> identifier = raw(seq(letter, many(alt(digit, letter)))).atom()
    % [](auto&& s){ return new Identifier(s); }
    /= [](Identifier* id){ return !kw_set.contains(id->name); };
  Parser<Expr*> number = raw(many1(digit)).atom() % [](auto&& s){ return new Num(std::stoi(s)); };

  static Parser<Expr*> expr;
  Parser<Expr*> factor = alt(number, identifier,
                             seq(lit("("), lazy(expr), lit(")")) % RESOLVE_OVERLOAD(std::get<1>));
  Parser<Expr*> unary_expr = seq(
    many(alt(kw("odd"), kw("not"), lit("++"), lit("--"))),
    factor
  ) %= [](const std::vector<std::string>& v, Expr* e) {
    return std::accumulate(v.rbegin(), v.rend(), e, [](Expr* e, const std::string& op)->Expr* {
      return new UnaryOp(op, e);
    });
  };
  Parser<Expr*> expr_1 = build_binary_parser(unary_expr,
    alt(lit("*"), lit("/"), lit("%") % [](auto&&){ return std::string("mod"); }));
  Parser<Expr*> expr_2 = build_binary_parser(expr_1, alt(lit("+"), lit("-")));
  Parser<Expr*> expr_3 = build_binary_parser(expr_2, alt(lit("<="), lit(">="), lit("=="), lit(">"), lit("<"), lit("!=")));
  Parser<Expr*> expr_4 = build_binary_parser(expr_3, lit("and"));
  Parser<Expr*> expr_5 = build_binary_parser(expr_4, alt(lit("or"), lit("xor")));
  expr = expr_5;

  static Parser<Stmt*> statement;
  auto lazy_stmt = lazy(statement);
  Parser<Stmt*> stmt_sequence = sep_by(lazy_stmt, lit(";")) % [](auto&& stmts){ return new StmtSequence(stmts); };
  Parser<Stmt*> read_stmt = seq(kw("read"), identifier) %= [](auto&&, auto&& id){ return new ReadStmt(id); };
  Parser<Stmt*> write_stmt = seq(kw("write"), expr) %= [](auto&&, auto&& e){ return new WriteStmt(e); };
  Parser<Stmt*> assign_stmt = seq(identifier, lit(":="), expr) %= [](auto&& id, auto&&, auto&& e){ return new AssignStmt(id, e); };
  Parser<Stmt*> if_stmt = seq(
    kw("if"),
    expr,
    kw("then"), stmt_sequence,
    opt(seq(kw("else"), stmt_sequence)) % [](const optional<std::tuple<std::string, Stmt*>>& x)->Stmt* {
      if (x.has_value()) return std::get<1>(x.value());
      else return empty_stmt;
    },
    kw("end")
  ) %= [](auto&&, auto&& e, auto&&, auto&& s1, auto&& s2, auto&&){
    return new IfStmt(e, s1, s2);
  };

  Parser<Stmt*> for_stmt = seq(
    kw("for"),
    lazy_stmt, lit(";"), expr, lit(";"), lazy_stmt, kw("do"),
    stmt_sequence, kw("end")
  ) %= [](auto&&, auto&& s1, auto&&, auto&& s2, auto&&, auto&& s3, auto&&, auto&& s4, auto&&) {
    return new ForStmt(s1, s2, s3, s4);
  };

  Parser<Stmt*> do_while = seq(kw("do"), stmt_sequence, kw("while"), expr) %=
    [](auto&&, auto&& s, auto&&, auto&& e) {
      return new ForStmt(s, e, empty_stmt, s);
    };

  Parser<Stmt*> repeat_until = seq(kw("repeat"), stmt_sequence, kw("until"), expr) %=
    [](auto&&, auto&& s, auto&&, auto&& e) {
     return new ForStmt(s, new UnaryOp("not", e), empty_stmt, s);
    };

  Parser<Stmt*> while_do = seq(kw("while"), expr, kw("do"), stmt_sequence, kw("end")) %=
     [](auto&&, auto&& e, auto&&, auto&& s, auto&&) {
       return new ForStmt(empty_stmt, e, empty_stmt, s);
     };

  Parser<Stmt*> control_stmt = alt(
    kw("break") % [](auto&&)->Stmt* { return new BreakStmt; },
    kw("exit") % [](auto&&)->Stmt* { return new ExitStmt; },
    kw("continue") % [](auto&&)->Stmt* { return new ContinueStmt; }
  );

  Parser<Stmt*> case_stmt = seq(
    kw("match"), expr, kw("of"),
    many(
      seq(kw("case"), expr, lit("=>"), stmt_sequence)
      %= [](auto&&, auto&& e, auto&&, auto&& s){ return std::make_pair(e, s); }),
    kw("end")
  ) %= [](auto&&, auto&& e, auto&&, auto&& v, auto&&) {
    return new CaseStmt(e, v);
  };

  statement = alt(
    read_stmt, write_stmt, assign_stmt,
    if_stmt, for_stmt, repeat_until, do_while, while_do, control_stmt, case_stmt
  );

  Parser<Stmt*> program = seq(stmt_sequence, eof) % RESOLVE_OVERLOAD(std::get<0>);

  return program;
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
