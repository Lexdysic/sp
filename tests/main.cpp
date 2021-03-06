// sp - string formatting micro-library
//
// Written in 2017 by Johan Sköld
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to the public
// domain worldwide. This software is distributed without any warranty.
//
// You should have received a copy of the CC0 Public Domain Dedication along
// with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.

#include <cfloat> // DBL_MAX, FLT_MIN, FLT_MAX
#include <cstdio> // std::printf, fmemopen
#include <cstdlib> // std::malloc, std::free
#include <string> // std::string

#include "../include/sp.hpp"

static const char* s_testCaseDescr = nullptr;
static int s_failed = 0;

#if defined(_MSC_VER)
#include <Windows.h>
#define DEBUG_BREAK() ((void)(IsDebuggerPresent() ? DebugBreak() : 0))
#else
#define DEBUG_BREAK()
#endif

#define TEST_CASE(descr)           \
    for (;;) {                     \
        s_testCaseDescr = (descr); \
        break;                     \
    }

#define REQUIRE(expr)                                           \
    for (; !(expr);) {                                          \
        std::printf("Test failed!\n");                          \
        std::printf("- Test case: %s\n", s_testCaseDescr);      \
        std::printf("- Location: %s:%d\n", __FILE__, __LINE__); \
        std::printf("- Expression: %s\n", #expr);               \
        std::printf("\n");                                      \
        DEBUG_BREAK();                                          \
        ++s_failed;                                             \
        break;                                                  \
    }

#define TEST_FORMAT(expected, fmt, ...)                                                  \
    for (;;) {                                                                           \
        auto buffer = (char*)std::malloc(10 * 1024 * 1024);                              \
        buffer[0] = 0;                                                                   \
        const auto expectedLen = int32_t(std::strlen(expected));                         \
        const auto actualLen = sp::format(buffer, 10 * 1024 * 1024, fmt, ##__VA_ARGS__); \
        REQUIRE(std::memcmp(expected, buffer, actualLen) == 0);                          \
        REQUIRE(expectedLen == actualLen);                                               \
        std::free(buffer);                                                               \
        break;                                                                           \
    }

struct Foo {
};

static bool format_value(sp::IWriter& writer, const sp::StringView& format, const Foo&)
{
    if (!format.length) {
        writer.write(7, "<empty>");
    } else {
        writer.write(format.length, format.ptr);
    }
    return true;
}

int main()
{
    TEST_CASE("Output with a string buffer")
    {
        // it should result in the length of the written string
        {
            char buffer[64];
            sp::StringWriter writer(buffer, sizeof(buffer));
            writer.write(3, "foo");
            REQUIRE(writer.result() == 3);
            sp::write_char(writer, 'd');
            REQUIRE(writer.result() == 4);
        }

        // it should not overflow
        {
            char buffer[6] = { -1, -1, -1, -1, -1, -1 };

            // Lie about the length so we can make sure it doesn't overflow
            sp::StringWriter writer(buffer, 4);
            writer.write(6, "foobar");

            REQUIRE(buffer[0] == 'f');
            REQUIRE(buffer[1] == 'o');
            REQUIRE(buffer[2] == 'o');
            REQUIRE(buffer[3] == 'b');
            REQUIRE(buffer[4] == -1);
            REQUIRE(buffer[5] == -1);
        }

        // it should result in the length of the full string, even when truncated
        {
            char buffer[5];

            sp::StringWriter writer(buffer, sizeof(buffer));
            writer.write(10, "ooga booga");

            REQUIRE(buffer[0] == 'o');
            REQUIRE(buffer[1] == 'o');
            REQUIRE(buffer[2] == 'g');
            REQUIRE(buffer[3] == 'a');
            REQUIRE(buffer[4] == ' ');

            REQUIRE(writer.result() == 10);
        }
    }

    TEST_CASE("Unformatted")
    {
        TEST_FORMAT("", "");
        TEST_FORMAT("foo", "foo");
        TEST_FORMAT("{", "{{");
        TEST_FORMAT("}", "}}");
        TEST_FORMAT("{}", "{{}}");
        TEST_FORMAT("}{", "}}{{");
        TEST_FORMAT("{0}", "{{0}}", 1);
        TEST_FORMAT("{{0}}", "{{{{0}}}}", 1);
        TEST_FORMAT("a{b", "a{{b");
        TEST_FORMAT("a}b", "a}}b");
    }

    TEST_CASE("Custom format")
    {
        TEST_FORMAT("<@:>f0\\", "{:<@:>f0\\}", Foo{});
        TEST_FORMAT("{}", "{:{{}}}", Foo{});
        TEST_FORMAT("<empty>}", "{:}}", Foo{});
    }

    TEST_CASE("Nested formats")
    {
        TEST_FORMAT("a b ", "{:{}}{:{}}", 'a', 2, 'b', 2);
        TEST_FORMAT("+    52.00", "{:{}}", 52.0f, "=+10.2f");
        TEST_FORMAT("oog", "{1:.{0}}", 3, "ooga");
        TEST_FORMAT("   1.0000", "{:{}.{}f}", 1.0f, 9, 4);

        // LET'S GO DEEPER
        TEST_FORMAT("Hello", "{0:{0:{1}}}", Foo{}, "Hello");

        // SO DEEP
        TEST_FORMAT("Hello", "{0:{0:{0:{1}}}}", Foo{}, "Hello");

        // IT'S LIKE SHAKESPEARE IN HERE. SO DEEP
        TEST_FORMAT("Hello", "{0:{0:{0:{0:{1}}}}}", Foo{}, "Hello");
    }

    TEST_CASE("Readme formats")
    {
        // Examples
        TEST_FORMAT("Hello, World!\n", "Hello, {}!\n", "World");
        TEST_FORMAT("+0000512", "{:+08}", 512);
        TEST_FORMAT("name=John,height=1.80,employed=true", "name={2},height={0:.2f},employed={1}", 1.8019f, true, "John");

        // Format string
        TEST_FORMAT("{:", "{:", 1);
        TEST_FORMAT("{:.}", "{:.}", 1);
        TEST_FORMAT("foo", "{:=}", "foo");
        TEST_FORMAT("{0!s}", "{0!s}", 1);
        TEST_FORMAT("{0!a}", "{0!a}", 1);
        TEST_FORMAT("0 2", "{} {2}", 0, 1, 2);
        TEST_FORMAT("{foo.bar}", "{foo.bar}", 1);
        TEST_FORMAT("{0[0]}", "{0[0]}", 1);
        TEST_FORMAT("0 1 2", "{} {} {}", 0, 1, 2);
        TEST_FORMAT("0 1 2", "{0} {1} {2}", 0, 1, 2);
        TEST_FORMAT("0 1 1 2 1", "{} {} {1} {} {1}", 0, 1, 2);
        TEST_FORMAT("0 1 1 2 1", "{0} {1} {1} {2} {1}", 0, 1, 2);
        TEST_FORMAT("..1", "{0:.>{1}}", 1, 3);
        TEST_FORMAT("+5.0 _", "{:{}{}} {}", 5.0f, '+', ".1f", '_');
        TEST_FORMAT("+5.0 _", "{0:{1}{2}} {3}", 5.0f, '+', ".1f", '_');
        TEST_FORMAT("{:_}", "{:_}", 1);
        TEST_FORMAT("{:,}", "{:,}", 1);
        TEST_FORMAT("{:n}", "{:n}", 1);
        TEST_FORMAT("A", "{:c}", 65);
        TEST_FORMAT("x", "{:#c}", 120);
        TEST_FORMAT("(100)", "{:c}", 256);
        TEST_FORMAT("(0xa0)", "{:#c}", 160);
        TEST_FORMAT("(-0x5)", "{:#c}", -5);
        TEST_FORMAT("314159265", "{}", 314159265.0);
    }

    TEST_CASE("Char formats")
    {
        TEST_FORMAT(" ", "{}", (char)32);
        TEST_FORMAT(" ", "{}", (char16_t)32);
        TEST_FORMAT(" ", "{}", (char32_t)32);
        TEST_FORMAT(" ", "{}", (wchar_t)32);
        TEST_FORMAT("x", "{}", 'x');
        TEST_FORMAT("A", "{:c}", 65);
        TEST_FORMAT("(-41)", "{:c}", -65);
        TEST_FORMAT("(-0x41)", "{:#c}", -65);
        TEST_FORMAT("x  ", "{:3}", 'x');
        TEST_FORMAT("  x", "{:>3}", 'x');
        TEST_FORMAT("\x7f", "{:c}", 0x7f);
        TEST_FORMAT("(80)", "{:c}", 0x80);
        TEST_FORMAT("(+80)", "{:+c}", 0x80);
        TEST_FORMAT("  (+80)  ", "{:^+9c}", 0x80);
    }

    TEST_CASE("Integer formats")
    {
        TEST_FORMAT("true", "{}", true);
        TEST_FORMAT("false", "{}", false);
        TEST_FORMAT("0", "{:d}", false);
        TEST_FORMAT("1", "{:x}", true);
        TEST_FORMAT(" true ", "{:^6}", true);

        TEST_FORMAT("1", "{}", (signed char)1);
        TEST_FORMAT("1", "{}", (unsigned char)1);
        TEST_FORMAT("1", "{}", (short)1);
        TEST_FORMAT("1", "{}", (unsigned short)1);
        TEST_FORMAT("1", "{}", (int)1);
        TEST_FORMAT("1", "{}", (unsigned)1);
        TEST_FORMAT("1", "{}", (long)1);
        TEST_FORMAT("1", "{}", (unsigned long)1);
        TEST_FORMAT("1", "{}", (long long)1);
        TEST_FORMAT("1", "{}", (unsigned long long)1);

        TEST_FORMAT("1", "{}", int8_t(1));
        TEST_FORMAT("1", "{}", uint8_t(1));
        TEST_FORMAT("1", "{}", int16_t(1));
        TEST_FORMAT("1", "{}", uint16_t(1));
        TEST_FORMAT("1", "{}", int32_t(1));
        TEST_FORMAT("1", "{}", uint32_t(1));
        TEST_FORMAT("1", "{}", int64_t(1));
        TEST_FORMAT("1", "{}", uint64_t(1));

        TEST_FORMAT("0", "{}", nullptr);
        TEST_FORMAT("7ff00000", "{}", (int32_t*)0x7ff00000);

        TEST_FORMAT("42", "{}", 42);
        TEST_FORMAT("-15", "{0}", -15);
        TEST_FORMAT("+96", "{:+}", 96);
        TEST_FORMAT(" 7", "{: }", 7);
        TEST_FORMAT(" 32", "{: 3}", 32);
        TEST_FORMAT("  75", "{: 4}", 75);
        TEST_FORMAT("   4", "{:4}", 4);
        TEST_FORMAT("300", "{:0<3}", 3);
        TEST_FORMAT(" 2  ", "{:^4}", 2);
        TEST_FORMAT("  8  ", "{:^5}", 8);
        TEST_FORMAT("+  52", "{:=+5}", 52);
        TEST_FORMAT("101000", "{:b}", 40);
        TEST_FORMAT("0b1000100", "{:#b}", 68);
        TEST_FORMAT("77", "{:o}", 63);
        TEST_FORMAT("0o36", "{:#o}", 30);
        TEST_FORMAT("abc", "{:x}", 2748);
        TEST_FORMAT("0xba", "{:#x}", 186);
        TEST_FORMAT("F00", "{:X}", 3840);
        TEST_FORMAT("0XBAD", "{:#X}", 2989);

        TEST_FORMAT("0x000001", "{:#08x}", 1);
        TEST_FORMAT("0x000001", "{:=#08x}", 1);
        TEST_FORMAT("0x100000", "{:<#08x}", 1);
        TEST_FORMAT("000x1000", "{:^#08x}", 1);
        TEST_FORMAT("000000x1", "{:>#08x}", 1);
        TEST_FORMAT("-0x00001", "{:-#08x}", -1);
        TEST_FORMAT("-0x00001", "{:=-#08x}", -1);
        TEST_FORMAT("-0x10000", "{:<-#08x}", -1);
        TEST_FORMAT("00-0x100", "{:^-#08x}", -1);
        TEST_FORMAT("0000-0x1", "{:>-#08x}", -1);

        TEST_FORMAT("-0b10000000", "{:#b}", INT8_MIN);
        TEST_FORMAT("+  177", "{:=+6o}", INT8_MAX);
        TEST_FORMAT(">> 18446744073709551615", "{:>> 23}", UINT64_MAX);
        TEST_FORMAT("0x7fffffffffffffff", "{:#x}", INT64_MAX);
    }

    TEST_CASE("Float formats")
    {
        TEST_FORMAT("nan", "{}", NAN);
        TEST_FORMAT("NAN", "{:F}", NAN);
        TEST_FORMAT("inf", "{}", INFINITY);
        TEST_FORMAT("INF", "{:F}", INFINITY);
        TEST_FORMAT("-inf", "{}", -INFINITY);
        TEST_FORMAT("-INF", "{:F}", -INFINITY);

        TEST_FORMAT("1", "{}", 1.0);
        TEST_FORMAT("1.5", "{}", 1.5f);
        TEST_FORMAT("1.79769313486232e+308", "{}", DBL_MAX);
        TEST_FORMAT("1.17549e-38", "{}", FLT_MIN);
        TEST_FORMAT("-3.40282e+38", "{}", -FLT_MAX);

        TEST_FORMAT(" 1.000000e+00", "{: e}", 1.0f);
        TEST_FORMAT("-1.000000e+00", "{:e}", -1.0f);
        TEST_FORMAT("1.234568E+05", "{:E}", 123456.789);
        TEST_FORMAT("5.12E+02", "{:.2E}", 512.1024);
        TEST_FORMAT("3.251923299534e+01", "{:.12e}", 32.5192329953432345);

        TEST_FORMAT("1.000000", "{:f}", 1.0f);
        TEST_FORMAT("-1.000000", "{:f}", -1.0f);
        TEST_FORMAT("+1.234568", "{:+f}", 1.23456789f);
        TEST_FORMAT("3.1416", "{:.4f}", 3.14159265f);
        TEST_FORMAT("1.57079633", "{:.8f}", 1.5707963267948966192);

        TEST_FORMAT("1", "{:g}", 1.0);
        TEST_FORMAT("-52", "{:g}", -52.0f);
        TEST_FORMAT("3.14", "{:G}", 3.14);
        TEST_FORMAT("+3.142", "{:+.4g}", 3.14159265);
        TEST_FORMAT("1.23457e+19", "{:.6g}", 12345678901234567890.0);

        TEST_FORMAT("   12", "{:5g}", 12.0f);
        TEST_FORMAT("42.0101  ", "{:<9.6g}", 42.0101);
        TEST_FORMAT("  12  ", "{:^6}", 12.0);
        TEST_FORMAT("xxx32.007", "{:x>9.3f}", 32.00723f);
        TEST_FORMAT("__1__", "{:_^5g}", 1.0f);
        TEST_FORMAT("??2???", "{:?^6g}", 2.0f);
    }

    TEST_CASE("String formats")
    {
        char abc[4] = {'a', 'b', 'c', 0};

        TEST_FORMAT("", "{}", (const char*)nullptr);
        TEST_FORMAT("abc", "{}", abc);
        TEST_FORMAT("abc", "a{}c", "b");
        TEST_FORMAT("bar", "{}{}{}", "b", "a", "r");
        TEST_FORMAT("baz", "{2}{0}{}", "a", "z", "b");
        TEST_FORMAT("{foo}", "{{{}}}", "foo");
        TEST_FORMAT("foo ", "{:4}", "foo");
        TEST_FORMAT("foo", "{:o<3}", "f");
        TEST_FORMAT(".foo", "{:.>4}", "foo");
        TEST_FORMAT("  foo  ", "{:^7}", "foo");
        TEST_FORMAT("  foo   ", "{:^8}", "foo");
        TEST_FORMAT("cc", "{:c<2s}", "c");
        TEST_FORMAT("trunc", "{:.5}", "truncate");
        TEST_FORMAT("--ball---", "{:-^9.4s}", "ballet");
        TEST_FORMAT("foo", "{}", sp::StringView("foo"));

        std::string str(999, ' ');
        str.push_back('a');
        TEST_FORMAT(str.data(), "{0:>1000}", "a");

        str = std::string(1000, ' ');
        TEST_FORMAT(str.data(), "{0:1000}", "");

        // We only null-terminate if the format is null-terminated, and only
        // if the entire format was able to be written. Basically we ignore
        // them but copy them as just any other character.
        {
            // Fill the entire buffer.
            char buffer[4] = { 0x7f, 0x7f, 0x7f, 0x7f };
            const auto written = sp::format(buffer, "{}{}{}{}", 1, 2, 3, 4);
            REQUIRE(written == 4);
            REQUIRE(buffer[0] == '1');
            REQUIRE(buffer[1] == '2');
            REQUIRE(buffer[2] == '3');
            REQUIRE(buffer[3] == '4');
        }

        {
            // Leave the remaining chars alone
            char buffer[5] = { 0x7f, 0x7f, 0x7f, 0x7f, 0x7f };
            const auto written = sp::format(buffer, "{}{}{}", 1, 2, 3);
            REQUIRE(written == 3);
            REQUIRE(buffer[0] == '1');
            REQUIRE(buffer[1] == '2');
            REQUIRE(buffer[2] == '3');
            REQUIRE(buffer[3] == 0x7f);
            REQUIRE(buffer[4] == 0x7f);
        }

        {
            // Keep the null terminator from the format, if it had one
            char buffer[4] = { 0x7f, 0x7f, 0x7f, 0x7f };
            const auto written = sp::format(buffer, sp::StringView("{}{}{}", 7), 1, 2, 3);
            REQUIRE(written == 4);
            REQUIRE(buffer[0] == '1');
            REQUIRE(buffer[1] == '2');
            REQUIRE(buffer[2] == '3');
            REQUIRE(buffer[3] == 0);
        }
    }

    TEST_CASE("StringWriter") {
        char buffer[64];
        sp::StringWriter writer(buffer, sizeof(buffer));
        char data[40] = {0x77};

        size_t written = writer.write(sizeof(data), data);
        REQUIRE(written == sizeof(data));
        written = writer.write(sizeof(data), data);
        REQUIRE(written == sizeof(buffer) - sizeof(data));
        written = writer.write(sizeof(data), data);
        REQUIRE(written == 0);

        REQUIRE(writer.result() == sizeof(data) * 3);
    }

    // This should work on other platforms too, but only linux implements
    // fmemopen, which makes this a lot easier to test. Since we're only
    // really testing our own logic, and not that of the CRT, it should be
    // fine to only test it on non-Windows platforms anyway.
#if defined(__linux__)
    TEST_CASE("StreamWriter") {
        char buffer[40];
        char data[40] = {0x77};

        FILE* stream = fmemopen(buffer, sizeof(buffer), "wb");
        REQUIRE(stream != nullptr);
        sp::StreamWriter writer(stream);

        // fmemopen is stupid enough to have fwrite return the amount that you
        // requested to write, irregardless of how much was *actually* written.
        // So really the only useful thing we can test is that it returns how
        // much we requested...
        size_t written = writer.write(sizeof(data), data);
        REQUIRE(written == sizeof(data));
    }
#endif

    if (!s_failed) {
        sp::print("All tests passed!\n");
    }

    return s_failed;
}
