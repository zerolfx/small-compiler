#include "compiler.hpp"


int main() {

//  auto parser = build_parser();


  auto in = R"|(
    write (1 + 1)
)|";
  compile(in);

//  auto in = Scanner(R"|(
//    a := 1 + 2 * 3;
//
//    read a;
//    write a;
//    a := a + a
//)|");
//  auto res = eof(parser)(in);
//  std::cout << res << std::endl;
//  std::cout << "Remain: " << in << std::endl;
//  Env env;
//  std::cout << res.value()->gen(env);

}

