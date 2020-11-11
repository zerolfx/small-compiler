#include "compiler.hpp"


int main() {
  auto in = R"|(
    write /*  1+ */1
)|";
  compile(in);
}
