//
// Copyright (c) 2025, Alex Bobryshev <alexbobryshev555@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include <delegates/delegates.hpp>
#include <memory>
#include <iostream>
#include <string>

USING_DELEGATES_BASE_NAMESPACE
using namespace DELEGATES_BASE_NAMESPACE::delegates;

int DelegateFn(std::string s) {
  std::cout << s << std::endl;
  return 42;
}

void DelegateWithFunctionHelloWorld() {
  // delegate make_unique < ResultType, arguments... >
  auto delegate = delegates::factory::make_unique<int, std::string>(&DelegateFn);

  // Set parameter #0 to string
  delegate->args()->set<std::string>(0, "Hello world!");

  // Perform call
  delegate->call();

  // Get call result
  int ret = delegate->result()->get<int>();

  std::cout << ret << std::endl;
}

void DelegateWithLambdaHelloWorld() {
  auto delegate = delegates::factory::make_unique<int, std::string>([](std::string s)->int {
    std::cout << s;
    return 42;
    });

  delegate->args()->set<std::string>(0, "Hello world!");
  delegate->call();
  int ret = delegate->result()->get<int>();

  std::cout << ret << std::endl;
}

using namespace std;

class Printer {
public:
  void PrintInt(int val) { cout << val << endl; }
  void PrintString(string s) { cout << s << endl; }
  void PrintIntConst(int val) const { cout << "const " << val << endl; }
};

void DelegatesClassMethods() {
  auto printer = make_shared<Printer>();  
  std::list<std::shared_ptr<IDelegate> > sigs;

  sigs.push_back(factory::make_shared(printer, &Printer::PrintInt, 42));
  sigs.push_back(factory::make_shared(printer, &Printer::PrintString, std::string("Hello")));
  sigs.push_back(factory::make_shared(printer, &Printer::PrintIntConst, 1234));

  for (auto& sig : sigs) {
    sig->call();
  }
}

void DelegatesMixedExample() {
  auto printer = make_shared<Printer>();
  list<shared_ptr<IDelegate> > calls;

  // parameters initial values may be set immediately in make_shared call
  calls.push_back(factory::make_shared(printer, &Printer::PrintInt, 42));
  calls.push_back(factory::make_shared(printer, &Printer::PrintString, string("Hello")));
  calls.push_back(factory::make_shared(printer, &Printer::PrintIntConst, 1234));

  // or set up later
  auto delegate1 = factory::make_shared(printer, &Printer::PrintInt);
  delegate1->args()->set<int>(0, 1234); // set parameteer 0 to 1234
  calls.push_back(delegate1);
  
  auto delegate2 = factory::make_shared(printer, &Printer::PrintString);
  delegate2->args()->set<string>(0, "TEST");
  calls.push_back(delegate2);

  // lambda, result type void, args: int, const std::string&
  auto delegate3 = delegates::factory::make_shared<void, int, const std::string&>([](int a, const std::string& s) { 
    std::cout << "delegate called, a=" << a << ", s=" << s << std::endl; 
  });

  delegate3->args()->set<int>(0, 5432);
  delegate3->args()->set<std::string>(1, "TestLambda");

  calls.push_back(delegate3);

  // call without know anything about parameters
  for (auto& d : calls) {
    d->call();
  }
}

void DelegateWithReferenceTypes() {
  auto delegate = factory::make_unique<void,const string&,string&>([](const string& in, std::string& out) {
    if (in == "hello")
      out = "world";
    });

  delegate->args()->set<string>(0, "hello"); // Set parameter #0 to string
  delegate->call();  // Perform call
  std::string& out = delegate->args()->get_ref<std::string>(1);
  cout << out << endl;  // print "world"
}

void DelegateUsageExample() {
  // create delegate with lambda like function   int delegate(const std::string& s)
  IDelegate* delegate = delegates::factory::make<int, const std::string&>([](const std::string& s)->int {
    std::cout << s;
    return 42;
    });

  // set argument 0 value separately from delegate function definition
  delegate->args()->set<std::string>(0, "hello world");

  // call delegate separately from arguments specification
  delegate->call();

  // get result value
  bool ret = delegate->result()->get<bool>();
  delete delegate;
}

void SignalSimpleExample() {
  std::string kDefaultString = "default";
  Signal<void, int, const std::string&> s/*(123, kDefaultString)*/;

  std::string checkstr = s.args()->get<std::string>(1);

  s += delegates::factory::make_shared<void, int, const std::string&>([](int a, const std::string& s) { std::cout << "signal called from 1, a=" << a << ", s=" << s << std::endl; });

  auto delegate2 = delegates::factory::make<void, int, std::string>([](int a, std::string s) { std::cout << "signal called from 2, a=" << a << ", s=" << s << std::endl; });
  s.add(delegate2, std::string(), ISignal::kDelegateArgsMode_UseSignalArgs, [](IDelegate* d) { delete d; });

  // set arguments
  s.args()->set<int>(0, 42);
  std::string str = "hello world";
	s.args()->set<const std::string&>(1, str);

	// perform call
  s();
}


void SignalToSignalExample() {
	Signal<void, int, const std::string&> s2;

  std::cout << "== Signal to signal example ==" << std::endl;

  {
    Signal<void, int, const std::string&> s1;
    s1 += delegates::factory::make_shared<void, int, const std::string&>([](int a, const std::string& s) { std::cout << "[1] signal called from 1, a=" << a << ", s=" << s << std::endl; });

    auto delegate2 = delegates::factory::make<void, int, std::string>([](int a, std::string s) { std::cout << "[1] signal called from 2, a=" << a << ", s=" << s << std::endl; });
    s1.add(delegate2, std::string(), ISignal::kDelegateArgsMode_UseSignalArgs, [](IDelegate* d) { delete d; });


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
  DelegatesMixedExample();
  DelegateWithReferenceTypes();
  DelegatesClassMethods();
  DelegateWithFunctionHelloWorld();
	SignalSimpleExample();
	SignalToSignalExample();
  return 0;
}



