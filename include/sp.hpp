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

#include <cstdint> // int32_t, uint64_t
#include <cstdio> // printf, FILE, fwrite, fprintf
#include <cstring> // std::memcpy
#include <algorithm> // std::min, std::max
#include <limits> // std::numeric_limits
#include <utility> // std::forward

///
// API
///

namespace sp {

    /// Output for the string formatter.
    class Output {
    public:
        /// Construct an output that writes to standard out.
        Output();

        /// Construct an output that writes into the provided buffer, with the
        /// given size.
        Output(char buffer[], size_t size);

        /// Construct an output that writes into the provided FILE stream.
        explicit Output(FILE* stream);

        /// Return the result of the print operation.
        ///
        /// If the underlying output is a FILE stream:
        /// - Return the amount of `char`s written in case of success.
        /// - Return `-1` in case of error.
        ///
        /// If the underlying output is a buffer:
        /// - Return the amount of `char`s that make up the result of the
        ///   formatted string. If the buffer was not big enough to hold the
        ///   entire formatted string, the returned length may be larger than
        ///   the size of the buffer.
        /// - Return `-1` in case of error.
        int32_t result() const;

        /// Write the provided character into the output.
        void write(char ch);

        /// Write the provided data into the output.
        void write(size_t length, const void* data);

    private:
        Output(const Output&) = delete;
        Output& operator=(const Output&) = delete;

        FILE* m_stream = nullptr;
        char* m_buffer = nullptr;
        int32_t m_size = 0;
        int32_t m_length = 0;
    };

    /// View into a string.
    struct StringView {
        const char* const ptr = nullptr; //< Pointer to the string.
        const int32_t length = 0; //< Length of the string.

        /// Construct an empty StringView.
        StringView();

        /// Construct a StringView from the provided null-terminated string.
        StringView(const char str[]);

        /// Construct a StringView from the provided string, with the provided
        /// length (in `char`).
        StringView(const char str[], int32_t length);
    };

    /// Print to standard out using the provided format with the provided
    /// format arguments. Return the amount of `char`s written, or `-1` in case
    /// of an error.
    template <class... Args>
    int32_t print(const StringView& fmt, Args&&... args);

    /// Print to the provided output using the provided format with the
    /// provided format arguments. Return the result of calling
    /// `output.result()`.
    template <class... Args>
    int32_t format(Output& output, const StringView& fmt, Args&&... args);

    /// Print to the provided FILE stream using the provided format with the
    /// provided format arguments. Return the amount of `char`s written, or
    /// `-1` in case of an error.
    template <class... Args>
    int32_t format(FILE* file, const StringView& fmt, Args&&... args);

    /// Print to the provided buffer of the provided size, using the provided
    /// format string with the provided format arguments. Return the amount of
    /// `char`s that make up the resulting formatted string. If the buffer was
    /// not big enough to hold the entire result, the returned value may be
    /// larger than the buffer size. Return `-1` in case of an error.
    template <class... Args>
    int32_t format(char buffer[], size_t size, const StringView& fmt, Args&&... args);

    /// Print to the provided statically sized buffer, using the provided
    /// format string with the provided format arguments. Return the amount of
    /// `char`s that make up the resulting formatted string. If the buffer was
    /// not big enough to hold the entire result, the returned value may be
    /// larger than the buffer size. Return `-1` in case of an error.
    template <size_t N, class... Args>
    int32_t format(char (&buffer)[N], const StringView& fmt, Args&&... args);

    /// Provided format functions.
    bool format_value(Output& output, const StringView& fmt, bool value);
    bool format_value(Output& output, const StringView& fmt, float value);
    bool format_value(Output& output, const StringView& fmt, double value);
    bool format_value(Output& output, const StringView& fmt, char value);
    bool format_value(Output& output, const StringView& fmt, signed char value);
    bool format_value(Output& output, const StringView& fmt, unsigned char value);
    bool format_value(Output& output, const StringView& fmt, short value);
    bool format_value(Output& output, const StringView& fmt, unsigned short value);
    bool format_value(Output& output, const StringView& fmt, int value);
    bool format_value(Output& output, const StringView& fmt, unsigned value);
    bool format_value(Output& output, const StringView& fmt, long value);
    bool format_value(Output& output, const StringView& fmt, unsigned long value);
    bool format_value(Output& output, const StringView& fmt, long long value);
    bool format_value(Output& output, const StringView& fmt, unsigned long long value);
    bool format_value(Output& output, const StringView& fmt, const char value[]);

    template <class T>
    bool format_value(Output& output, const StringView& fmt, T* value);

} // namespace sp

