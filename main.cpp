#include "compiler.hpp"
#include <fstream>

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cout << "Usage: " << argv[0] << " input-file output-file" << std::endl;
    return 1;
  }
  std::ifstream ifs{argv[1]};
  std::string in{std::istreambuf_iterator<char>{ifs}, {}};
  ifs.close();

  std::ofstream ofs{argv[2]};
  ofs << compile(in);
  ofs.close();
}
