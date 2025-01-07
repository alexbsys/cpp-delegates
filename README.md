# cpp-delegates

C++ delegates and signals library

* C++14 standard, no dependencies
* Dependency-injection interface is supported
* RTTI is recommended (used for type checking in runtime, library may work without RTTI support)
* Headers-only. All headers can be concatenated to single file
* Slots can be: static functions, lambdas, `std::function`, class methods (regular and const)
* For class method calls pointer to class can be raw pointer, `shared_ptr` or `weak_ptr`
* Thread-safe

## Delegates and signals

**Delegate** means single call activity with stored arguments inside it, and saved result. Caller may not know parameters or result type, but he can call delegate via `IDelegate::call()`.

**Signal** is "delegates aggregator": it implements `IDelegate` interface, but can hold more than one delegate inside, but with same arguments and return type.
When *Signal* is called, all delegates inside it will be called with same parameters.

## How to build

* In your own project: just copy directory `include/delegates` to your project includes dir and just include in C++: c++`#include <delegates/delegates.hpp>`
* Build examples and tests: run `cmake -B build` in project directory

## Minimal usage example

### Delegate with static function call

```c++
#include <delegates/delegates.hpp>
#include <iostream>
#include <string>
#include <memory>

using namespace std;
using namespace delegates;

int DelegateFn(std::string s) {
  std::cout << s << std::endl;
  return 42;
}

void main() {
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
```

### Delegate with lambda call

```c++
#include <delegates/delegates.hpp>
#include <iostream>
#include <string>
#include <memory>

using namespace std;
using namespace delegates;

void main() {
  auto delegate = factory::make_unique<int,string>([](string s)->int {
    cout << s << endl;
    return 42;
    });

  delegate->args()->set<string>(0, "Hello world!");
  delegate->call();
  int ret = delegate->result()->get<int>();
  cout << ret << endl;
}
```

### Class methods calls

```c++
#include <delegates/delegates.hpp>
#include <iostream>
#include <string>
#include <memory>
#include <list>

using namespace std;
using namespace delegates;

class Printer {
public:
  void PrintInt(int val) { cout << val << endl; }
  void PrintString(string s) { cout << s << endl; }
  void PrintIntConst(int val) const { cout << "const " << val << endl; }
};

void main() {
  auto printer = make_shared<Printer>();  
  list<shared_ptr<IDelegate> > sigs;

  // parameters initial values may be set in make_shared call
  sigs.push_back(factory::make_shared(printer, &Printer::PrintInt, 42));
  sigs.push_back(factory::make_shared(printer, &Printer::PrintString, string("Hello")));
  sigs.push_back(factory::make_shared(printer, &Printer::PrintIntConst, 1234));

  for (auto& sig : sigs) {
    sig->call();
  }
}
```

## Features

### Delegates

Certainly, delegate definition, arguments specification, call and result checking can be performed in separated places from different threads, etc.
Also `IDelegate` is common virtual C++ interface struct without any template parameters: caller is not required to know the details (but can check them if necessary)

Delegates with different types, functions, arguments types and count, can be stored in single list and called without providing any information.
Delegate argument can be simple or complex type. 

```c++
// create delegate with lambda like function   int delegate(const std::string& s)
IDelegate* delegate = delegates::factory::make<int,const std::string&>([](const std::string& s)->int {
  std::cout << s;
  return 42;
});

// set argument 0 value separately from delegate function definition
delegate->args()->set<std::string>(0, "hello world");

// call delegate separately from arguments specification
delegate->call();

// get result value
bool ret = delegate->result()->get<bool>();
```

Raw pointers, shared pointers and unique pointers are provided by factory:
```c++
IDelegate* d1 = delegates::factory::make<TResult, TArgs...>(lambda); // call lambda with result type and args
IDelegate* d2 = delegates::factory::make<TResult, TArgs...>(std::function<TResult(TArgs...)>); // call functional with result type and args
IDelegate* d3 = delegates::factory::make<TClass, TResult, TArgs...>(class_ptr, &TClass::Method); // call class method

std::shared_ptr<IDelegate> d4 = delegates::factory::make_shared<TResult, TArgs...>(lambda); // create shared delegate
std::unique_ptr<IDelegate> d5 = delegates::factory::make_unique<TResult, TArgs...>(std::function<TResult(TArgs...)>); // create unique delegate

// for any call of make, make_shared, make_unique, initial arguments values may be provided
IDelegate* d6 = delegates::factory::make<void, float, int>(lambda, 1.2f, 4);
std::shared_ptr<IDelegate> d7 = delegates::factory::make_shared<void, int, float>(lambda, 5, 2.3f);

// For complex types, explicit values declaration may be useful:
auto d8 = delegates::factory::make_unique<void, std::string, SomeClass>(lambda, delegates::DelegateArgs<std::string, SomeClass>("test", SomeClass(123)));
```

