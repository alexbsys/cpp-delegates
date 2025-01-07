
#include "delegates_tests.h"

#include <delegates/delegates.hpp>
#include <algorithm>
#include <thread>
#include <mutex>
#include <condition_variable>

#define DELEGATE_TESTS_WITH_EXCEPTIONS_ENABLED 1

USING_DELEGATES_BASE_NAMESPACE
using namespace DELEGATES_BASE_NAMESPACE::delegates;

TEST_F(DeferredCallTests, DelegateArgs_SimpleValues) {
  // with default arguments
  DelegateArgs<int, float> args1(std::nullptr_t{});
  ASSERT_EQ(args1.size(), 2);

  ASSERT_EQ(args1.get<int>(0), 0);
  ASSERT_EQ(args1.get<float>(1), 0.0f);

  ASSERT_TRUE(args1.set<int>(0, 123));
  ASSERT_TRUE(args1.set<float>(1, 1.23f));

  ASSERT_EQ(args1.get<int>(0), 123);
  ASSERT_EQ(args1.get<float>(1), 1.23f);

  ASSERT_EQ(args1.hash_code(0), typeid(int).hash_code());
  ASSERT_EQ(args1.hash_code(1), typeid(float).hash_code());

  args1.clear();

  ASSERT_EQ(args1.get<int>(0), 0);
  ASSERT_EQ(args1.get<float>(1), 0.0f);

  // with initial arguments values
  DelegateArgs<int, float> args2(5, 6.12f);
  ASSERT_EQ(args2.size(), 2);

  ASSERT_EQ(args2.get<int>(0), 5);
  ASSERT_EQ(args2.get<float>(1), 6.12f);

  ASSERT_TRUE(args2.set<int>(0, 123));
  ASSERT_TRUE(args2.set<float>(1, 1.23f));

  ASSERT_EQ(args2.get<int>(0), 123);
  ASSERT_EQ(args2.get<float>(1), 1.23f);

  args2.clear();

  ASSERT_EQ(args1.get<int>(0), 0);
  ASSERT_EQ(args1.get<float>(1), 0.0f);

  // with no arguments
  DelegateArgs<> args3;
  ASSERT_EQ(args3.size(), 0);

  // with no arguments
  DelegateArgs<> args4(std::nullptr_t{});
  ASSERT_EQ(args4.size(), 0);
}

TEST_F(DeferredCallTests, DelegateArgs_StringsVectors) {
  DelegateArgs<std::string, std::vector<int> > args1(std::nullptr_t{});
  ASSERT_EQ(args1.size(), 2);

  ASSERT_TRUE(args1.get<std::string>(0) == std::string());
  ASSERT_TRUE(args1.get<std::vector<int> >(1).size() == 0);

  ASSERT_TRUE(args1.set<std::string>(0, "hello"));
  ASSERT_TRUE(args1.set<std::vector<int> >(1, std::vector<int> { 1, 2 }));

  ASSERT_TRUE(args1.get<std::string>(0) == "hello");
  std::vector<int> ta = args1.get<std::vector<int> >(1);
  ASSERT_EQ(ta.size(), 2);
  ASSERT_EQ(ta[0], 1);
  ASSERT_EQ(ta[1], 2);

  ASSERT_EQ(args1.hash_code(0), typeid(std::string).hash_code());
  ASSERT_EQ(args1.hash_code(1), typeid(std::vector<int>).hash_code());

  args1.clear();

  ASSERT_TRUE(args1.get<std::string>(0) == std::string());
  ASSERT_TRUE(args1.get<std::vector<int> >(1).size() == 0);
}

TEST_F(DeferredCallTests, DelegateArgs_StringRef) {
  DelegateArgs<const std::string&> args1(std::nullptr_t{});
  ASSERT_EQ(args1.size(), 1);

  ASSERT_TRUE(args1.get<std::string>(0) == std::string());

  ASSERT_TRUE(args1.set<std::string>(0, "hello"));
  ASSERT_TRUE(args1.get<std::string>(0) == "hello");
}

TEST_F(DeferredCallTests, SignalArgs_StringConstRef) {
  Signal<bool, const std::string&> sig(DelegateArgs<const std::string&>(std::nullptr_t{}));
  ASSERT_EQ(sig.args()->size(), 1);

  ASSERT_TRUE(sig.args()->get<std::string>(0) == std::string());

  ASSERT_TRUE(sig.args()->set<std::string>(0, "hello"));
  ASSERT_TRUE(sig.args()->get<std::string>(0) == "hello");

  sig += factory::make_shared<bool, const std::string&>([](const std::string& s)->bool { return s == "hello"; });
  sig();

  ASSERT_TRUE(sig.result()->has_value());
  ASSERT_TRUE(sig.result()->get<bool>());
}

