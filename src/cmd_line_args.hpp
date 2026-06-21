/**
 * @file cmd_line_args.hpp
 * @brief Get command line arguments.
 * @author Masashi Kitamura (tenka@6809.net)
 * @date 2023-12-03
 * @note
 *
 *  ex)
 *    cmd_line_args<> args(argc, argv);
 *    int    opt_level = 0;
 *    string opt_oname;
 *    bool   verbose = false;
 *    while (args.has_arg()) {
 *        if (args.prepare_get()) {  // option.
 *            if (args.get_opt('h')) {
 *                return usage();
 *            } else if (args.get_opt('v', verbose)) {
 *            } else if (args.get_opt('o', opt_oname)) {
 *            } else if (args.get_opt("-level", opt_level)) {
 *            } else if (args.get_opt2('d', "-dir", dirname)) {
 *            } else if (args.get_opt("--")) {
 *                args.disable_opt();
 *            }
 *        } else if (*args.get_arg() == '@') {
 *            args.replace_response_str(file_load<std::string>(args.get_arg()+1));
 *        } else { // file.
 *            DO_SOMETHING(args.get_arg());
 *        }
 *    }
 */
#ifndef ZATU_CMDLINEARGS_HPP_INCLUDED
#define ZATU_CMDLINEARGS_HPP_INCLUDED

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cassert>
#if !defined(ZATU_UNUSE_WCHAR_T)
#include <cwchar>
#endif

#if !defined(ZATU_NOEXCEPT)
 #if __cplusplus >= 201103L || _MSVC_LANG >= 201103L
  #define ZATU_NOEXCEPT     noexcept
 #else
  #define ZATU_NOEXCEPT     throw()
 #endif
#endif
#if !defined(ZATU_CONSTEXPR17)
 #if __cplusplus >= 201703L || _MSVC_LANG >= 201703L
  #define ZATU_CONSTEXPR17  constexpr
 #else
  #define ZATU_CONSTEXPR17
 #endif
#endif

