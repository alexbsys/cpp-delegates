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
