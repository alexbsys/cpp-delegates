# cpp-delegates

## What is difference between delegate and signal

**Delegate** means single call activity with stored arguments inside it, and saved result. Caller may not know parameters or result type, but he can call delegate:

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

Certainly, delegate definition, arguments specification, call and result checking can be performed in separated places from different threads, etc.
Also `IDelegate` is common virtual C++ interface struct without any template parameters: caller is not required to know the details (but can check them if necessary)

Delegates with different types, functions, arguments types and count, can be stored in single list and called without any internal information:

```c++
std::list<IDelegate*> calls;
calls.push_back(delegates::factory::make<int,const std::string&>([](const std::string& s)->int {
  std::cout << s;
  return 42;
}, "Arguments may be provided directly in declaration"));

calls.push_back(delegates::factory::make<void>([]() { std::cout << "some activity"; }));

int v;
calls.push_back(delegates::factory::make<bool>([&v]()->bool { v = 66; return true; }));

// ...

// call all delegates
for (auto& c : calls)  
  c->call();
```

**Signal** is "delegates aggregator": it implements IDelegate, but can hold more than one delegate inside, but with same arguments and return type.
When *Signal* is called, all delegates inside it will be called with same parameters.



## Features and limitations

* Delegate may call:
** Non-const class methods (class pointer may be raw pointer, `std::shared_ptr` or `std::weak_ptr`)
** Const class methods
** `std::function`
** lambdas
** C-style functions

* For signal: static object declaration C++ syntax can be used, or dependency injected pointers

```c++
using namespace delegates;

Signal<bool,int> static_signal;   // Static signal implemented ISignal interface too
std::shared_ptr<ISignal> shared_signal = factory::make_shared_signal<void,int>();
std::unique_ptr<ISignal> unique_signal = factory::make_unique_signal<void,int>();
ISignal* raw_signal = factory::make<void,int>();
```

* For delegates: raw pointers, shared pointers and unique pointers are provided by factory


* Non-const reference types inside signals and delegates are not allowed
```c++
Signal<void, std::string> signal0;  // OK
Signal<void, const std::string&> signal1; // OK, string can be passed
Signal<void, std::string&> signal2; // FAIL, compilation error

// If you need return parameters as arguments, please use pointers:
Signal<void, std::string*> signal3; // OK, delegate can change value through pointer
```