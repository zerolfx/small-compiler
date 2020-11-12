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

TEST(comment, z) {
  EXPECT_EQ(go("write /*1+*/ 1"), "1\n");
  EXPECT_EQ(go("write /* */ 1 /* */"), "1\n");
  EXPECT_THROW(go("w/**/rite 1"), std::runtime_error);
  EXPECT_EQ(go(R"(
    write
    // comment 1
    1
    // comment 2
  )"), "1\n");
}

TEST(for_loop, z) {
  EXPECT_EQ(go("for i := 1; i < 3; i := i + 1 do write i end"), "1\n2\n");
  EXPECT_EQ(go("for i := 1; i <= 4; i := i + 1 do i := i + 1; write i end"), "2\n4\n");
}

TEST(do_while, z) {
  EXPECT_EQ(go("i:=0; do write i; i := i + 1 while i < 3"), "0\n1\n2\n");
}

TEST(while_do, z) {
  EXPECT_EQ(go("i:=0; while i < 3 do write i; i := i + 1 end"), "0\n1\n2\n");
}

TEST(repeat_until, z) {
  EXPECT_EQ(go("i:=0; repeat write i; i := i + 1 until i == 3"), "0\n1\n2\n");
}

TEST(more_operators, z) {
  EXPECT_EQ(go("write 5 % 2"), "1\n");
  EXPECT_EQ(go("write 6 % 2"), "0\n");
  EXPECT_EQ(go("i := 1; write ++i; write i"), "2\n2\n");
  EXPECT_EQ(go("i := 1; write --i; write i"), "0\n0\n");
  EXPECT_EQ(go("if odd 3 then write 1 else write 0 end"), "1\n");
  EXPECT_EQ(go("if not odd 3 then write 1 else write 0 end"), "0\n");
  EXPECT_EQ(go("if 1 < 2 or 1 > 2 then write 1 else write 0 end"), "1\n");
  EXPECT_EQ(go("if 1 < 2 and 1 > 2 then write 1 else write 0 end"), "0\n");
  EXPECT_EQ(go("if 1 < 2 xor 1 > 2 then write 1 else write 0 end"), "1\n");
  EXPECT_EQ(go("if 1 < 2 xor 1 < 2 then write 1 else write 0 end"), "0\n");
}

TEST(loop_break, z) {
  EXPECT_EQ(go("for i := 1; i < 3; i := i + 1 do write i; break end; write i"), "1\n1\n");
}

TEST(loop_continue, z) {
  EXPECT_EQ(go(R"(
    for i := 1; i < 4; i := i + 1 do
      if not odd i then continue end;
      write i
    end
  )"), "1\n3\n");
}

TEST(exit, z) {
  EXPECT_EQ(go("write 1; exit; write 2"), "1\n");
}

TEST(bug, fixed) {
  EXPECT_THROW(go("write odd1"), std::runtime_error);
}

TEST(gcd, z) {
  EXPECT_EQ(go(R"(
    x := 72;
    y := 192;
    while y != 0 do
      t := y;
      y := x % y;
      x := t
    end;
    write x
  )"), "24\n");
}

TEST(prime, z) {
  EXPECT_EQ(go(R"(
    for i := 2; i <= 100; i := i + 1 do
      flag := 1;
      for j := 2; j * j <= i; j := j + 1 do
        if i % j == 0 then flag := 0; break end
      end;
      if flag == 1 then write i end
    end
  )"), "2\n3\n5\n7\n11\n13\n17\n19\n23\n29\n31\n37\n41\n43\n47\n53\n59\n61\n67\n71\n73\n79\n83\n89\n97\n");
}