TEST_F(DeferredCallTests, SignalArgs_StringPtr) {
  Signal<void, std::string*> sig(DelegateArgs<std::string*>(std::nullptr_t{}));
  ASSERT_EQ(sig.args()->size(), 1);

  ASSERT_TRUE(sig.args()->get<std::string*>(0) == nullptr);

  std::string s = "hello";
  ASSERT_TRUE(sig.args()->set<std::string*>(0, &s));
  ASSERT_TRUE(sig.args()->get<std::string*>(0) == &s);

  sig += factory::make_shared<void, std::string*>([](std::string* s) { *s = "world"; });
  sig();

  ASSERT_TRUE(s == "world");
}

void test_fn(int* a, int* b, int* c) {
  ASSERT_TRUE(a != nullptr);
  ASSERT_TRUE(b != nullptr);
  ASSERT_TRUE(c != nullptr);

  ASSERT_TRUE(*a == 1);
  ASSERT_TRUE(*b == 2);
  ASSERT_TRUE(*c == 3);

  *a = 4;
  *b = 5;
  *c = 6;
}

TEST_F(DeferredCallTests, TestFunction) {
  int a = 1;
  int b = 2;
  int c = 3;
  int d = 3;
  IDelegate* call = delegates::factory::make_function_delegate(&test_fn, &a, &b, &c);
  ASSERT_TRUE(call != nullptr);
  ASSERT_TRUE(a == 1);

  ASSERT_EQ(3, call->args()->size());
  ASSERT_EQ(typeid(int*).hash_code(), call->args()->hash_code(0));
  ASSERT_EQ(typeid(int*).hash_code(), call->args()->hash_code(1));
  ASSERT_EQ(typeid(int*).hash_code(), call->args()->hash_code(2));

  int *pa = *reinterpret_cast<int**>(call->args()->get_ptr(0));
  ASSERT_EQ(pa, &a);

  int *pb = *reinterpret_cast<int**>(call->args()->get_ptr(1));
  ASSERT_EQ(pb, &b);

  int *pc = *reinterpret_cast<int**>(call->args()->get_ptr(2));
  ASSERT_EQ(pc, &c);

  bool ret = call->call();
  ASSERT_TRUE(ret);

  ASSERT_TRUE(a == 4);
  ASSERT_TRUE(b == 5);
  ASSERT_TRUE(c == 6);

  int* pd = &d;
  a = 1; b = 2; c = 3;
  call->args()->set_ptr(2, &pd, typeid(int*).hash_code());

  ret = call->call();
  ASSERT_TRUE(ret);

  ASSERT_EQ(3, c);
  ASSERT_TRUE(d == 6);

  delete call;
}

TEST_F(DeferredCallTests, TestLambda) {
  int a = 1;
  int b = 2;
  int c = 3;
  int sum = a+b+c;

  auto z = [&a,&b,&c]() -> int { int sum = a+b+c; a=4; b=5; c=6; return sum; };
  auto call = delegates::factory::make_lambda_delegate<int>(std::move(z));

  ASSERT_TRUE(call != nullptr);
  ASSERT_TRUE(a == 1);

  bool ret = call->call();
  ASSERT_TRUE(ret);

  int v = call->result()->get<int>();

  ASSERT_TRUE(a == 4);
  ASSERT_TRUE(b == 5);
  ASSERT_TRUE(c == 6);

  ASSERT_EQ(v, sum);

  delete call;

  auto call2 = delegates::factory::make_lambda_delegate<int,int,int>([](int a, int b) -> int { return a+b; }, 
    delegates::DelegateArgs<int,int>(4, 5));
  call2->call();
  v = call2->result()->get<int>();
  ASSERT_EQ(v, 9);
  delete call2;

  auto call5 = delegates::factory::make<int, int, int>([](int a, int b) -> int { return a + b; },
    4, 5);
  call5->call();
  v = call5->result()->get<int>();
  ASSERT_EQ(v, 9);
  delete call5;



  a = 1; b=2;
  auto call3 = delegates::factory::make_lambda_delegate<int>([&a,&b]() -> int { return a+b; });
  call3->call();
  v = call3->result()->get<int>();
  ASSERT_EQ(v, 3);
  delete call3;

  auto call4 = delegates::factory::make_lambda_delegate([&a,&b]() { a = 6; b=6; });
  call4->call();
  ASSERT_FALSE(call4->result()->has_value());
  ASSERT_EQ(b, 6);
  delete call4;
}