namespace zatu {

//  -   -   -   -   -   -   -   -   -   -   -   -   -   -

#if !defined(_ZATU_STR_N_DUP_DEFINED)
#define _ZATU_STR_N_DUP_DEFINED
template<typename C>
C* str_n_dup(C const* s, std::size_t len) {
    C* d = (C*)std::malloc((len + 1) * sizeof(C));
    assert(d != NULL);
    d[len] = 0;
    return (C*)std::memcpy(d, s, len * sizeof(C));
}
#endif

#if !defined(_ZATU_STRZ_TO_LL_DEFINED)
#define _ZATU_STRZ_TO_LL_DEFINED
#if 1   // 0b?:2 0o?:8 0x?:16 other:10  (0??? not oc
namespace _detail {
    template<typename C> inline int
    strz_to_ll_prefix_check(C const*& str) ZATU_NOEXCEPT {
        C   c = (*str == '0') ? str[1] : 0;
        int b = (c == 'x' || c == 'X') ? 16
              : (c == 'b' || c == 'B') ?  2
              : (c == 'o' || c == 'O') ?  8
              :                          10;
        if (b != 10)
            str += 2;
        return b;
    }
}
inline long long strz_to_ull(char const* p,char const** q=0) ZATU_NOEXCEPT {
    using namespace std;
    int base = _detail::strz_to_ll_prefix_check(p);
    return strtoull(p,(char**)q,base);
}
#if !defined(ZATU_UNUSE_WCHAR_T)
inline long long strz_to_ull(wchar_t const* p, wchar_t const** q = 0) ZATU_NOEXCEPT {
    using namespace std;
    int base = _detail::strz_to_ll_prefix_check(p);
    return wcstoull(p, (wchar_t**)q, base);
}
#endif
template<typename C>
inline long long strz_to_ll(C const* p,C const** q=0) ZATU_NOEXCEPT {
    bool sf = *p == '-';
    if (sf || *p == '+')
        ++p;
    long long v = (long long)strz_to_ull(p,q);
    if (sf) v = -v;
    return v;
}
#else
inline long long strz_to_ll(char const* p, char const** q = 0) ZATU_NOEXCEPT {
    using namespace std;
    return strtoll(p, (char**)q, 0);
}
inline long long strz_to_ull(char const* p, char const** q = 0) ZATU_NOEXCEPT {
    using namespace std;
    return strtoull(p, (char**)q, 0);
}
#if !defined(ZATU_UNUSE_WCHAR_T)
inline long long strz_to_ll(wchar_t const* p, wchar_t const** q = 0) ZATU_NOEXCEPT {
    using namespace std;
    return wcstoll(p, (wchar_t**)q, 0);
}
inline long long strz_to_ull(wchar_t const* p, wchar_t const** q = 0) ZATU_NOEXCEPT {
    using namespace std;
    return wcstoull(p, (wchar_t**)q, 0);
}
#endif
#endif
inline long double strz_to_ld(char const* p, char const** q = 0) ZATU_NOEXCEPT {
    using namespace std;
    return strtold(p, (char**)q);
}
#if !defined(ZATU_UNUSE_WCHAR_T)
inline long double strz_to_ld(wchar_t const* p, wchar_t const** q = 0) ZATU_NOEXCEPT {
    using namespace std;
    return wcstold(p, (wchar_t**)q);
}
#endif
#endif

#if !defined(_ZATU_FROM_STRZ_DEFINED)
#define _ZATU_FROM_STRZ_DEFINED
//namespace _detail {
    template<typename T> inline void from_strz(T& v, char const* p) { v = p; }
    template<typename T> inline void from_strz(T& v, char const* p, char const** q) { v = p; if (q) *q = p + std::strlen(p); }
    inline void from_strz(char& v, char const* p, char const** q) { v = *p; if (v && q) *q = p + 1; }
    #define _ZATU_DEF_FROM_STRZ(C,T,F) inline void from_strz(T& v, C const* p, C const** q=0) ZATU_NOEXCEPT { v = (T)F(p,q); }
    _ZATU_DEF_FROM_STRZ(char, long long            , strz_to_ll)
    _ZATU_DEF_FROM_STRZ(char, long                 , strz_to_ll)
    _ZATU_DEF_FROM_STRZ(char, int                  , strz_to_ll)
    _ZATU_DEF_FROM_STRZ(char, short                , strz_to_ll)
    _ZATU_DEF_FROM_STRZ(char, signed char          , strz_to_ll)
    _ZATU_DEF_FROM_STRZ(char, unsigned long long   , strz_to_ull)
    _ZATU_DEF_FROM_STRZ(char, unsigned long        , strz_to_ull)
    _ZATU_DEF_FROM_STRZ(char, unsigned int         , strz_to_ull)
    _ZATU_DEF_FROM_STRZ(char, unsigned short       , strz_to_ull)
    _ZATU_DEF_FROM_STRZ(char, unsigned char        , strz_to_ull)
    _ZATU_DEF_FROM_STRZ(char, long double          , strz_to_ld)
    _ZATU_DEF_FROM_STRZ(char, double               , strz_to_ld)
    _ZATU_DEF_FROM_STRZ(char, float                , strz_to_ld)
    #if !defined(ZATU_UNUSE_WCHAR_T)
    inline void from_strz(wchar_t& v, wchar_t const* p, wchar_t const** q) { v = *p; if (v && q) *q = p + 1; }
    _ZATU_DEF_FROM_STRZ(wchar_t, long long         , strz_to_ll)
    _ZATU_DEF_FROM_STRZ(wchar_t, long              , strz_to_ll)
    _ZATU_DEF_FROM_STRZ(wchar_t, int               , strz_to_ll)
    _ZATU_DEF_FROM_STRZ(wchar_t, short             , strz_to_ll)
    _ZATU_DEF_FROM_STRZ(wchar_t, signed char       , strz_to_ll)
    _ZATU_DEF_FROM_STRZ(wchar_t, unsigned long long, strz_to_ull)
    _ZATU_DEF_FROM_STRZ(wchar_t, unsigned long     , strz_to_ull)
    _ZATU_DEF_FROM_STRZ(wchar_t, unsigned int      , strz_to_ull)
    _ZATU_DEF_FROM_STRZ(wchar_t, unsigned short    , strz_to_ull)
    _ZATU_DEF_FROM_STRZ(wchar_t, unsigned char     , strz_to_ull)
    _ZATU_DEF_FROM_STRZ(wchar_t, long double       , strz_to_ld)
    _ZATU_DEF_FROM_STRZ(wchar_t, double            , strz_to_ld)
    _ZATU_DEF_FROM_STRZ(wchar_t, float             , strz_to_ld)
    #endif  // !_ZATU_UNUSE_WCHAR_T
    #undef  _ZATU_DEF_FROM_STRZ
//}
//template<typename T, typename C> inline void from_strz(T& t, C const* p, C const** q=0) { _detail::from_strz(t,p,q); }
template<typename T, typename C> inline T strz_to(C const* p, C const** q=0) { T t; from_strz(t,p,q); return t; }
template<typename T, typename C> inline T strz_get(C const*& p) { return strz_to<T>(p, &p); }
#endif  // _ZATU_FROM_STRZ_DEFINED


//  -   -   -   -   -   -   -   -   -   -   -   -   -   -

namespace _detail {