### Signals

Static object declaration C++ syntax can be used (`Signal<void> sig`), or dependency injected pointers (`std::shared_ptr<ISignal> sig = delegates::factory::make_shared_signal<void>(...)`).

```c++
using namespace delegates;

Signal<bool,int> static_signal;   // Static signal implemented ISignal interface too
std::shared_ptr<ISignal> shared_signal = factory::make_shared_signal<void,int>();
std::unique_ptr<ISignal> unique_signal = factory::make_unique_signal<void,int>();
ISignal* raw_signal = factory::make<void,int>();
```

Non-const reference types inside signals and delegates are not allowed:

```c++
Signal<void, std::string> signal0;  // OK
Signal<void, const std::string&> signal1; // OK, string can be passed
Signal<void, std::string&> signal2; // FAIL, compilation error

// If you need return parameters as arguments, please use pointers:
Signal<void, std::string*> signal3; // OK, delegate can change value through pointer
```

#### Add delegates to signal

When signal is static object:

```c++
Signal<bool,int> signal;
signal += factory::make_shared<bool,int>([](int a)->bool { return a==42; });
signal += factory::make_shared<bool,int>([](int a)->bool { return a==43; });
signal += factory::make<bool,int>([](int a)->bool { return a==44; });

signal(); // call
```

When signal is pointer:

```c++
std::shared_ptr<ISignal> signal = factory::make_shared_signal<void,int>();
signal->add(factory::make_shared<void,int>([](int){}));
signal->add(factory::make<void,int>([](int){}));

signal->call();  // call
```

## Set and get arguments

Arguments are accessible through `IDelegateArgs` interface:
```c++
std::shared_ptr<IDelegate> delegate;
...
delegate->args(); // returns pointer to IDelegateArgs

Signal<void,int> signal1;
std::shared_ptr<ISignal> signal2;
...
signal1.args();
signal2->args(); // returns pointer to IDelegateArgs
```

High-level interface:
* Get arguments count
* Set argument value by index when he knows argument type
* Get argument value by index when he knows argument type
* Clear argument value

```c++
size_t args_count = delegate->args()->size(); // get delegate or signal arguments count

int v = delegate->args()->get<int>(1);  // get argument #1 as int. Works when arg type is int or const int&
// if type is not the same, exception will be thrown

delegate->args()->set<int>(2, 4); // set argument #2 to int(4)

// For pointers or complex type, user may provide deleter for argument value.
// Deleter will be called when argument value is released (delegate deleted or argument value updated)
delegate->args()->set<int*>(0, new int[10], [](int* p) { delete [] p; });

delegate->args()->clear(1); // clear argument #1 value 
delegate->args()->clear(); // clear all arguments values and set to default
```

Low-level interface:
* Check RTTI type hash by argument index (RTTI must be enabled)
* Get pointer to argument value, type may be unknown
* Set pointer to argument value, type may be unknown

```c++
size_t arg_type_hash = delegate->args->hash_code(2); // get argument #2 hash code. Arguments numbers started from 0
// hash code equals to typeid(T).hash_code(), hashes for T, T& and const T& are the same

void* p = delegate->args()->get_ptr(2); // get raw ptr to argument value #2. Type is unknown

// set value when type is unknown. User has pointer to value and RTTI type hash of value type
int n = 6;
void* pn = reinterpret_cast<void*>(&n);
size_t n_hash = typeid(int).hash_code();
delegate->args()->set_ptr(2, pn, n_hash);

```

## Get call result

Delegate or signal call result is accessible through `IDelegateResult` interface:
```c++
std::shared_ptr<IDelegate> delegate;
...
delegate->result(); // returns pointer to IDelegateResult

Signal<void> signal1;
std::shared_ptr<ISignal> signal2;
...
signal1.result();
signal2->result(); // returns pointer to IDelegateResult
```

How to use:
```c++
size_t result_type_hash = delegate->result()->hash_code(); // equals to typeid(TResult).hash_code()

if (delegate->result()->has_value()) { // has_value returns true when result type is not void and result was set
  int r = delegate->result()->get<int>(); // get result value when its type is int
  std::string r = delegate->result()->get<std::string>(); // get std::string result  
}

delegate->result()->clear(); // Clear result and free memory used by value
```

For signals, result will be saved only from last delegate call.
