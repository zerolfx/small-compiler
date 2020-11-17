#ifndef ZPC_AST_H
#define ZPC_AST_H

#include <fmt/format.h>
#include <string>
#include <map>
#include <ranges>
#include <numeric>
#include "env.h"

std::string gen_label(const std::string& prefix = "L");

class Env;

struct Node {
  virtual std::string to_string() const = 0;
  virtual std::string gen(Env&) const = 0;
};

struct Expr : Node {};
struct Stmt : Node {};

struct EmptyExpr : Expr {
  EmptyExpr() {}
  std::string to_string() const override { return {}; }
  std::string gen(Env&) const override { return {}; }
};
static EmptyExpr *empty_expr = new EmptyExpr{};

struct EmptyStmt : Stmt {
  EmptyStmt() {}
  std::string to_string() const override { return {}; }
  std::string gen(Env&) const override { return {}; }
};
static EmptyStmt *empty_stmt = new EmptyStmt{};

struct Identifier : Expr {
  explicit Identifier(std::string s) : name(std::move(s)) {}
  std::string name;
  std::string to_string() const override {
    return fmt::format("Identifier({})", name);
  }
  std::string gen(Env& env) const override;
};

struct BinaryOp : Expr {
  std::string op;
  Expr *lhs, *rhs;
  BinaryOp(Expr* lhs, std::string op, Expr* rhs): lhs(lhs), op(std::move(op)), rhs(rhs) {}

  std::string to_string() const override {
    return fmt::format("({} {} {})", lhs->to_string(), op, rhs->to_string());
  }
  std::string gen(Env& env) const override {
    static std::map<std::string, std::string> op_map = {
        {"+", "add"}, {"-", "sub"}, {"*", "mul"}, {"/", "div"},
        {">", "grt"}, {"<", "les"}, {">=", "geq"}, {"<=", "leq"}, {"==", "equ"}, {"!=", "neq"},
    };
    auto code = op_map.count(op) ? op_map[op] + " i\n" : op + "\n";
    return lhs->gen(env) + rhs->gen(env) + code;
  }
};

struct UnaryOp : Expr {
  std::string op;
  Expr* expr;
  UnaryOp(std::string op, Expr* expr): op(std::move(op)), expr(expr) {}

  std::string to_string() const override {
    return fmt::format("({} {})", op, expr->to_string());
  }
  std::string gen(Env& env) const override;
};

struct Num : Expr {
  int v;
  explicit Num(int v): v(v) {}
  std::string to_string() const override {
    return fmt::format("Num({})", v);
  }
  std::string gen(Env&) const override {
    return fmt::format("ldc i {}\n", v);
  }
};



struct AssignStmt : Stmt {
  Identifier *id;
  Expr *expr;
  AssignStmt(Expr* id, Expr* expr): id(dynamic_cast<Identifier*>(id)), expr(expr) {}
  std::string to_string() const override {
    return fmt::format("{} := {}", id->to_string(), expr->to_string());
  }
  std::string gen(Env& env) const override;
};

struct ReadStmt : Stmt {
  Identifier *id;
  explicit ReadStmt(Expr *id): id(dynamic_cast<Identifier*>(id)) {}
  std::string to_string() const override {
    return fmt::format("Read({})", id->to_string());
  }
  std::string gen(Env& env) const override {
    return fmt::format("in i\n") + AssignStmt(id, empty_expr).gen(env);
  }
};

struct WriteStmt : Stmt {
  Expr *expr;
  explicit WriteStmt(Expr *exp): expr(exp) {}
  std::string to_string() const override {
    return fmt::format("Write({})", expr->to_string());
  }
  std::string gen(Env& env) const override {
    return expr->gen(env) + "out i\nldc c '\\n'\nout c\n";
  }
};

struct IfStmt : Stmt {
  Expr *expr;
  Stmt *s1, *s2;
  IfStmt(Expr *expr, Stmt *s1, Stmt *s2): expr(expr), s1(s1), s2(s2) {}
  std::string to_string() const override {
    return fmt::format("If(Cond: {}, Then: {}, Else: {})", expr->to_string(), s1->to_string(), s2->to_string());
  }
  std::string gen(Env& env) const override {
    auto else_label = gen_label("if"), end_label = gen_label("if");
    return expr->gen(env) + fmt::format("fjp {}\n", else_label) + s1->gen(env) + fmt::format("ujp {}\n", end_label) +
           fmt::format("{}:\n", else_label) + s2->gen(env) + fmt::format("{}:\n", end_label);
  }
};

struct StmtSequence : Stmt {
  std::vector<Stmt*> stmts;
  explicit StmtSequence(const std::vector<Stmt*>& stmts): stmts(stmts) {}
  std::string to_string() const override {
    static int indent = 0;
    std::string s;
    s += "{\n";
    indent += 2;
    for (auto& stmt: stmts) s += std::string(indent, ' ') + stmt->to_string() + '\n';
    indent -= 2;
    s += std::string(indent, ' ') + "}";
    return s;
  }
  std::string gen(Env& env) const override;
};

struct ForStmt : Stmt { // for (s1; s2; s3) s4
  Stmt *s1, *s3, *s4;
  Expr *s2;
  ForStmt(Stmt* s1, Expr* s2, Stmt* s3, Stmt* s4): s1(s1), s2(s2), s3(s3), s4(s4) {}
  std::string to_string() const override {
    return fmt::format("For(Init: {}, Cond: {}, Update: {}, Body: {})",
                       s1->to_string(), s2->to_string(), s3->to_string(), s4->to_string());
  }
  std::string gen(Env& env) const override;
};

struct CaseStmt : Stmt {
  Expr *expr;
  std::vector<std::pair<Expr*, Stmt*>> cases;
  CaseStmt(Expr* expr, const std::vector<std::pair<Expr*, Stmt*>>& cases): expr(expr), cases(cases) {}
  std::string to_string() const override {
    std::string cs;
    for (int i = 0; i < cases.size(); ++i) {
      if (i) cs += ", ";
      cs += fmt::format("{} => {}", cases[i].first->to_string(), cases[i].second->to_string());
    }
    return fmt::format("Match({}: {})", expr->to_string(), cs);
  }

  std::string gen(Env& env) const override {
    auto end_label = gen_label("case_end");
    auto next_label = gen_label("case");
    auto res = expr->gen(env);
    for (auto& c: cases) {
      res += fmt::format("{}:\n", next_label);
      next_label = gen_label("case");
      res += "dpl i\n" + c.first->gen(env) + "equ i\n";
      res += fmt::format("fjp {}\n", next_label);
      res += c.second->gen(env);
      res += fmt::format("ujp {}\n", end_label);
    }
    res += "pop\n";
    res += fmt::format("{}:\n{}:\n", next_label, end_label);
    return res;
  }
};

struct BreakStmt : Stmt {
  std::string to_string() const override { return "Break"; }
  std::string gen(Env& env) const override;
};

struct ContinueStmt : Stmt {
  std::string to_string() const override { return "Continue"; }
  std::string gen(Env& env) const override;
};

struct ExitStmt : Stmt {
  std::string to_string() const override { return "Exit"; }
  std::string gen(Env& env) const override { return "hlt\n"; }
};

#endif //ZPC_AST_H
