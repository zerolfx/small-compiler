#include "compiler.hpp"


int main() {
  auto in = R"|(
    for i := 1 ; i < 3 ; i := i + 1 do
      write i
)|";
  compile(in);
}
