#include "compiler.hpp"


int main() {
  auto in = R"|(
    if 1 + 1 then write 1 else write 2 end
)|";
  compile(in);
}