    /**
     *  Parse response string and insert into argv.
     *  # Do not free malloc-memory, leave it to OS process termination.
     */
    template<typename C> bool
    insert_str_to_args(C const* bgn, C const* end, int index, int& rArgc, C**& rArgv, bool& rAlloc) ZATU_NOEXCEPT {
        std::size_t max_arg_sz = 0;
        C**         argv    = rArgv;
        C**         arg_ary = NULL;
        bool        rc      = true;
        int         num     = 0;
        for (int pass = 0; pass < 2; ++pass) {
            C const*    s   = bgn;
            bool        dq  = false;
            bool        cmt = false;
            bool        ltop = true;
            std::size_t arg_sz = 0;
            while (s < end) {
                C   c = *s++;
                if (c == 0)
                    break;
                if (!dq) {
                    if (c == C('\n')) {
                        cmt  = false;
                        ltop = true;
                    }
                    if (cmt)
                        continue;
                    if (unsigned(c) <= 0x20 || c == 0x7f) {
                        if (arg_sz) {
                            if (max_arg_sz < arg_sz)
                                max_arg_sz = arg_sz;
                            if (pass) {
                                argv[num] = str_n_dup(s-1 - arg_sz, arg_sz);
                            }
                            ++num;
                        }
                        arg_sz = 0;
                        continue;
                    } else if (c == C('"')) {
                        dq = true;
                        continue;
                    } else if (c == C('#') && ltop) {
                        cmt  = true;
                        ltop = false;
                        continue;
                    }
                } else {
                    if (c == C('"')) {
                        if (s < end && *s == C('"')) {
                            ++s;
                        } else {
                            dq = false;
                            continue;
                        }
                    }
                }
                ++arg_sz;
                ltop = false;
            }
            if (arg_sz) {
                if (max_arg_sz < arg_sz)
                    max_arg_sz = arg_sz;
                if (pass) {
                    argv[num] = str_n_dup(s - 1 - arg_sz, arg_sz);
                }
                ++num;
            }
            if (pass)
                break;
            if (num == 0)
                break;
            std::size_t arg_ary_size = (rArgc + num + 1) * sizeof(C*);
            arg_ary = (C**)std::calloc(1, arg_ary_size);
            if (!arg_ary) {
                rc = false;
                break;
            }
            for (int i = 0; i < index; ++i)
                arg_ary[i] = argv[i];
            for (int i = index; i < rArgc; ++i)
                arg_ary[i+num] = argv[i];
            if (rAlloc)
                std::free(argv);
            rAlloc  = true;
            rArgv   = arg_ary;
            argv    = arg_ary;
            rArgc   += num;
            num     = index;
        }
        if (!rc)
            std::free(arg_ary);
        return rc;
    }

}   // _detail


//  -   -   -   -   -   -   -   -   -   -   -   -   -   -

/// Get command line arguments.
template< unsigned int F=3, typename C=char>
class cmd_line_args {
    enum flag { use_opt_next_arg = 1, enable_short_opt = 2, clr_opt_arg = 4 };
public:
    typedef C char_type;