TEST_F(DeferredCallTests, TestClassMemberCall) {
  static const std::string kTestValue = "hello";

  struct TestClass {
    int Method(const std::string& str) {
      if (str == kTestValue) {
        calls_++;
        return 42;
      }

      return -1;
    }
    int calls_ = 0;
  };

  TestClass test_class;

  IDelegate* delegate = delegates::factory::make_method_delegate(&test_class, &TestClass::Method);
  delegate->args()->set<std::string>(0, kTestValue);

  bool r = delegate->call();

  ASSERT_TRUE(r);
  ASSERT_TRUE(test_class.calls_ == 1);
  ASSERT_TRUE(delegate->result()->has_value());
  ASSERT_EQ(delegate->result()->get<int>(), 42);
}


TEST_F(DeferredCallTests, TestClassConstMethodCall) {
  static const std::string kTestValue = "hello";

  struct TestClass {
    int Method(const std::string& str) const {
      if (str == kTestValue)
        return 42;

      return -1;
    }
  };

  TestClass test_class;

  IDelegate* delegate = delegates::factory::make_const_method_delegate(&test_class, &TestClass::Method);
  delegate->args()->set<std::string>(0, kTestValue);

  bool r = delegate->call();

  ASSERT_TRUE(r);
  ASSERT_TRUE(delegate->result()->has_value());
  ASSERT_EQ(delegate->result()->get<int>(), 42);
}

TEST_F(DeferredCallTests, TestClassSharedPtrMemberCall) {
  static const std::string kTestValue = "hello";

  struct TestClass {
    int Method(const std::string& str) {
      if (str == kTestValue) {
        calls_++;
        return 42;
      }

      return -1;
    }
    int calls_ = 0;
  };

  std::shared_ptr<TestClass> test_class_ptr = std::make_shared<TestClass>();

  IDelegate* delegate = delegates::factory::make_method_delegate(test_class_ptr, &TestClass::Method);
  delegate->args()->set<std::string>(0, kTestValue);

  bool r = delegate->call();

  ASSERT_TRUE(r);
  ASSERT_TRUE(test_class_ptr->calls_ == 1);
  ASSERT_TRUE(delegate->result()->has_value());
  ASSERT_EQ(delegate->result()->get<int>(), 42);
}

TEST_F(DeferredCallTests, TestClassWeakPtrMethodCallNonVoidResult) {
  static const std::string kTestValue = "hello";

  struct TestClass {
    int Method(const std::string& str) {
      if (str == kTestValue) {
        calls_++;
        return 42;
      }

      return -1;
    }
    int calls_ = 0;
  };

  std::shared_ptr<TestClass> test_class_ptr = std::make_shared<TestClass>();
  std::weak_ptr<TestClass> test_class_weak = test_class_ptr;

  IDelegate* delegate = delegates::factory::make_method_delegate(test_class_weak, &TestClass::Method);
  delegate->args()->set<std::string>(0, kTestValue);

  bool r = delegate->call();

  ASSERT_TRUE(r);
  ASSERT_TRUE(test_class_ptr->calls_ == 1);
  ASSERT_TRUE(delegate->result()->has_value());
  ASSERT_EQ(delegate->result()->get<int>(), 42);

  test_class_ptr.reset();

  r = delegate->call();
  ASSERT_FALSE(r);
}

TEST_F(DeferredCallTests, TestClassWeakPtrMethodCallVoidResult) {
  static const std::string kTestValue = "hello";

  struct TestClass {
    void Method(const std::string& str, int& ret) {
      if (str == kTestValue) {
        calls_++;
        ret = 42;
        return;
      }

      ret = -1;
    }
    int calls_ = 0;
  };

  std::shared_ptr<TestClass> test_class_ptr = std::make_shared<TestClass>();
  std::weak_ptr<TestClass> test_class_weak = test_class_ptr;

  int ret = 0;
  IDelegate* delegate = delegates::factory::make_method_delegate<TestClass, void, const std::string&, int&>(
    test_class_weak, &TestClass::Method, kTestValue, ret);

  bool r = delegate->call();

  ASSERT_TRUE(r);
  ASSERT_TRUE(test_class_ptr->calls_ == 1);
  ASSERT_FALSE(delegate->result()->has_value());
  ASSERT_EQ(ret, 42);

  test_class_ptr.reset();

  r = delegate->call();
  ASSERT_FALSE(r);
}

