#include "compiler.hpp"


int main() {
  auto in = R"|(
    write odd not 3
)|";
  compile(in);
}