    cmd_line_args(int argc, char_type* argv[]) ZATU_NOEXCEPT
        : argv_(argv), arg_(&nil_), arg_0_(0), argc_(argc), index_(1)
        , alloc_(false), enable_opt_(true), sub_opt_(false)
        , short_idx_(0), pre_short_idx_(0), nil_(0)
    { }

    ~cmd_line_args() {
        //if (alloc_) std::free(argv_);
    }

    int         argc() const ZATU_NOEXCEPT { return argc_; }
    char_type** argv() ZATU_NOEXCEPT { return argv_; }

    bool has_arg() const ZATU_NOEXCEPT { return (index_ < argc_); }

    void disable_opt() ZATU_NOEXCEPT { enable_opt_ = false; }

    // @return true:-option false:other
    bool prepare_get() ZATU_NOEXCEPT;

    char_type* get_arg() ZATU_NOEXCEPT { return arg_; }
    char_type* get_arg_0() ZATU_NOEXCEPT { return arg_0_; }

    bool get_opt(char_type const* opt) ZATU_NOEXCEPT {
        if ZATU_CONSTEXPR17 (F & enable_short_opt) {
            if (short_idx_)
                return false;
        }
        char_type* p = get_opt1(opt);
        return p != NULL && *p == 0;
    }

    bool get_opt(char_type const* opt, bool& b) ZATU_NOEXCEPT {
        char_type* p = get_opt1(opt);
        if (p) b = *p != '-';
        return p != NULL;
    }

    template<class U>
    bool get_opt(char_type const* opt, U& u) { return get_opt(opt, u, true); }

    template<class U>
    bool get_opt(char_type const* opt, U& u, bool next_arg) {
        char_type* p = get_opt1(opt);
        if (p) {
            if ZATU_CONSTEXPR17 (F & use_opt_next_arg) {
                from_strz(u, get_opt_arg(p, next_arg));
            } else {
                from_strz(u, p);
            }
        }
        return p != NULL;
    }

    bool get_opt2(char_type const* opt1, char_type const* opt2) ZATU_NOEXCEPT {
        return get_opt(opt1) || get_opt(opt2);
    }

    bool get_opt2(char_type const* opt1, char_type const* opt2, bool& b) ZATU_NOEXCEPT {
        return get_opt(opt1, b) || get_opt(opt2, b);
    }

    template<typename U>
    bool get_opt2(char_type const* opt1, char_type const* opt2, U& u) {
        return get_opt2(opt1, opt2, u, true);
    }

    template<typename U>
    bool get_opt2(char_type const* opt1, char_type const* opt2, U& u, bool next_arg) {
        return get_opt(opt1, u, next_arg) || get_opt(opt2, u, next_arg);
    }

    bool get_opt(char_type c) ZATU_NOEXCEPT;

    bool get_opt(char_type c, bool& b) ZATU_NOEXCEPT {
        if ZATU_CONSTEXPR17 (F & enable_short_opt) {
            if (get_opt(c)) {
                b = *arg_ != '-';
                if (!b)
                    ++arg_;
                return true;
            }
        } else {
            assert(F & enable_short_opt);
        }
        return false;
    }