TEST_F(DeferredCallTests, TestClassSharedPtrConstMethodCall) {
  static const std::string kTestValue = "hello";

  struct TestClass {
    int Method(const std::string& str) const {
      if (str == kTestValue)
        return 42;

      return -1;
    }
  };

  std::shared_ptr<TestClass> test_class_ptr = std::make_shared<TestClass>();

  IDelegate* delegate = delegates::factory::make_const_method_delegate(test_class_ptr, &TestClass::Method);
  delegate->args()->set<std::string>(0, kTestValue);

  bool r = delegate->call();

  ASSERT_TRUE(r);
  ASSERT_TRUE(delegate->result()->has_value());
  ASSERT_EQ(delegate->result()->get<int>(), 42);
}

TEST_F(DeferredCallTests, TestLambda_EmptyArgs_SetVectorArg) {
  int a = 1;
  std::vector<int> b = { 2, 3 };

  auto call2 = delegates::factory::make_lambda_delegate<int, int, std::vector<int> >(
    [](int a, const std::vector<int>& b) -> int { return a + b[0] + b[1]; });

  call2->args()->set(0, a);
  call2->args()->set(1, b);

  call2->call();

  int v = call2->result()->get<int>();
  ASSERT_EQ(v, 6);
  delete call2;
}

TEST_F(DeferredCallTests, TestLambda_ReplaceConstRefArgument) {
  int a = 1;
  std::vector<int> b = { 2, 3 };
  std::vector<int> c = { 4, 5 };

  auto call = delegates::factory::make_lambda_delegate<int, int, const std::vector<int>& >(
    [](int a, const std::vector<int>& b) -> int { return a + b[0] + b[1]; }, 
    delegates::DelegateArgsValues<int, const std::vector<int>& >(a, b));

  call->args()->set(0, a);
  call->args()->set(1, c);

  ASSERT_FALSE(call->result()->has_value());

  call->call();

  ASSERT_TRUE(call->result()->has_value());

  int v = call->result()->get<int>();
  ASSERT_EQ(v, 10);
  delete call;
}

TEST_F(DeferredCallTests, TestLambda_CallArgDeleter) {
  class Arg {
   public:
    ~Arg() { if (deleted_) *deleted_ = 1; }
    int called_ = 0;
    int* deleted_ = nullptr;
  };

  int deleted_a = 0;
  int deleted_b = 0;

  auto call = delegates::factory::make_lambda_delegate<void, Arg*>([](Arg* a) { a->called_++; });

  Arg* pa = new Arg();
  pa->deleted_ = &deleted_a;

  Arg* pb = new Arg();
  pb->deleted_ = &deleted_b;

  call->args()->set<Arg*>(0, pa, [](Arg* a) { delete a; });

  ASSERT_EQ(deleted_a, 0);
  ASSERT_EQ(deleted_b, 0);

  call->args()->set<Arg*>(0, pb, [](Arg* b) { delete b; });

  ASSERT_EQ(deleted_a, 1);

  call->call();

  ASSERT_EQ(deleted_b, 0);
  ASSERT_EQ(pb->called_, 1);
  delete call;

  ASSERT_EQ(deleted_b, 1);
}


TEST_F(DeferredCallTests, TestLambda_SetWrongArgType) {
  char a = 1;
  std::vector<char> b = { 2, 3 };

  auto call2 = delegates::factory::make_lambda_delegate<int, int, std::vector<int> >(
    [](int a, const std::vector<int>& b) -> int { return a + b[0] + b[1]; });

  bool r = call2->args()->set(0, a);
  ASSERT_FALSE(r);

  r = call2->args()->set(1, b);
  ASSERT_FALSE(r);

  r = call2->args()->try_get(0, a);
  ASSERT_FALSE(r);

  int v = 1234;
  r = call2->args()->try_get(0, v);
  ASSERT_TRUE(r);
  ASSERT_EQ(v, int());

  delete call2;
}

