#include <gtest/gtest.h>
#include <fstream>
#include "compiler.hpp"

// https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
std::string exec(const char* cmd) {
  std::array<char, 128> buffer{};
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
}

std::string go(const std::string& in) {
  auto p_code = compile(in);
  std::ofstream ofs("tmp.txt", std::ios::binary);
  ofs << p_code;
  ofs.close();
  std::cout << p_code << std::endl;
  auto output = exec("./Pmachine tmp.txt");
  for (int i = 0; i < 3; ++i) {
    auto pos = output.find_last_of('\n');
    output.erase(pos);
  }
  std::cout << "Result:\n" << output << std::endl;
  return output;
}



TEST(arithmetic, z) {
  EXPECT_EQ(go("write 1 + 1"), "2\n");
  EXPECT_EQ(go("write (1 + 1) * 2 + 3 + 3"), "10\n");
  EXPECT_EQ(go("write 10 / 3"), "3\n");
  EXPECT_EQ(go("write 11 / 3"), "3\n");
  EXPECT_EQ(go("write 12 / 3"), "4\n");
}

TEST(variable, z) {
  EXPECT_EQ(go("x := 1; write x"), "1\n");
  EXPECT_EQ(go("x := 1; y := x + 1; write y"), "2\n");
  EXPECT_EQ(go("x := 1; x := x + 1; write x"), "2\n");
}

TEST(if_statement, z) {
  EXPECT_EQ(go("if 2>=1 then write 1 else write 2 end"), "1\n");
  EXPECT_EQ(go("if 2<=1 then write 1 else write 2 end"), "2\n");
  EXPECT_EQ(go("if 2>=1 then write 1 end"), "1\n");
  EXPECT_EQ(go("if 2<=1 then write 1 end"), "");
  EXPECT_EQ(go("if 1 < 2 then write 1; write 2 end"), "1\n2\n");
}