    template<class U>
    bool get_opt(char_type c, U& u) { return get_opt(c, u, true); }

    template<class U>
    bool get_opt(char_type c, U& u, bool next_arg) {
        if ZATU_CONSTEXPR17 (F & enable_short_opt) {
            if (get_opt(c)) {
                if (*arg_ == C('='))
                    ++arg_;
                if ZATU_CONSTEXPR17 (F & use_opt_next_arg) {
                    from_strz(u, get_opt_arg(arg_, next_arg));
                } else {
                    from_strz(u, arg_);
                }
                short_idx_ = 0;
                arg_ = &nil_;
                return true;
            }
        } else {
            assert(F & enable_short_opt);
        }
        return false;
    }

    bool get_opt2(char_type c, char_type const* opt) ZATU_NOEXCEPT {
        if ZATU_CONSTEXPR17 (F & enable_short_opt) {
            return get_opt(c) || get_opt(opt);
        } else {
            assert(F & enable_short_opt);
            return false;
        }
    }

    bool get_opt2(char_type c, char_type const* opt, bool& b) ZATU_NOEXCEPT {
        if ZATU_CONSTEXPR17 (F & enable_short_opt) {
            return get_opt(c, b) || get_opt(opt, b);
        } else {
            assert(F & enable_short_opt);
            return false;
        }
    }

    template<typename U>
    bool get_opt2(char_type c, char_type const* opt, U& u) {
        return get_opt2(c, opt, u, true);
    }

    template<typename U>
    bool get_opt2(char_type c, char_type const* opt, U& u, bool next_arg) {
        return get_opt(c, u, next_arg) || get_opt(opt, u, next_arg);
    }

    void reset() ZATU_NOEXCEPT {
        index_ = 1;
        if ZATU_CONSTEXPR17 (F & clr_opt_arg)
            clear_opt_args<void>();
    }

    bool get_first_ch(C ch) ZATU_NOEXCEPT {
        if (*arg_ == ch) {
            ++arg_;
            return true;
        }
        return false;
    }

    template<typename K> bool insert_response_str(K const* s) ZATU_NOEXCEPT {  // K=char
        C const* e = s + std::char_traits<K>::length(s);
        return _detail::insert_str_to_args(s, e, index_, argc_, argv_, alloc_);
    }

    template<typename S> bool insert_response_str(S const& s, typename S::value_type* = NULL) ZATU_NOEXCEPT {  // S=string
        if (s.empty()) return false;
        return _detail::insert_str_to_args(&s[0], &s[s.size()], index_, argc_, argv_, alloc_);
        //return _detail::insert_str_to_args(*s.begin(), &*s.end(), index_, argc_, argv_, alloc_);
    }

    template<typename K> bool replace_response_str(K const* s) ZATU_NOEXCEPT {
        erase_current_arg<void>();
        return insert_response_str(s);
    }
    template<typename S> bool replace_response_str(S const& s) ZATU_NOEXCEPT {
        erase_current_arg<void>();
        return insert_response_str(s);
    }

private:
    C*      get_opt1(C const* opt) ZATU_NOEXCEPT;
    C*      get_opt_arg(C* opt_arg, bool next_arg) ZATU_NOEXCEPT;

    template<class DMY>
    void    clear_opt_args() ZATU_NOEXCEPT {
        if ZATU_CONSTEXPR17 (F & clr_opt_arg) {
            int argc = argc_;
            int j = 1;
            C** argv = argv_;
            for (int i = 1; i < argc; ++i) {
                if (argv[i]) {
                    if (j < i)
                        argv[j] = argv[i];
                    ++j;
                }
            }
            argc_ = j;
            while (j < argc)
                argv[j++] = NULL;
        }
    }