#if DELEGATE_TESTS_WITH_EXCEPTIONS_ENABLED
TEST_F(DeferredCallTests, TestLambda_SetWrongArgIdx) {
  char a = 1;
  int b = 2;

  auto call2 = delegates::factory::make_lambda_delegate<int, char, int>(
    [](char a, int b) -> int { return a + b; });

  try {
    call2->args()->set<char>(2, a);
    FAIL();
  } catch(std::exception&) {
  }

  try {
    call2->args()->get<int>(3);
    FAIL();
  } catch(std::exception&) {
  }

  try {
    call2->args()->hash_code(3);
    FAIL();
  } catch(std::exception&) {
  }

  delete call2;
}
#endif //DELEGATE_TESTS_WITH_EXCEPTIONS_ENABLED


TEST_F(DeferredCallTests, TestLambda_CallFromDifferentThread) {
  std::shared_ptr<delegates::IDelegate> call;
  std::condition_variable notifier1;
  std::condition_variable notifier2;
  std::mutex mutex;

  bool called = false;
  bool checked = false;
  bool result = false;

  auto thread = std::thread([&call, &notifier1, &notifier2, &mutex, &called, &checked, &result]() {
    int a = 1;
    int b = 2;
    int c = 3;
    int sum = a+b+c;

    auto z = [&a,&b,&c]() -> int { int sum = a+b+c; a=4; b=5; c=6; return sum; };
    call = delegates::factory::make_shared<int>(std::move(z));

    {
      std::lock_guard<std::mutex> lock(mutex);
      notifier1.notify_all();
    }
    
    while (!called) {
      std::unique_lock<std::mutex> lock(mutex);
      notifier2.wait_for(lock, std::chrono::milliseconds(100));
    }

    if (a==4 && b==5 && c==6)
      result = true;

    checked = true;
    {
      std::lock_guard<std::mutex> lock(mutex);
      notifier1.notify_all();
    }
  });

  while(!call) {
    std::unique_lock<std::mutex> lock(mutex);
    notifier1.wait_for(lock, std::chrono::milliseconds(100));
  }

  bool r = call->call();
  ASSERT_TRUE(r);

  called = true;
  {
    std::lock_guard<std::mutex> lock(mutex);
    notifier2.notify_all();
  }

  while(!checked) {
    std::unique_lock<std::mutex> lock(mutex);
    notifier1.wait_for(lock, std::chrono::milliseconds(100));
  }

  int v = call->result()->get<int>();
  ASSERT_EQ(v, 6);

  call.reset();
  thread.join();
}

TEST_F(DeferredCallTests, TestDelegates_MultiCalls_VoidResult) {
  auto delegates = delegates::factory::make_shared_signal<void, int, std::string>();

  int r1i = 0;
  std::string r1s;

  int r2i = 0;
  std::string r2s;

  int r3i = 0;
  std::string r3s;

  auto call1 = delegates::factory::make_lambda_delegate<void, int, std::string>([&r1i, &r1s](int i, std::string s) { r1i = i; r1s = s; });
  auto call2 = delegates::factory::make_lambda_delegate<void, int, std::string>([&r2i, &r2s](int i, std::string s) { r2i = i; r2s = s; });
  std::shared_ptr<IDelegate> call3 = delegates::factory::make_shared<void, int, std::string>([&r3i, &r3s](int i, std::string s) { r3i = i; r3s = s; });

  ASSERT_EQ(delegates->args()->size(), 2);
  ASSERT_EQ(call1->args()->size(), 2);
  ASSERT_EQ(call2->args()->size(), 2);
  ASSERT_EQ(call3->args()->size(), 2);

  delegates->add(call1, "", [](IDelegate* c) { delete c; });
  delegates->add(call2, "call2", [](IDelegate* c) { delete c; });
  delegates->add(call3, "call3");

  int v = 42;
  delegates->args()->set<int>(0, v);

  delegates->args()->set<std::string>(1, std::string("hello"));

  bool ret = delegates->call();
  ASSERT_TRUE(ret);

  ASSERT_EQ(r1i, 42);
  ASSERT_TRUE(r1s == "hello");

  ASSERT_EQ(r2i, 42);
  ASSERT_TRUE(r2s == "hello");

  ASSERT_EQ(r3i, 42);
  ASSERT_TRUE(r3s == "hello");
}

int g_delegates_multicalls_test_result_instances = 0;

