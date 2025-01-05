
#include <delegates/delegates.hpp>
#include <memory>
#include <iostream>
#include <string>

using namespace delegates;

void SignalSimpleExample() {
  std::string kDefaultString = "default";
  Signal<void, int, std::string> s/*(123, kDefaultString)*/;

  std::string checkstr = s.args()->get<std::string>(1);

  s += delegates::factory::make_shared<void, int, const std::string&>([](int a, const std::string& s) { std::cout << "signal called from 1, a=" << a << ", s=" << s << std::endl; });

  auto delegate2 = delegates::factory::make<void, int, std::string>([](int a, std::string s) { std::cout << "signal called from 2, a=" << a << ", s=" << s << std::endl; });
  s.add(delegate2, std::string(), [](IDelegate* d) { delete d; });

  // set arguments
  s.args()->set<int>(0, 42);
  std::string str = "hello world";
	s.args()->set<const std::string&>(1, str);

	// perform call
  s();
}


void SignalToSignalExample() {
	Signal<void, int, std::string> s2;

  std::cout << "== Signal to signal example ==" << std::endl;

  {
    Signal<void, int, std::string> s1;
    s1 += delegates::factory::make_shared<void, int, const std::string&>([](int a, const std::string& s) { std::cout << "[1] signal called from 1, a=" << a << ", s=" << s << std::endl; });

    auto delegate2 = delegates::factory::make<void, int, std::string>([](int a, std::string s) { std::cout << "[1] signal called from 2, a=" << a << ", s=" << s << std::endl; });
    s1.add(delegate2, std::string(), [](IDelegate* d) { delete d; });


    s2 += s1;
    s2 += delegates::factory::make_shared<void, int, std::string>([](int a, std::string s) { std::cout << "[2] signal called from 3, a=" << a << ", s=" << s << std::endl; });

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