    template<class DMY>
    void    erase_current_arg() ZATU_NOEXCEPT {
        if (index_) {
            --index_;
            if (index_ && sub_opt_)
                --index_;
        }
        for (std::size_t i = index_ + 1; i < argc_; ++i) {
            argv_[i - 1] = argv_[i];
        }
        if (argc_) {
            --argc_;
            argv_[argc_] = NULL;
        }
    }

private:
    char_type**     argv_;
    char_type*      arg_;
    char_type*      arg_0_;
    int             argc_;
    int             index_;
    bool            alloc_;
    bool            enable_opt_;
    bool            sub_opt_;
    unsigned char   short_idx_;
    unsigned char   pre_short_idx_;
    char_type       nil_;
};

template<unsigned int F, typename C>
bool cmd_line_args<F,C>::prepare_get() ZATU_NOEXCEPT {
    assert(index_ < argc_);
    if (short_idx_) {
        if (*arg_) {
            if (pre_short_idx_ < short_idx_) {
                pre_short_idx_ = short_idx_;
                return true;
            }
            assert(pre_short_idx_ < short_idx_);
        }
        short_idx_ = 0;
    }
    sub_opt_ = false;
    pre_short_idx_ = short_idx_;
    arg_ = arg_0_ = argv_[index_++];
    bool rc = enable_opt_ && arg_ && *arg_ == '-';
    if ZATU_CONSTEXPR17 (F & clr_opt_arg) {
        if (rc)
            argv_[index_-1] = NULL;
    }
    return  rc;
}

template<unsigned int F, typename C>
bool cmd_line_args<F,C>::get_opt(char_type c) ZATU_NOEXCEPT {
    if ZATU_CONSTEXPR17 (F & enable_short_opt) {
        if (!c)
            return false;
        if (short_idx_) {
            if (*arg_ == c) {
                if (short_idx_ < 255)
                    ++short_idx_;
                ++arg_;
                return true;
            }
        } else if (*arg_ == C('-') && arg_[1] == c) {
            short_idx_ = 1;
            arg_ += 2;
            return true;
        }
    } else {
        assert(F & enable_short_opt);
    }
    return false;
}

template<unsigned int F, typename C>
C* cmd_line_args<F,C>::get_opt1(C const* opt) ZATU_NOEXCEPT {
    if (opt == NULL)
        return NULL;
    std::size_t opt_len = std::char_traits<C>::length(opt);
    if (std::char_traits<C>::compare(arg_, opt, opt_len) == 0) {
        C* p = arg_ + opt_len;
        if (*p == C('='))
            ++p;
        return p;
    }
    return NULL;
}

template<unsigned int F, typename C>
C* cmd_line_args<F,C>::get_opt_arg(C* opt_arg, bool next_arg) ZATU_NOEXCEPT {
    if ZATU_CONSTEXPR17 (F & use_opt_next_arg) {
        assert(opt_arg != 0);
        sub_opt_ = false;
        if (next_arg && *opt_arg == 0 && index_ < argc_) {
            sub_opt_ = true;
            opt_arg = argv_[index_++];
            if ZATU_CONSTEXPR17 (F & clr_opt_arg)
                argv_[index_-1] = NULL;
        }
    }
    return opt_arg;
}

namespace cmd_line_args_util {}

}   // zatu


//  -   -   -   -   -   -   -   -   -   -   -   -   -
//
#ifdef ZATU_USE_CMD_LINE_ARGS_UTIL

#if defined(_WIN32) || defined(ZATU_DOS)
#include <io.h>
#else
#include <unistd.h>
#endif
#include <fcntl.h>
#include <sys/stat.h>

namespace zatu {
    // Excerpt from other libraries.
    namespace cmd_line_args_util {
        template<typename C>
        C*   fname_base(C const* p) {
            C const *adr = p;
            while (*p) {
                unsigned c = *p++;
               #if defined(_WIN32) || defined(ZATU_DOS)
                if (c == ':' || c == '/' || c == '\\') adr = p;
               #else
                if (c == ':' || c == '/') adr = p;
               #endif
            }
            return (C*)adr;
        }

