#ifndef ZPC_AST_H
#define ZPC_AST_H

#include <fmt/format.h>
#include <string>
#include <map>
#include <ranges>
#include <numeric>
#include "env.h"

class Env;

struct Node {
  virtual std::string to_string() const = 0;
  virtual std::string gen(Env&) const = 0;
};

struct Expr : Node {};

struct EmptyExpr : Expr {
  EmptyExpr() {}
  std::string to_string() const override {
    return {};
  }
  std::string gen(Env&) const override {
    return {};
  }
};

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
    return lhs->gen(env) + rhs->gen(env) + op_map[op] + " i\n";
  }

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

struct Stmt : Node {};

struct AssignStmt : Stmt {
  Identifier *id;
  Expr *exp;
  AssignStmt(Expr* id, Expr* exp): id(dynamic_cast<Identifier*>(id)), exp(exp) {}
  std::string to_string() const override {
    return fmt::format("{} := {}", id->to_string(), exp->to_string());
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
    return fmt::format("in i") + AssignStmt(id, new EmptyExpr{}).gen(env);
  }
};

struct WriteStmt : Stmt {
  Expr *exp;
  explicit WriteStmt(Expr *exp): exp(exp) {}
  std::string to_string() const override {
    return fmt::format("Write({})", exp->to_string());
  }
  std::string gen(Env& env) const override {
    return exp->gen(env) + "out i\nldc c '\\n'\nout c\n";
  }
};



struct StmtSequence : Stmt {
  std::vector<Stmt*> stmts;
  explicit StmtSequence(const std::vector<Stmt*>& stmts): stmts(stmts) {}
  std::string to_string() const override {
    std::string s;
    for (auto& stmt: stmts) s += stmt->to_string() + '\n';
    return s;
  }
  std::string gen(Env& env) const override;
};


#endif //ZPC_AST_H