///
// Implementation
///

namespace sp {

    inline Output::Output()
        : m_stream(stdout)
    {
    }

    inline Output::Output(char buffer[], size_t size)
        : m_buffer(buffer)
        , m_size(int32_t(size))
    {
    }

    inline Output::Output(FILE* stream)
        : m_stream(stream)
    {
    }

    inline int32_t Output::result() const
    {
        return m_length;
    }

    inline void Output::write(char ch)
    {
        write(sizeof(ch), &ch);
    }

    inline void Output::write(size_t bytes, const void* data)
    {
        if (m_length >= 0) {
            if (m_stream) {
                const auto written = std::fwrite(data, 1, bytes, m_stream);

                if (written == bytes) {
                    m_length += int32_t(written);
                } else {
                    m_length = -1;
                }
            } else {
                m_length += int32_t(bytes);

                if (m_size) {
                    const auto toCopy = std::min(size_t(m_size) - 1, bytes);
                    std::memcpy(m_buffer, data, toCopy);
                    m_buffer[toCopy] = 0;
                    m_buffer += toCopy;
                    m_size -= int32_t(toCopy);
                }
            }
        }
    }

    inline StringView::StringView() {}

    inline StringView::StringView(const char str[])
        : ptr(str)
        , length(int32_t(std::strlen(str)))
    {
    }

    inline StringView::StringView(const char str[], int32_t length)
        : ptr(str)
        , length(length)
    {
    }

    struct FormatFlags {
        char fill = 0;
        char align = 0;
        char sign = 0;
        bool alternate = false;
        int32_t width = -1;
        int32_t precision = -1;
        char type = 0;
    };

    inline bool parse_format(const StringView& fmt, FormatFlags* flags)
    {
        enum State {
            STATE_ALIGN,
            STATE_SIGN,
            STATE_ALTERNATE,
            STATE_WIDTH,
            STATE_PRECISION,
            STATE_TYPE,
            STATE_DONE,
        };

        auto state = STATE_ALIGN;
        auto next = fmt.ptr;
        auto term = fmt.ptr + fmt.length;
        *flags = FormatFlags{};

        while (next < term) {
            const auto ptr = next++;
            const auto ch = *ptr;

            switch (state) {
            case STATE_ALIGN: {
                const auto isAlign = [](char ch) {
                    return (ch >= '<' && ch <= '>') || ch == '^';
                };

                if (next < term && isAlign(*next)) {
                    flags->fill = ch;
                    flags->align = *next++;
                } else if (isAlign(ch)) {
                    flags->align = ch;
                } else {
                    --next;
                }

                state = STATE_SIGN;
                break;
            }

            case STATE_SIGN:
                switch (ch) {
                case '+':
                case '-':
                case ' ':
                    flags->sign = ch;
                    break;
                default:
                    --next;
                    break;
                }
                state = STATE_ALTERNATE;
                break;

            case STATE_ALTERNATE:
                if (ch == '#') {
                    flags->alternate = true;
                } else {
                    --next;
                }
                state = STATE_WIDTH;
                break;

            case STATE_WIDTH:
                if (ch >= '0' && ch <= '9') {
                    if (flags->width < 0) {
                        if (ch == '0') {
                            flags->fill = '0';
                            flags->align = '=';
                        }
                        flags->width = 0;
                    }
                    flags->width = (flags->width * 10) + (ch - '0');
                } else {
                    state = STATE_PRECISION;
                    --next;
                }
                break;

            case STATE_PRECISION:
                if (flags->precision < 0 && ch == '.') {
                    flags->precision = 0;
                } else if (flags->precision >= 0 && ch >= '0' && ch <= '9') {
                    flags->precision = (flags->precision * 10) + (ch - '0');
                } else {
                    state = STATE_TYPE;
                    --next;
                }
                break;

            case STATE_TYPE:
                switch (ch) {
                case 'b':
                case 'd':
                case 'e':
                case 'E':
                case 'f':
                case 'F':
                case 'g':
                case 'G':
                case 'o':
                case 's':
                case 'x':
                case 'X':
                case '%':
                    flags->type = ch;
                    break;
                default:
                    --next;
                    break;
                }
                state = STATE_DONE;
                break;

            case STATE_DONE:
                // if we get here, it means there's stuff in there after the
                // flags, in which case we should fail.
                return false;
            }
        }

        return true;
    }

