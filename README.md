# cpp-delegates

Universal C++ delegates and signals library for cross-thread execution and IPC/RPC

## Project Goals

The primary goal of this library is to **unify calls between threads and in IPC/RPC scenarios**. It enables:

* **Universal task execution**: Executors can work with `IDelegate*` pointers without knowing argument or result types
* **Type-safe convenience**: Users can use `TypedDelegate` for convenient direct calls: `delegate(42, "hello")`
* **Cross-thread/IPC safety**: Arguments are stored by value (references decayed) to ensure safe execution in different contexts
* **Serialization support**: Built-in serialization for IPC/RPC (JSON and binary formats)

The executor (thread pool, IPC handler) simply calls `delegate->call()` without needing to know call details, while the task enqueuer can execute delegates synchronously or asynchronously using either the convenient typed API or the universal untyped interface.

## Features

* C++14 standard, no dependencies (serialization is optional). Simple intuitive integration. No OS-specific code
* Headers-only. All headers can be concatenated to single file
* Pass arguments as values, references, const references, pointers, etc
* Slots can be: static functions, lambdas, `std::function`, class methods (regular and const)
* For class method, pointer to class can be raw pointer, `shared_ptr` or `weak_ptr`
* Dependency-injection interfaces are supported
* RTTI type checking (but library may work without RTTI support)
* Thread-safe
* **Automatic type deduction** for convenient delegate creation
* **Serialization support** (JSON via nlohmann/json, Binary via msgpack-c) for IPC/RPC

## Delegates and signals

**Delegate** means single call activity with stored arguments inside it, and saved result. Caller may not know parameters or result type, but he can call delegate via `IDelegate::call()`. This enables universal execution patterns where executors work with delegates without knowing their types.

**Signal** is "delegates aggregator": it implements `IDelegate` interface, but can hold more than one delegate inside, but with same arguments and return type.
When *Signal* is called, all delegates inside it will be called with same parameters.

## How to build

### Basic build (without serialization)

* In your own project: just copy directory `include/delegates` to your project includes dir and just include in C++: `#include <delegates/delegates.hpp>`
* Build examples and tests: run in the project directory
```bash
cmake -B build
cd build
make -j
```

### Build with serialization support

The library supports optional serialization backends for IPC/RPC scenarios:

* **JSON serialization** (via nlohmann/json)
* **Binary serialization** (via msgpack-c)

To enable serialization, configure CMake with:

```bash
# Enable JSON serialization
cmake -B build -DCPPDELEGATES_WITH_JSON_SERIALIZATION=ON

# Enable binary serialization
cmake -B build -DCPPDELEGATES_WITH_BINARY_SERIALIZATION=ON

# Enable both
cmake -B build \
  -DCPPDELEGATES_WITH_JSON_SERIALIZATION=ON \
  -DCPPDELEGATES_WITH_BINARY_SERIALIZATION=ON

cd build
make -j
```

Dependencies (nlohmann/json and msgpack-c) are automatically fetched via CMake `FetchContent`, so no manual installation is required.

## Supported platforms
Tested compilers:

* Microsoft Visual Studio
* gcc
* clang

Tested platforms:

* Windows
* Linux
* MacOS

## Usage examples

### Modern API: TypedDelegate with automatic type deduction

The recommended way to create delegates when types can be inferred:

```c++
#include <delegates/delegates.hpp>
#include <iostream>
#include <string>

using namespace delegates;

int main() {
  // Automatic type deduction - no need to specify types!
  auto delegate = factory::make_delegate_auto(
    [](int x, std::string s) -> int {
      std::cout << "x=" << x << ", s=" << s << std::endl;
      return x + static_cast<int>(s.length());
    }
  );
  
  // Direct call with arguments - convenient and type-safe
  int result = delegate(42, "hello");
  std::cout << "Result: " << result << std::endl;
  
  // Access to untyped interface for executors
  IDelegate* untyped = delegate.get_interface();
  untyped->args()->set<int>(0, 100);
  untyped->args()->set<std::string>(1, "world");
  untyped->call();
  int result2 = untyped->result()->get<int>();
  std::cout << "Result2: " << result2 << std::endl;
  
  return 0;
}
```

### TypedDelegate with explicit types (for reference control)

When you need explicit control over reference types:

```c++
#include <delegates/delegates.hpp>

using namespace delegates;

int main() {
  // Explicit types allow control over references
  auto delegate = factory::make_delegate<int, const std::string&>(
    [](const std::string& s) -> int {
      return static_cast<int>(s.length());
    }
  );
  
  std::string str = "test";
  int result = delegate(str);  // Passes by const reference
  return 0;
}
```

### TypedDelegate with class methods

