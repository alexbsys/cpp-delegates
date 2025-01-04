
#include "../include/delegate.hpp"
#include "../include/i_delegate.h"
#include "../include/signal.hpp"
#include <memory>
#include <iostream>
#include <string>

using namespace delegates;

void SignalSimpleExample() {
  Signal<void, int, const std::string&> s;
  s += delegates::factory::make_shared<void, int, const std::string&>([](int a, const std::string& s) { std::cout << "signal called from 1, a=" << a << ", s=" << s << std::endl; }, std::nullptr_t{});

  auto delegate2 = delegates::factory::make<void, int, std::string>([](int a, std::string s) { std::cout << "signal called from 2, a=" << a << ", s=" << s << std::endl; }, std::nullptr_t{});
  s.add(delegate2, std::string(), [](IDelegate* d) { delete d; });

  // set arguments
  s.args()->set<int>(0, 42);
	s.args()->set<std::string>(1, "hello world");

	// perform call
  s();
}


void SignalToSignalExample() {
	Signal<void, int, const std::string&> s2;

  std::cout << "== Signal to signal example ==" << std::endl;

  {
    Signal<void, int, const std::string&> s1;
    s1 += delegates::factory::make_shared<void, int, const std::string&>([](int a, const std::string& s) { std::cout << "[1] signal called from 1, a=" << a << ", s=" << s << std::endl; }, std::nullptr_t{});

    auto delegate2 = delegates::factory::make<void, int, std::string>([](int a, std::string s) { std::cout << "[1] signal called from 2, a=" << a << ", s=" << s << std::endl; }, std::nullptr_t{});
    s1.add(delegate2, std::string(), [](IDelegate* d) { delete d; });


    s2 += s1;
    s2 += delegates::factory::make_shared<void, int, std::string>([](int a, std::string s) { std::cout << "[2] signal called from 3, a=" << a << ", s=" << s << std::endl; }, std::nullptr_t{});

    // set arguments
    s2.args()->set<int>(0, 42);
    s2.args()->set<std::string>(1, "hello world");

    std::cout << "Perform call with 2 signals" << std::endl;
    s2();
  }

  // set arguments
  s2.args()->set<int>(0, 43);
  s2.args()->set<std::string>(1, "hello world2");

  std::cout << "Perform call when one signal destroyed" << std::endl;
  s2();
}

int main(int argc, char* argv[]) {
	SignalSimpleExample();
	SignalToSignalExample();
  return 0;
}