    inline bool format_int(Output& output, const FormatFlags& flags, bool isNegative, uint64_t value)
    {
        // determine base
        int32_t base = 10;

        switch (flags.type) {
        case 'b':
            base = 2;
            break;
        case 'o':
            base = 8;
            break;
        case 'x':
        case 'X':
            base = 16;
            break;
        }

        // count digits, and copy them to a buffer (so we don't have to repeat
        // this later)
        char buffer[66]; // max needed; 64-bit binary with alternate form
        char* digits = buffer + sizeof(buffer);
        int32_t ndigits = 0;

        {
            const auto digitchars = (flags.type == 'X')
                ? "0123456789ABCDEF"
                : "0123456789abcdef";

            uint64_t v = value;
            do {
                *(--digits) = digitchars[v % base];
                v /= base;
                ++ndigits;
            } while (v);

            if (flags.alternate) {
                switch (base) {
                case 2:
                    *(--digits) = 'b';
                    *(--digits) = '0';
                    ndigits += 2;
                    break;
                case 8:
                    *(--digits) = 'o';
                    *(--digits) = '0';
                    ndigits += 2;
                    break;
                case 16:
                    *(--digits) = flags.type;
                    *(--digits) = '0';
                    ndigits += 2;
                    break;
                }
            }
        }

        // determine sign
        char sign = 0;

        if (isNegative) {
            sign = '-';
        } else if (flags.sign == '+' || flags.sign == ' ') {
            sign = flags.sign;
        }

        // determine spacing
        int32_t nchars = sign ? ndigits + 1 : ndigits;
        int32_t leadSpace = 0;
        int32_t tailSpace = 0;
        {
            int32_t width = std::max(flags.width, nchars);

            switch (flags.align) {
            case '^':
                leadSpace = (width / 2) - ((nchars + 1) / 2); // nchars rounded up
                tailSpace = ((width + 1) / 2) - (nchars / 2); // width rounded up
                leadSpace += (width - nchars - 1) & 1; // if both are even or both are odd, we need to add one for correction
                tailSpace -= (width - nchars - 1) & 1; // if both are even or both are odd, we need to remove one for correction
                break;
            case '<':
                tailSpace = width - nchars;
                break;
            case '>':
            case '=':
            default:
                leadSpace = width - nchars;
                break;
            }
        }

        // print sign, if it should be before the padding
        if (sign && flags.align == '=') {
            output.write(sign);
        }

        // apply the leading padding
        const char fill = flags.fill ? flags.fill : ' ';

        while (leadSpace--) {
            output.write(fill);
        }

        // print the sign, if it should be after the padding
        if (sign && flags.align != '=') {
            output.write(sign);
        }

        // print digits
        output.write(ndigits, digits);

        // print tailing padding
        while (tailSpace--) {
            output.write(fill);
        }

        return true;
    }

    template <class F>
    bool format_float(Output& output, const StringView& format, F value)
    {
        FormatFlags flags;

        if (!parse_format(format, &flags)) {
            return false;
        }

        // I *really* have no interest in serializing floats/doubles... so
        // let's not. Instead, let's build a format string for snprintf to do
        // the heavy work, and we'll just do alignment and stuff.
        const char* suffix = "";
        int32_t precision;
        char type;

        switch (flags.type) {
        case 'f':
        case 'F':
        case 'e':
        case 'E':
            precision = flags.precision >= 0 ? flags.precision : 6;
            type = flags.type;
            break;
        case 'g':
        case 'G':
            precision = (flags.precision != 0)
                ? (flags.precision > 0 ? flags.precision : 6)
                : 1;
            type = flags.type;
            break;
        case '%':
            precision = flags.precision >= 0 ? flags.precision : 6;
            type = 'f';
            value *= 100;
            suffix = "%%";
            break;
        default:
            precision = flags.precision >= 0 ? flags.precision : std::numeric_limits<F>::digits10;
            type = 'g';
            break;
        }

        char numFormat[16];
        snprintf(numFormat, sizeof(numFormat), "%%+.%d%c%s", precision, type, suffix);

        // produce the formatted value
        char buffer[512];
        const int32_t ndigits = snprintf(buffer, sizeof(buffer), numFormat, value) - 1;
        const auto digits = buffer + 1;

        // determine sign
        char sign = 0;

        if (value < 0) {
            sign = '-';
        } else if (flags.sign == '+' || flags.sign == ' ') {
            sign = flags.sign;
        }

        // determine width
        const int nchars = sign ? ndigits + 1 : ndigits;
        const int32_t width = std::max(flags.width, nchars);

        // determine alignment
        int32_t leadSpace = 0;
        int32_t tailSpace = 0;

        switch (flags.align) {
        case '^':
            leadSpace = (width / 2) - ((nchars + 1) / 2); // nchars rounded up
            tailSpace = ((width + 1) / 2) - (nchars / 2); // width rounded up
            leadSpace += (width - nchars - 1) & 1; // if both are even or both are odd, we need to add one for correction
            tailSpace -= (width - nchars - 1) & 1; // if both are even or both are odd, we need to remove one for correction
            break;
        case '<':
            tailSpace = width - nchars;
            break;
        case '>':
        default:
            leadSpace = width - nchars;
            break;
        }

        // print sign, if it should be before the padding
        if (sign && flags.align == '=') {
            output.write(sign);
        }

        // apply leading padding
        const char fill = flags.fill ? flags.fill : ' ';

        while (leadSpace--) {
            output.write(fill);
        }

        // print sign, if it should be after the padding
        if (sign && flags.align != '=') {
            output.write(sign);
        }

        // write string
        output.write(ndigits, digits);

        // apply tailing padding
        while (tailSpace--) {
            output.write(fill);
        }

        return true;
    }