```c++
#include <delegates/delegates.hpp>
#include <memory>

using namespace delegates;

class Calculator {
public:
  int add(int a, int b) { return a + b; }
  int multiply(int a, int b) const { return a * b; }
};

int main() {
  auto calc = std::make_shared<Calculator>();
  
  // Method delegate
  auto add_delegate = factory::make_delegate(calc, &Calculator::add);
  int sum = add_delegate(10, 20);  // Direct call
  
  // Const method delegate
  auto mult_delegate = factory::make_delegate(calc, &Calculator::multiply);
  int product = mult_delegate(5, 6);
  
  return 0;
}
```

### Traditional API: Low-level interface

For executors that work with delegates without knowing their types:

```c++
#include <delegates/delegates.hpp>
#include <iostream>
#include <string>
#include <memory>

using namespace std;
using namespace delegates;

int main() {
  // Create delegate with return type 'int' and one argument 'std::string'
  auto delegate = factory::make_unique<int,string>([](string s)->int {
    cout << s << endl;
    return 42;
  });

  delegate->args()->set<string>(0, "Hello world!"); // Set parameter #0 to string
  delegate->call();  // Perform call
  int ret = delegate->result()->get<int>(); // Get call return value
  cout << ret << endl;
  
  return 0;
}
```

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
  delegate->args()->set<std::string>(0, "Hello world!"); // Set parameter #0 to string

  delegate->call(); // Perform call
  int ret = delegate->result()->get<int>(); // Get call return value
  std::cout << ret << std::endl;
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

### Delegate with lambda call and reference arguments

```c++
#include <delegates/delegates.hpp>
#include <iostream>
#include <string>
#include <memory>

using namespace std;
using namespace delegates;

void main() {
  auto delegate = factory::make_unique<void,const string&,string&>([](const string& in, std::string& out) {
    if (in == "hello")
      out = "world";
    });

  delegate->args()->set<string>(0, "hello"); // Set parameter #0 to string
  delegate->call();  // Perform call
  std::string& out = delegate->args()->get_ref<std::string>(1);
  cout << out << endl;  // print "world"
}
```


## Features

### Delegates

Delegate definition, arguments specification, call and result checking can be performed in separated places from different threads, etc.
`IDelegate` is common virtual C++ interface struct without any template parameters: caller should not know details about arguments or return type (but can check them if necessary).

Delegates with different types, functions, arguments count and types, may be stored in single list and called without providing arguments/return information.
Delegate arguments may contain simple or complex type, reference, const reference, pointers.

```c++
// create delegate with lambda like function   int delegate(const std::string& s)
IDelegate* delegate = delegates::factory::make<int,const std::string&>([](const std::string& s)->int {
  std::cout << s;
  return 42;
});

delegate->args()->set<std::string>(0, "hello world"); // set argument 0 value separately from delegate function definition
delegate->call(); // call delegate separately from arguments specification
bool ret = delegate->result()->get<bool>(); // get result value
```

Delegates can be construct as raw pointers, shared or unique pointers:
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

Signals support static object declaration C++ syntax (`Signal<void> sig`), or dependency injected pointers (`std::shared_ptr<ISignal> sig = delegates::factory::make_shared_signal<void>(...)`):

```c++
using namespace delegates;

Signal<bool,int> static_signal;   // Static signal implemented ISignal interface too
std::shared_ptr<ISignal> shared_signal = factory::make_shared_signal<void,int>();
std::unique_ptr<ISignal> unique_signal = factory::make_unique_signal<void,int>();
ISignal* raw_signal = factory::make<void,int>();
```

Value types, pointers, const and non-const reference types inside signals and delegates are allowed:

```c++
Signal<void, std::string> signal0;  // OK
Signal<void, const std::string&> signal1; // OK, string can be passed
Signal<void, std::string&> signal2; // OK

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

DelegateArgs<> always owns values, but they are provided to delegates and signals as references.

High-level interface:
* Get arguments count
* Set argument value by index when user knowns argument type
* Get argument value by index when user knowns argument type
* Get reference to argument value inside DelegateArgs<> when user knowns agrument type
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

## Serialization for IPC/RPC

The library provides serialization support for cross-process communication. Two backends are available:

* **JSON serialization** (via nlohmann/json) - human-readable, good for debugging
* **Binary serialization** (via msgpack-c) - compact, efficient for production

### Supported types

Serializers support:
* Basic types: `int`, `long`, `float`, `double`, `bool`, `char`, `uint8_t`, `short`, `unsigned short`, `std::string`, `std::wstring`
* Containers: `std::vector<T>` and `std::list<T>` for basic types
* Custom types: Register your own types via `register_custom_type()`

### JSON Serialization Example

```c++
#ifdef DELEGATES_WITH_JSON_SERIALIZATION
#include <delegates/serialization/json_serializer.hpp>
#include <delegates/serialization/i_serializer.h>

using namespace delegates;

