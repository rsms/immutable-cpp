#include "test.h"
#include <map>
#include <string>

struct Test {
  const char* name;
  TestFunc    fn;
};

static std::map<std::string,Test>* tests = nullptr;

void TestAdd(const char* name, TestFunc f, unsigned int priority) {
  if (!tests) {
    tests = new std::map<std::string,Test>;
  }
  char buf[12];
  snprintf(buf, sizeof(buf)-1, "%08x", priority);
  tests->emplace(std::string(buf) + "-" + name, Test{name, f});
}


int main(int, const char*[]) {
  if (tests) for (auto& p : *tests) {
    auto& test = p.second;
    printf("test %s ... ", test.name);
    test.fn();
    printf("ok.\n");
  }
  return 0;
}