    inline bool format_string(Output& output, const StringView& format, const StringView& str)
    {
        FormatFlags flags;

        if (!parse_format(format, &flags)) {
            return false;
        }

        // determine the amount of characters to write
        auto nchars = str.length;

        if (flags.precision >= 0) {
            nchars = std::min(flags.precision, nchars);
        }

        // determine width
        const int32_t width = std::max(flags.width, nchars);

        // determine alignment
        int32_t leadSpace = 0;
        int32_t tailSpace = 0;

        switch (flags.align) {
        case '^':
            leadSpace = (width / 2) - ((nchars + 1) / 2); // nchars rounded up
            tailSpace = ((width + 1) / 2) - (nchars / 2); // width rounded up
            leadSpace += (width - nchars - 1) & 1; // if both are even or both are odd, we need to add one for correction
            tailSpace -= (width - nchars - 1) & 1; // if both are even or both are odd, we need to remove one for correction
            break;
        case '>':
            leadSpace = width - nchars;
            break;
        case '<':
        default:
            tailSpace = width - nchars;
            break;
        }

        // apply leading padding
        const char fill = flags.fill ? flags.fill : ' ';

        while (leadSpace--) {
            output.write(fill);
        }

        // write string
        output.write(nchars, str.ptr);

        // apply tailing padding
        while (tailSpace--) {
            output.write(fill);
        }

        return true;
    }

    struct DummyArg {
    };

    inline bool format_value(Output&, const StringView&, const DummyArg&)
    {
        return false;
    }

    inline bool format_index(Output&, const StringView&, int32_t)
    {
        return false;
    }

    template <class Arg, class... Rest>
    bool format_index(Output& output, const StringView& format, int32_t index, Arg&& arg, Rest&&... rest)
    {
        if (!index) {
            return format_value(output, format, std::forward<Arg>(arg));
        } else {
            return format_index(output, format, index - 1, std::forward<Rest>(rest)...);
        }
    }

    template <class... Args>
    int32_t print(const StringView& fmt, Args&&... args)
    {
        Output output;
        format(output, fmt, std::forward<Args>(args)...);
        return output.result();
    }

    template <class... Args>
    int32_t format(Output& output, const StringView& fmt, Args&&... args)
    {
        enum State {
            STATE_OPENER,
            STATE_INDEX,
            STATE_MARKER,
            STATE_FLAGS,
            STATE_CLOSER,
        };

        auto before = output.result();
        auto state = STATE_OPENER;
        auto next = fmt.ptr;
        auto start = fmt.ptr;
        auto term = fmt.ptr + fmt.length;

        const char* formatStart = nullptr;
        auto prev = -1;
        auto index = -1;

        while (next < term) {
            const auto ptr = next++;
            const auto ch = *ptr;

            switch (state) {
            case STATE_OPENER:
                if (ch == '{') {
                    output.write(int32_t(ptr - start), start);

                    if (next < term) {
                        if (*next != '{') {
                            index = -1;
                            start = ptr;
                            formatStart = nullptr;
                            state = STATE_INDEX;
                        } else {
                            start = next++;
                        }
                    }
                } else if (ch == '}') {
                    output.write(int32_t(next - start), start);
                    if (next < term && *next == '}') {
                        ++next;
                    }
                    start = next;
                }
                break;

            case STATE_INDEX:
                if (ch >= '0' && ch <= '9') {
                    index = (index < 0)
                        ? (ch - '0')
                        : (index * 10) + (ch - '0');
                } else {
                    if (index < 0) {
                        index = prev + 1;
                    }
                    prev = index;
                    state = STATE_MARKER;
                    --next;
                }
                break;

            case STATE_MARKER:
                if (ch == ':') {
                    state = STATE_FLAGS;
                } else {
                    state = STATE_CLOSER;
                    --next;
                }
                break;

            case STATE_FLAGS:
                if (!formatStart) {
                    formatStart = ptr;
                }

                if (ch == '}' || ch == '{') {
                    state = STATE_CLOSER;
                    --next;
                }

                break;

            case STATE_CLOSER:
                if (ch == '}') {
                    StringView format = formatStart
                        ? StringView(formatStart, int32_t(ptr - formatStart))
                        : StringView();

                    if (format_index(output, format, index, std::forward<Args>(args)...)) {
                        start = next;
                    }
                }
                state = STATE_OPENER;
                break;
            }
        }

        // Print remaining data
        if (start != term) {
            output.write(int32_t(term - start), start);
        }

        auto after = output.result();
        return after < 0 ? after : (after - before);
    }

