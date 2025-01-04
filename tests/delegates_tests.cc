
#include "delegates_tests.h"

#include "../include/delegate.hpp"
//#include "notifier_impl.h"
//#include "multithread.h"

#include <algorithm>
//#include <tools/i_thread.h>
//#include "thread_impl.h"

#define DELEGATE_TESTS_WITH_EXCEPTIONS_ENABLED 0


using namespace delegates;

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

TEST_F(DeferredCallTests, Test1) {
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

TEST_F(DeferredCallTests, TestLambda1) {
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


  auto call2 = delegates::factory::make_lambda_delegate<int,int,int>([](int a, int b) -> int { return a+b; }, 4, 5);
  call2->call();
  v = call2->result()->get<int>();
  ASSERT_EQ(v, 9);
  delete call2;

  a = 1; b=2;
  auto call3 = delegates::factory::make_lambda_delegate<int>([&a,&b]() -> int { return a+b; });
  call3->call();
  v = call3->result()->get<int>();
  ASSERT_EQ(v, 3);
  delete call3;

  auto call4 = delegates::factory::make_lambda_delegate([&a,&b]() { a = 6; b=6; });
  call4->call();
//  v = call3->Result()->GetValue<int>();
  ASSERT_EQ(b, 6);
  delete call4;
}

TEST_F(DeferredCallTests, TestLambda_EmptyArgs_SetVectorArg) {
  int a = 1;
  std::vector<int> b = { 2, 3 };

  auto call2 = delegates::factory::make_lambda_delegate<int, int, std::vector<int> >(
    [](int a, const std::vector<int>& b) -> int { return a + b[0] + b[1]; }, std::nullptr_t{});

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
    [](int a, const std::vector<int>& b) -> int { return a + b[0] + b[1]; }, a, b);

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

  auto call = delegates::factory::make_lambda_delegate<void, Arg*>([](Arg* a) { a->called_++; }, std::nullptr_t{});

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
    [](int a, const std::vector<int>& b) -> int { return a + b[0] + b[1]; }, std::nullptr_t{});

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

  auto call2 = DeferredCallFactory::CreateLambdaDeferredCall<int, char, int>(
    [](char a, int b) -> int { return a + b; }, delegate::Empty{});

  try {
    call2->GetParams()->SetArgValue<char>(2, a);
    FAIL();
  } catch(std::exception&) {
  }

  try {
    call2->GetParams()->GetArgValue<int>(3);
    FAIL();
  } catch(std::exception&) {
  }

  try {
    call2->GetParams()->GetArgTypeHash(3);
    FAIL();
  } catch(std::exception&) {
  }

  delete call2;
}
#endif //DELEGATE_TESTS_WITH_EXCEPTIONS_ENABLED

/*
TEST_F(DeferredCallTests, TestLambda_CallFromDifferentThread) {
  std::shared_ptr<delegate::IDelegate> call;
  std::shared_ptr<delegate::INotifier> notifier1 = delegate::CreateNotifier();
  std::shared_ptr<delegate::INotifier> notifier2 = delegate::CreateNotifier();
  bool called = false;
  bool checked = false;
  bool result = false;

  auto thread = CreateThread([&call, notifier1, notifier2, &called, &checked, &result]()->bool {
    int a = 1;
    int b = 2;
    int c = 3;
    int sum = a+b+c;

    auto z = [&a,&b,&c]() -> int { int sum = a+b+c; a=4; b=5; c=6; return sum; };
    call = delegate_factory::make_shared<int>(std::move(z));

    notifier1->Notify();
    while (!called) {
      notifier2->Wait(100);
    }

    if (a==4 && b==5 && c==6)
      result = true;

    checked = true;
    notifier1->Notify();
    return false;
  });

  thread->StartThread();
  while(!call) {
    notifier1->Wait(100);
  }

  bool r = call->call();
  ASSERT_TRUE(r);

  called = true;
  notifier2->Notify();

  while(!checked) {
    notifier1->Wait(100);
  }

  int v = call->result()->get<int>();
  ASSERT_EQ(v, 6);

  call.reset();
  r = thread->Join(1000);
  ASSERT_TRUE(r);
}
*/