        template<typename C>
        C const* fname_ext(C const* p) {
            p = fname_base(p);
            char const* e = strrchr(p, '.');
            return e ? e : "";
        }

        template<typename S>
        void str_replace(S& str, typename S::value_type old_c, typename S::value_type new_c) {
            typedef typename S::value_type C;
            typename S::iterator s = str.begin();
            typename S::iterator e = str.end();
            while (s != e) {
                if (*s == old_c)
                    *s = new_c;
                ++s;
            }
        }

        //
        inline std::size_t file_size(const char* fpath, std::size_t err_size=std::size_t(-1)) {
         #if defined(_WIN32) || defined(ZATU_DOS)
            struct _stat st;
            int   rc = ::_stat(fpath, &st);
         #else
            struct stat st;
            int   rc = ::stat(fpath, &st);
         #endif
            return (rc == 0) ? std::size_t(st.st_size) : err_size;
        }

        inline bool file_exist(char const* fpath) {
         #if defined(_WIN32) || defined(ZATU_DOS)
            return ::_access(fpath, 0) == 0;
         #else
            struct stat st;
            return ::stat(fpath, &st) == 0;
         #endif
        }

        namespace _detail {
            template<typename C>
            bool file_load_sub(C const* fname, void* dst, std::size_t bytes, std::size_t max_bytes) {
                if (fname == NULL || dst == NULL || bytes == 0)
                    return false;
                if (max_bytes && bytes > max_bytes)
                    bytes = max_bytes;
             #if defined(_WIN32) || defined(ZATU_DOS)
                int fd = ::_open(fname, _O_RDONLY|_O_BINARY);
                if (fd == -1)
                    return false;
                std::size_t rbytes = ::_read(fd, dst, (unsigned)bytes);
                _close(fd);
             #else
                int fd = ::open(fname, O_RDONLY);
                if (fd == -1)
                    return false;
                std::size_t rbytes = read(fd, dst, bytes);
                close(fd);
             #endif
                return rbytes == bytes;
            }
        }
        template<class V>
        bool file_load(char const* fname, V& v, std::size_t max_size = 0) {
            v.clear();
            std::size_t bytes = file_size(fname);
            if (bytes == std::size_t(-1))
                return false;
            bool rc = true;
            std::size_t len = std::size_t((bytes + sizeof(v[0]) - 1) / sizeof(v[0]));
            v.resize(len + 1);
            std::size_t max_bytes = max_size * sizeof(v[0]);
            rc &= _detail::file_load_sub(fname, &v[0], bytes, max_bytes);
            v.resize(len);
            return rc;
        }
        template<class STR> STR
        file_load(char const* fname, std::size_t max_size = 0, bool* pRetCode=0) {
            STR buf;
            bool rc = file_load(fname, buf, max_size);
            if (pRetCode)
                *pRetCode = rc;
            return buf;
        }

        template< unsigned int F=3, typename C=char> bool
        cmd_line_args_insert_res_file(cmd_line_args<F,C>& args, char const* fname) {
            if (file_exist(fname)) {
                std::basic_string<C> tmp;
                if (file_load(fname, tmp))
                    return args.insert_response_str(tmp);
            }
            return false;
        }

        template< unsigned int F=3, typename C=char> bool
        cmd_line_args_replace_res_file(cmd_line_args<F,C>& args, char const* fname) {
            if (file_exist(fname)) {
                std::basic_string<C> tmp;
                if (file_load(fname, tmp))
                    return args.replace_response_str(tmp);
            }
            return false;
        }

    }   // cmd_line_args_util
}   // zatu

#endif  // ZATU_USE_CMD_LINE_ARGS_UTIL

#endif  // ZATU_CMDLINEARGS_HPP_INCLUDED