    template <class... Args>
    int32_t format(FILE* file, const StringView& fmt, Args&&... args)
    {
        Output output(file);
        format(output, fmt, std::forward<Args>(args)...);
        return output.result();
    }

    template <class... Args>
    int32_t format(char buffer[], size_t size, const StringView& fmt, Args&&... args)
    {
        Output output(buffer, size);
        format(output, fmt, std::forward<Args>(args)...);
        return output.result();
    }

    template <size_t N, class... Args>
    int32_t format(char (&buffer)[N], const StringView& fmt, Args&&... args)
    {
        Output output(buffer, N);
        format(output, fmt, std::forward<Args>(args)...);
        return output.result();
    }

    inline bool format_value(Output& output, const StringView& fmt, bool value)
    {
        return format_string(output, fmt, value ? "true" : "false");
    }

    inline bool format_value(Output& output, const StringView& fmt, float value)
    {
        return format_float(output, fmt, value);
    }

    inline bool format_value(Output& output, const StringView& fmt, double value)
    {
        return format_float(output, fmt, value);
    }

    inline bool format_value(Output& output, const StringView& fmt, char value)
    {
        return std::numeric_limits<char>::is_signed
            ? format_value(output, fmt, (long long)value)
            : format_value(output, fmt, (unsigned long long)value);
    }

    inline bool format_value(Output& output, const StringView& fmt, signed char value)
    {
        return format_value(output, fmt, (long long)value);
    }

    inline bool format_value(Output& output, const StringView& fmt, unsigned char value)
    {
        return format_value(output, fmt, (unsigned long long)value);
    }

    inline bool format_value(Output& output, const StringView& fmt, short value)
    {
        return format_value(output, fmt, (long long)value);
    }

    inline bool format_value(Output& output, const StringView& fmt, unsigned short value)
    {
        return format_value(output, fmt, (unsigned long long)value);
    }

    inline bool format_value(Output& output, const StringView& fmt, int value)
    {
        return format_value(output, fmt, (long long)value);
    }

    inline bool format_value(Output& output, const StringView& fmt, unsigned value)
    {
        return format_value(output, fmt, (unsigned long long)value);
    }

    inline bool format_value(Output& output, const StringView& fmt, long value)
    {
        return format_value(output, fmt, (long long)value);
    }

    inline bool format_value(Output& output, const StringView& fmt, unsigned long value)
    {
        return format_value(output, fmt, (unsigned long long)value);
    }

    inline bool format_value(Output& output, const StringView& fmt, long long value)
    {
        static_assert(sizeof(value) == sizeof(uint64_t), "invalid cast on negation");

        const auto abs = (value >= 0 || value == std::numeric_limits<long long>::min())
            ? uint64_t(value)
            : uint64_t(-value);

        FormatFlags flags;

        return parse_format(fmt, &flags)
            && format_int(output, flags, value < 0, abs);
    }

    inline bool format_value(Output& output, const StringView& fmt, unsigned long long value)
    {
        FormatFlags flags;

        return parse_format(fmt, &flags)
            && format_int(output, flags, false, uint64_t(value));
    }

    inline bool format_value(Output& output, const StringView& fmt, const char value[])
    {
        return format_string(output, fmt, value);
    }

    template <class T>
    bool format_value(Output& output, const StringView& fmt, T* value)
    {
        FormatFlags flags;

        if (parse_format(fmt, &flags)) {
            if (!flags.type) {
                flags.type = 'x';
            }

            return format_int(output, flags, false, uint64_t(value));
        }

        return false;
    }

} // namespace sp