TEST_F(DeferredCallTests, TestDelegates_MultiCalls_VoidResult) {
  auto delegates = delegates::factory::make_shared_multidelegate<void, int, std::string>(std::nullptr_t{});

  int r1i = 0;
  std::string r1s;

  int r2i = 0;
  std::string r2s;

  int r3i = 0;
  std::string r3s;

  auto call1 = delegates::factory::make_lambda_delegate<void, int, std::string>([&r1i, &r1s](int i, std::string s) { r1i = i; r1s = s; }, std::nullptr_t{});
  auto call2 = delegates::factory::make_lambda_delegate<void, int, std::string>([&r2i, &r2s](int i, std::string s) { r2i = i; r2s = s; }, std::nullptr_t{});
  std::shared_ptr<IDelegate> call3 = delegates::factory::make_shared<void, int, std::string>([&r3i, &r3s](int i, std::string s) { r3i = i; r3s = s; }, std::nullptr_t{});

  ASSERT_EQ(delegates->args()->size(), 2);
  ASSERT_EQ(call1->args()->size(), 2);
  ASSERT_EQ(call2->args()->size(), 2);
  ASSERT_EQ(call3->args()->size(), 2);

  delegates->add(call1, "", [](IDelegate* c) { delete c; });
  delegates->add(call2, "call2", [](IDelegate* c) { delete c; });
  delegates->add(call3, "call3");

  int v = 42;
  delegates->args()->set<int>(0, v);

//  std::string vs = ;
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
  auto delegates = delegates::factory::make_shared_multidelegate<TestResult, int, const std::string&>(42, kTestValue);

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
    },
    std::nullptr_t{});

  auto call2 = delegates::factory::make_lambda_delegate<TestResult, int, const std::string&>(
    [&r2i, &r2s](int i, std::string s)->TestResult {
      r2i = i;
      r2s = s == kTestValue;
      return TestResult { 2 };
    },
    0, kEmptyValue);

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

  delegates->add(call1, "", [](IDelegate* c) { delete c; });
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
   private:

  };

  TestResult result { 42 };

  static const std::string& kTestValue = "hello";
  auto multidelegate = delegates::factory::make_unique_multidelegate<const TestResult&, int, const std::string&>(std::nullptr_t{});

  int r1i = 0;
  bool r1s = false;

  int r3i = 0;
  bool r3s = false;

  auto call1 = delegates::factory::make_unique_lambda_delegate<const TestResult&, int, const std::string&>(
    [&r1i, &r1s, &result](int i, const std::string& s)->const TestResult& {
      r1i = i;
      r1s = s == kTestValue;
      return result;
    },
    std::nullptr_t{});

  std::shared_ptr<IDelegate> call2 = delegates::factory::make_shared<const TestResult&, int, const std::string&>(
    [&r3i, &r3s, &result](int i, std::string s)->const TestResult& {
      r3i = i;
      r3s = s == kTestValue;
      return result;
    },
    std::nullptr_t{});

  ASSERT_EQ(multidelegate->args()->size(), 2);
  ASSERT_EQ(call1->args()->size(), 2);
  ASSERT_EQ(call2->args()->size(), 2);

  multidelegate->add(call1.get());
  multidelegate->add(call2);

  multidelegate->args()->set<int>(0, 42);
  std::string vs = kTestValue;
  multidelegate->args()->set<std::string>(1, vs);

  bool ret = multidelegate->call();
  ASSERT_TRUE(ret);

  ASSERT_EQ(r1i, 42);
  ASSERT_TRUE(r1s);

  ASSERT_EQ(r3i, 42);
  ASSERT_TRUE(r3s);

  {
    ASSERT_TRUE(multidelegate->result()->has_value());

    const TestResult& check_r = multidelegate->result()->get<const TestResult&>();
    ASSERT_EQ(check_r.ret_, 42);

    TestResult check_r2 = multidelegate->result()->get<TestResult>();
    ASSERT_EQ(check_r2.ret_, 42);
  }

  multidelegate.reset();
  call1.reset();
  call2.reset();

  ASSERT_EQ(g_delegates_multicalls_test_result_instances, 1);
}