TEST_F(DeferredCallTests, TestDelegates_MultiCalls_RefParamsWithResult) {
  g_delegates_multicalls_test_result_instances = 0;
  struct TestResult {
    TestResult() { g_delegates_multicalls_test_result_instances++; }
    TestResult(int ret) : ret_(ret) { g_delegates_multicalls_test_result_instances++; }
    TestResult(const TestResult& other) {
      ret_ = other.ret_;
      g_delegates_multicalls_test_result_instances++;
    }
    ~TestResult() { g_delegates_multicalls_test_result_instances--; }

    int ret_ = 0;
  };

  static const std::string& kTestValue = "hello";
  std::string kEmptyValue;
  auto delegates = delegates::factory::make_shared_signal<TestResult, int, const std::string&>(42, kTestValue);

  int r1i = 0;
  bool r1s = false;

  int r2i = 0;
  bool r2s = false;

  int r3i = 0;
  bool r3s = false;

  auto call1 = delegates::factory::make_lambda_delegate<TestResult, int, const std::string&>(
    [&r1i, &r1s](int i, const std::string& s)->TestResult {
      r1i = i;
      r1s = s == kTestValue;
      return TestResult { 1 };
    });

  auto call2 = delegates::factory::make_lambda_delegate<TestResult, int, const std::string&>(
    [&r2i, &r2s](int i, const std::string& s)->TestResult {
      r2i = i;
      r2s = s == kTestValue;
      return TestResult { 2 };
    },
    delegates::DelegateArgsValues<int, const std::string&>(0, kEmptyValue));

  std::shared_ptr<IDelegate> call3 = delegates::factory::make_shared<TestResult, int, const std::string&>(
    [&r3i, &r3s](int i, std::string s)->TestResult {
      r3i = i;
      r3s = s == kTestValue;
      return TestResult { 3 };
    },
    0, kEmptyValue);

  ASSERT_EQ(delegates->args()->size(), 2);
  ASSERT_EQ(call1->args()->size(), 2);
  ASSERT_EQ(call2->args()->size(), 2);
  ASSERT_EQ(call3->args()->size(), 2);

  delegates->add(call1, std::string(), [](IDelegate* c) { delete c; });
  delegates->add(call2, "call2", [](IDelegate* c) { delete c; });
  delegates->add(call3, "call3");

  bool ret = delegates->call();
  ASSERT_TRUE(ret);

  ASSERT_EQ(r1i, 42);
  ASSERT_TRUE(r1s);

  ASSERT_EQ(r2i, 42);
  ASSERT_TRUE(r2s);

  ASSERT_EQ(r3i, 42);
  ASSERT_TRUE(r3s);

  {
    ASSERT_TRUE(delegates->result()->has_value());
    TestResult check_r = delegates->result()->get<TestResult>();
    ASSERT_EQ(check_r.ret_, 3);
  }

  delegates.reset();
  call3.reset();

  ASSERT_EQ(g_delegates_multicalls_test_result_instances, 0);
}

TEST_F(DeferredCallTests, TestDelegates_MultiCalls_RefParamsWithRefResult) {
  g_delegates_multicalls_test_result_instances = 0;
  struct TestResult {
    TestResult() { g_delegates_multicalls_test_result_instances++; }
    TestResult(const TestResult& other) {
      ret_ = other.ret_;
      g_delegates_multicalls_test_result_instances++;
    }

      TestResult(int ret) : ret_(ret) { g_delegates_multicalls_test_result_instances++; }
    ~TestResult() { g_delegates_multicalls_test_result_instances--; }
    int ret_ = 0;
  };

  TestResult result { 42 };

  static const std::string& kTestValue = "hello";
  auto sig = delegates::factory::make_unique_signal<const TestResult&, int, const std::string&>();

  int r1i = 0;
  bool r1s = false;

  int r2i = 0;
  bool r2s = false;

  auto call1 = delegates::factory::make_unique_lambda_delegate<const TestResult&, int, const std::string&>(
    [&r1i, &r1s, &result](int i, const std::string& s)->const TestResult& {
      r1i = i;
      r1s = s == kTestValue;
      return result;
    });

  std::shared_ptr<IDelegate> call2 = delegates::factory::make_shared<const TestResult&, int, const std::string&>(
    [&r2i, &r2s, &result](int i, std::string s)->const TestResult& {
      r2i = i;
      r2s = s == kTestValue;
      return result;
    });

  ASSERT_EQ(sig->args()->size(), 2);
  ASSERT_EQ(call1->args()->size(), 2);
  ASSERT_EQ(call2->args()->size(), 2);

  sig->add(call1.get());
  sig->add(call2);

  sig->args()->set<int>(0, 42);
  std::string vs = kTestValue;
  sig->args()->set<std::string>(1, vs);

  bool ret = sig->call();
  ASSERT_TRUE(ret);

  ASSERT_EQ(r1i, 42);
  ASSERT_TRUE(r1s);

  ASSERT_EQ(r2i, 42);
  ASSERT_TRUE(r2s);

  {
    ASSERT_TRUE(sig->result()->has_value());

    const TestResult& check_r = sig->result()->get<const TestResult&>();
    ASSERT_EQ(check_r.ret_, 42);

    TestResult check_r2 = sig->result()->get<TestResult>();
    ASSERT_EQ(check_r2.ret_, 42);
  }

  sig.reset();
  call1.reset();
  call2.reset();

  ASSERT_EQ(g_delegates_multicalls_test_result_instances, 1);
}

