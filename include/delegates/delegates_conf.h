
#ifndef DELEGAGES_CONF_HEADER
#define DELEGAGES_CONF_HEADER

// User can setup its own base namespace here
// example:
//    #define DELEGATES_BASE_NAMESPACE          mylib
//    #define DELEGATES_BASE_NAMESPACE_BEGIN    namespace mylib {
//    #define DELEGATES_BASE_NAMESPACE_END      }
//    #define USING_DELEGATES_BASE_NAMESPACE    using namespace mylib;
// Delegates will be accessible as mylib::delegates::xxx

#define DELEGATES_BASE_NAMESPACE
#define DELEGATES_BASE_NAMESPACE_BEGIN
#define DELEGATES_BASE_NAMESPACE_END
#define USING_DELEGATES_BASE_NAMESPACE

// Strict mode is used for debug checks: in this mode signals and delegates throws exceptions on all errors
#define DELEGATES_STRICT  0

// Trace mode: print message to cerr on errors
#define DELEGATES_TRACE   1

#endif //DELEGAGES_CONF_HEADER
