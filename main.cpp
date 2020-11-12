#include "compiler.hpp"


int main() {
  auto in = R"|(
    i := 1;
    match i of
      case 1 => write 1
      case 2 => write 2
    end
)|";
  compile(in);
}