TEST_F(DeferredCallTests, TestDelegates_SignalCalls_Remove) {
  static const std::string& kTestValue = "hello";
  auto sig = delegates::factory::make_unique_signal<void, int, const std::string&>();

  int r1i = 0;
  bool r1s = false;

  int r2i = 0;
  bool r2s = false;

  auto call1 = delegates::factory::make_unique_lambda_delegate<void, int, const std::string&>(
    [&r1i, &r1s](int i, const std::string& s) {
      r1i = i;
      r1s = s == kTestValue;
    });

  std::shared_ptr<IDelegate> call2 = delegates::factory::make_shared<void, int, const std::string&>(
    [&r2i, &r2s](int i, std::string s) {
      r2i = i;
      r2s = s == kTestValue;
    });

  // add both calls without tags
  sig->add(call1.get());
  sig->add(call2);

  sig->args()->set<int>(0, 42);
  std::string vs = kTestValue;
  sig->args()->set<std::string>(1, vs);

  // remove call2 by shared ptr
  sig->remove(call2);

  bool ret = sig->call();
  ASSERT_TRUE(ret);

  ASSERT_EQ(r1i, 42);
  ASSERT_TRUE(r1s);

  // check that call2 was not called
  ASSERT_EQ(r2i, 0);
  ASSERT_FALSE(r2s);

  r1i = 0; r1s = false;

  // remove call1 by raw ptr
  sig->remove(call1.get());

  sig->call();

  // check that call1 and call2 were not called
  ASSERT_EQ(r1i, 0);
  ASSERT_FALSE(r1s);
  ASSERT_EQ(r2i, 0);
  ASSERT_FALSE(r2s);

  // add both calls with tags
  sig->add(call1.get(), "call1");
  sig->add(call2, "call2");

  // check that both were called
  ret = sig->call();
  ASSERT_TRUE(ret);

  ASSERT_EQ(r1i, 42);
  ASSERT_TRUE(r1s);
  ASSERT_EQ(r2i, 42);
  ASSERT_TRUE(r2s);

  // remove signal1 by tag
  sig->remove("call1");

  r1i = 0; r1s = false;
  r2i = 0; r2s = false;

  sig->call();

  ASSERT_EQ(r1i, 0);
  ASSERT_FALSE(r1s);
  ASSERT_EQ(r2i, 42);
  ASSERT_TRUE(r2s);

  // add call1 with same tag
  sig->add(call1.get(), "call2");

  r1i = 0; r1s = false;
  r2i = 0; r2s = false;

  sig->call();

  ASSERT_EQ(r1i, 42);
  ASSERT_TRUE(r1s);
  ASSERT_EQ(r2i, 42);
  ASSERT_TRUE(r2s);

  // remove calls by tag: both calls must be removed because they have same tag
  r1i = 0; r1s = false;
  r2i = 0; r2s = false;

  sig->remove("call2");

  sig->call();

  ASSERT_EQ(r1i, 0);
  ASSERT_FALSE(r1s);
  ASSERT_EQ(r2i, 0);
  ASSERT_FALSE(r2s);

  // add both calls and check remove_all
  sig->add(call1.get());
  sig->add(call2);
  sig->remove_all();

  sig->call();

  ASSERT_EQ(r1i, 0);
  ASSERT_FALSE(r1s);
  ASSERT_EQ(r2i, 0);
  ASSERT_FALSE(r2s);

  sig.reset();
  call1.reset();
  call2.reset();
}