int main() {
  // Create delegate
  auto delegate = factory::make_delegate<int, std::string, int>(
    [](std::string s, int x) -> int {
      return static_cast<int>(s.length()) + x;
    }
  );
  
  // Set arguments and call
  delegate("hello", 10);
  
  // Serialize
  serialization::JsonSerializer json_serializer;
  serialization::DelegateSerializer serializer(&json_serializer);
  
  std::vector<uint8_t> serialized;
  if (serializer.serialize_args(delegate.get_interface(), serialized)) {
    std::cout << "Serialized to JSON: " << serialized.size() << " bytes" << std::endl;
    
    // Deserialize in another process/thread
    auto new_delegate = factory::make_delegate<int, std::string, int>(
      [](std::string s, int x) -> int {
        return static_cast<int>(s.length()) + x;
      }
    );
    
    size_t offset = 0;
    if (serializer.deserialize_args(new_delegate.get_interface(), serialized, offset)) {
      new_delegate.call();
      int result = new_delegate.get_result<int>();
      std::cout << "Deserialized and called, result: " << result << std::endl;
    }
  }
  
  return 0;
}
#endif
```

### Binary Serialization Example

```c++
#ifdef DELEGATES_WITH_BINARY_SERIALIZATION
#include <delegates/serialization/binary_serializer.hpp>
#include <delegates/serialization/i_serializer.h>

using namespace delegates;

int main() {
  // Create delegate
  auto delegate = factory::make_delegate<int, int, int>(
    [](int a, int b) -> int {
      return a + b;
    }
  );
  
  // Set arguments
  delegate(5, 7);
  
  // Serialize
  serialization::BinarySerializer binary_serializer;
  serialization::DelegateSerializer serializer(&binary_serializer);
  
  std::vector<uint8_t> serialized;
  if (serializer.serialize_args(delegate.get_interface(), serialized)) {
    std::cout << "Serialized to binary: " << serialized.size() << " bytes" << std::endl;
    
    // Deserialize
    auto new_delegate = factory::make_delegate<int, int, int>(
      [](int a, int b) -> int {
        return a + b;
      }
    );
    
    size_t offset = 0;
    if (serializer.deserialize_args(new_delegate.get_interface(), serialized, offset)) {
      new_delegate.call();
      int result = new_delegate.get_result<int>();
      std::cout << "Deserialized and called, result: " << result << std::endl;
    }
  }
  
  return 0;
}
#endif
```

### Custom Type Serialization

You can register custom types for serialization:

```c++
#ifdef DELEGATES_WITH_JSON_SERIALIZATION
#include <delegates/serialization/json_serializer.hpp>

struct MyCustomType {
  int value;
  std::string name;
};

int main() {
  serialization::JsonSerializer serializer;
  
  // Register custom type
  size_t type_hash = typeid(MyCustomType).hash_code();
  serializer.register_custom_type(
    type_hash,
    [](const void* ptr, std::vector<uint8_t>& output) -> bool {
      const MyCustomType* obj = static_cast<const MyCustomType*>(ptr);
      // Serialize to JSON
      nlohmann::json j;
      j["value"] = obj->value;
      j["name"] = obj->name;
      std::string json_str = j.dump();
      output.insert(output.end(), json_str.begin(), json_str.end());
      return true;
    },
    [](const std::vector<uint8_t>& input, size_t& offset, void* ptr) -> bool {
      MyCustomType* obj = static_cast<MyCustomType*>(ptr);
      // Deserialize from JSON
      std::string json_str(input.begin() + offset, input.end());
      nlohmann::json j = nlohmann::json::parse(json_str);
      obj->value = j["value"];
      obj->name = j["name"];
      offset = input.size();
      return true;
    }
  );
  
  return 0;
}
#endif
```

## Thread Pool / Executor Pattern

The library is designed for thread pool and executor patterns where tasks are enqueued and executed in different contexts:

```c++
#include <delegates/delegates.hpp>
#include <queue>
#include <thread>
#include <mutex>

using namespace delegates;

class TaskExecutor {
  std::queue<IDelegate*> task_queue_;
  std::mutex mutex_;
  bool running_ = true;
  
public:
  // Enqueuer: uses convenient TypedDelegate API
  template<typename Result, typename... Args>
  void enqueue(TypedDelegate<Result, Args...> delegate) {
    std::lock_guard<std::mutex> lock(mutex_);
    task_queue_.push(delegate.get_interface());
  }
  
  // Executor: works with untyped interface
  void worker_thread() {
    while (running_) {
      IDelegate* task = nullptr;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!task_queue_.empty()) {
          task = task_queue_.front();
          task_queue_.pop();
        }
      }
      
      if (task) {
        task->call();  // Executor doesn't need to know types!
        // Result can be retrieved via task->result()->get<T>()
      }
    }
  }
};

int main() {
  TaskExecutor executor;
  
  // Enqueue tasks with convenient API
  auto task1 = factory::make_delegate_auto([](int x) -> int { return x * 2; });
  executor.enqueue(task1);
  
  auto task2 = factory::make_delegate_auto([](std::string s) -> void {
    std::cout << s << std::endl;
  });
  executor.enqueue(task2);
  
  // Executor thread handles all tasks uniformly
  std::thread worker([&executor]() { executor.worker_thread(); });
  worker.join();
  
  return 0;
}
```
