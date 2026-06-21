/**
 * @file esupath.cpp
 * @brief Show/add/remove the PATH of the SYSTEM/USER (Windows)
 * @author Masashi Kitamura ( https://github.com/tenk-a/ )
 * @date 2025-2026
 */
#include <windows.h>
#include <shellapi.h>
#include <cstdio>
#include <cctype>
#include <string>
#include <vector>
#include <algorithm>
#include <stdarg.h>
#define ZATU_USE_CMD_LINE_ARGS_UTIL
#include "cmd_line_args.hpp"

#ifdef _MSC_VER
 #pragma comment(lib, "Advapi32.lib")
 #pragma comment(lib, "User32.lib")
 #pragma comment(lib, "Shell32.lib")
#endif

#define USE_JP

#if defined(USE_JP)
//static wchar_t const usage_jp[] = L""
static char const usage_jp[] = ""
       "Usage: esupath [オプション] <dir...>\n"
       "  環境変数 PATH の表示/追加/削除.\n"
       "[オプション]\n"
       " -l --list              PATH のディレクトリ一覧出力.\n"
       " -r --remove  <DIR...>  ディレクトリを削除. ワイルドカード指定可.\n"
       " -p --prepend <DIR...>  PATH の先頭にディレクトリ追加.\n"
       " -a --append  <DIR...>  PATH の最後にディレクトリ追加.\n"
       " -u --user              ユーザー環境変数を対象(レジストリ操作)\n"
       " -s --system            システム環境変数を対象(レジストリ操作)\n"
       " -e --env               現在プロセスの環境変数を対象.\n"
       " -b --batch <FILE>      現在プロセスの環境変数を対象、変更結果はバッチ出力.\n"
       " -y --yes               書き込み前のキー入力待ちをスキップ.\n"
       "    --var <NAME>        PATH でなく対象環境変数を <NAME> に変更.\n"
       "    --delete-var        --var で指定した環境変数を削除.\n"
       "    --silent            確認のための編集前後の内容表示や入力待ちを行わない.\n"
       " @file                  file からコマンドライン引数を取得.\n"
       "\n"
       "# PATHから -r,-p,-a で指定のディレクトリを削除した後-p先頭-a最後への追加を行う.\n"
       "# -r のワイルドカード文字は ? * **。* は / \\ にマッチせず、** はマッチする.\n"
       "# ダブりは後のモノが削除される.\n"
       "# 直接編集できるのは SYSTEM/USER レジストリのみ.\n"
       "# 現プロセスの環境変数へは編集結果を反映できない.\n"
       "# 現在の環境変数へ反映するには -b で出力したバッチを実行のこと.\n"
       ;
#endif

static char const usage_en[] = ""
       "Usage: esupath [options] <dir...>\n"
       "  Show/add/remove directories in the PATH environment variable.\n"
       "[options]\n"
       " -l --list              List directories in PATH.\n"
       " -r --remove  <DIR...>  Remove directories. Wildcards are allowed.\n"
       " -p --prepend <DIR...>  Add directories to the beginning of PATH.\n"
       " -a --append  <DIR...>  Add directories to the end of PATH.\n"
       " -u --user              Target the user environment variable.\n"
       " -s --system            Target the system environment variable.\n"
       " -e --env               Target the current process environment.\n"
       " -b --batch <FILE>      Target current env. and write a batch file.\n"
       " -y --yes               Do not wait for a key before writing.\n"
       "    --var <NAME>        Use NAME instead of PATH.\n"
       "    --delete-var        Delete the variable named by --var.\n"
       "    --silent            Do not show before/after or wait for input.\n"
       " @file                  Read command line arguments from file.\n"
       "\n"
       "# Paths specified by -r, -p, and -a are removed from PATH first.\n"
       "# Then -p paths are added to the beginning, and -a paths to the end.\n"
       "# Wildcards for -r are ?, *, and **. * does not match / or \\.\n"
       "# ** matches / and \\.\n"
       "# Duplicate paths are removed; the earlier path is kept.\n"
       "# Only SYSTEM/USER registry variables can be edited directly.\n"
       "# The current process variable cannot be changed by this program.\n"
       "# Run the batch file written by -b to update the current shell.\n"
       ;

using namespace std;
using namespace zatu;

using   Char    = wchar_t;
using   String  = wstring;
using   StrVec  = vector<String>;
#define _C(x)   L##x
#define NameCmp _wcsicmp


namespace StrUtl {

    /// wchar_t string to UTF-8 string for win
    ///
    string wcsToUtf8(wstring const& ws) {
        string r;
        if (ws.empty())
            return r;
        int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), int(ws.size()), NULL, 0, NULL, NULL);
        r.reserve(len+1);
        r.resize(len);
        WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), int(ws.size()), &r[0], len, NULL, NULL);
        return r;
    }

    /// UTF-8 string to wchar_t string for win
    ///
    wstring utf8ToWcs(string const& ss) {
        wstring r;
        if (ss.empty())
            return r;
        int len = MultiByteToWideChar(CP_UTF8, 0, ss.c_str(), int(ss.size()), NULL, 0);
        r.reserve(len+1);
        r.resize(len);
        MultiByteToWideChar(CP_UTF8, 0, ss.c_str(), int(ss.size()), &r[0], len);
        return r;
    }

    /// ディレクトリ・セパレータ?
    ///
    bool isSep(uint32_t c) { return c == '/' || c == '\\'; }

    /// UTF16 での大文字の小文字化.
    ///
    uint32_t toLower(uint32_t c) { return (c < 0xffff) ? (uint32_t)(uintptr_t)CharLowerW((LPWSTR)(uintptr_t)c) : c; }

    /// Unicode 1文字取得.
    ///
    uint32_t getCh(Char const*& s) {
        uint32_t c = *s;
        if (c) {
            ++s;
            if (c >= 0xD800 && c <= 0xDBFF && *s)
                c = (c << 16) | uint32_t(*s++);
        }
        return c;
    }

    /// ディレクトリ名名比較. (最後の \\ / の有無を同一視)
    ///
    bool dirnameEqu(Char const* ptn, Char const* tgt) {
        Char const* tgt2 = tgt;
        uint32_t    tc   = getCh(tgt2);
        switch (*ptn) {
        case _C('\0'):
            if (isSep(tc) && *tgt2 == _C('\0'))
                return true;
            return tc == _C('\0');
        case _C('\\'):
        case _C('/'):
            if (ptn[1] == _C('\0') && tc == _C('\0'))
                return true;
            return isSep(tc) && dirnameEqu(ptn + 1, tgt2);
        default:
            uint32_t pc = getCh(ptn);
            pc = toLower(pc);
            tc = toLower(tc);
            return (pc == tc) && dirnameEqu(ptn, tgt2);
        }
    }

    /// ワイルドカード有のディレクトリ名名比較. (最後の \\ / の有無を同一視)
    ///
    bool dirnameMatch(Char const* ptn, Char const* tgt) {
        Char const* tgt2 = tgt;
        uint32_t    tc   = getCh(tgt2);
        switch (*ptn) {
        case _C('\0'):
            if (isSep(tc) && *tgt2 == _C('\0'))
                return true;
            return tc == _C('\0');
        case _C('\\'):
        case _C('/'):
            if (ptn[1] == _C('\0') && tc == _C('\0'))
                return true;
            return isSep(tc) && dirnameMatch(ptn + 1, tgt2);
        case _C('?'):
            return tc && !isSep(tc) && dirnameMatch(ptn + 1, tgt2);
        case _C('*'):
            if (*(ptn + 1) == '*')
                return dirnameMatch(ptn + 2, tgt) || (tc && dirnameMatch(ptn, tgt2));
            else
                return dirnameMatch(ptn + 1, tgt) || (tc && !isSep(tc) && dirnameMatch(ptn, tgt2));
        default:
            uint32_t pc = getCh(ptn);
            pc = toLower(pc);
            tc = toLower(tc);
            return (pc == tc) && dirnameMatch(ptn, tgt2);
        }
    }

    /// ワイルドカード文字を含むか?
    ///
    bool hasWildCardChar(Char const* fname) {
        while (*fname) {
            Char c = *fname++;
            if (c == _C('?') || c == _C('*'))
                return true;
        }
        return false;
    }

    /// 大文字小文字無視で一致判定.
    ///
    bool strEqu(String const& a, Char const* b) {
        return NameCmp(a.c_str(), b) == 0;
    }
    bool strEqu(String const& a, String const& b) {
        return NameCmp(a.c_str(), b.c_str()) == 0;
    }

    /// 二重引用符全除去.
    ///
    String stripQuotes(String const& s) {
        String r = s;
        r.erase(remove(r.begin(), r.end(), _C('"')), r.end());
        return r;
    }

    /// table に同じものがある?
    ///
    bool containsPaths(StrVec const& table, String const& path) {
        for (size_t i = 0; i < table.size(); ++i) {
            if (dirnameEqu(table[i].c_str(), path.c_str()))
                return true;
        }
        return false;
    }

    bool containsPathsWC(StrVec const& table, String const& path) {
        for (size_t i = 0; i < table.size(); ++i) {
            if (dirnameMatch(table[i].c_str(), path.c_str()))
                return true;
        }
        return false;
    }

    /// ';' 結合.
    ///
    String joinPaths(StrVec const& v) {
        String r;
        for (size_t i = 0; i < v.size(); ++i) {
            if (i)
                r += _C(';');
            r += v[i];
        }
        return r;
    }

    /// 複数パス文字列の分解.
    ///
    void splitPaths(StrVec& dst, StrVec const& src) {
        for (auto& it : src) {
            String tmp = stripQuotes(it);
            if (!tmp.empty())
                dst.push_back(tmp);
        }
    }

    /// 複数パス文字列の分解.
    ///
    StrVec splitPaths(StrVec const& src) {
        StrVec dst;
        splitPaths(dst, src);
        return dst;

    }

    /// 複数パス文字列の分解.
    ///
    void splitPaths(StrVec& dst, String const& src) {
        size_t i  = 0;
        size_t j;
        if (src.empty())
            return;
        while ((j = src.find(_C(';'), i)) != String::npos) {
            if (j > i) {
                String s = stripQuotes(src.substr(i, j - i));
                if (!s.empty()) {
                    dst.push_back(s);
                }
            }
            i = j + 1;
        }
        if (i < src.size()) {
            String s = stripQuotes(src.substr(i));
            if (!s.empty()) {
                dst.push_back(s);
            }
        }
    }

    /// 複数パス文字列の分解.
    ///
    StrVec splitPaths(String const& src) {
        StrVec dst;
        splitPaths(dst, src);
        return dst;

    }

    /// パス配列の各パスの " を削除.
    ///
    void normalizePaths(StrVec& paths) {
        StrVec t;
        for (size_t i = 0; i < paths.size(); ++i) {
            String s = stripQuotes(paths[i]);
            if (!s.empty())
                t.push_back(s);
        }
        paths.swap(t);
    }

    /// パス配列からダブりを削除.
    ///
    bool removeDupPaths(StrVec& paths) {
        size_t num = paths.size();
        for (int i = int(num); --i >= 0;) {
            Char const* tgt = paths[i].c_str();
            for (int j = i; --j >= 0;) {
                if (StrUtl::dirnameEqu(tgt, paths[j].c_str())) {
                    paths.erase(paths.begin() + i);
                    break;
                }
            }
        }
        return num != paths.size();
    }

    /// パス配列から、指定のパス(複数)を削除.
    ///
    bool removePaths(StrVec& dst, StrVec const& src) {
        size_t num = dst.size();
        for (int i = int(num); --i >= 0;) {
            Char const* tgt = dst[i].c_str();
            for (size_t j = 0; j < src.size(); ++j) {
                if (StrUtl::dirnameEqu(src[j].c_str(), tgt)) {
                    dst.erase(dst.begin() + i);
                    break;
                }
            }
        }
        return num != dst.size();
    }

    /// パス配列から、指定のパス(複数)をワイルドカード指定で削除.
    ///
    bool removePathsWC(StrVec& dst, StrVec const& src) {
        size_t num = dst.size();
        for (int i = int(num); --i >= 0;) {
            Char const* tgt = dst[i].c_str();
            for (size_t j = 0; j < src.size(); ++j) {
                if (StrUtl::dirnameMatch(src[j].c_str(), tgt)) {
                    dst.erase(dst.begin() + i);
                    break;
                }
            }
        }
        return num != dst.size();
    }

    /// コマンドライン先頭引数(argv[0])をスキップ.
    ///
    Char const* skipCmdLineArg0(Char const* cmdline) {
        Char const* p = cmdline;
        if (p) {
            bool  f = 0;
            while (*p && *p <= _C(' '))
                ++p;
            while (*p) {
                if (*p == _C('"')) {
                    f = !f;
                    ++p;
                    continue;
                }
                if (!f && *p && *p <= _C(' '))
                    break;
                ++p;
            }
            while (*p && *p <= _C(' '))
                ++p;
        }
        return p;
    }

}


namespace WinUtl {

    /// 管理者権限チェック.
    ///
    bool isAdmin() {
        BYTE  sid_buf[SECURITY_MAX_SID_SIZE];
        DWORD sid_size = sizeof(sid_buf);
        BOOL  is_memb  = FALSE;
        return CreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, sid_buf, &sid_size)
            && CheckTokenMembership(NULL, sid_buf, &is_memb)
            && is_memb == TRUE;
    }

    /// 管理者として再実行.
    ///
    bool restartAdmin() {
        Char exe[MAX_PATH];
        GetModuleFileNameW(NULL, exe, sizeof(exe)/sizeof(exe[0]));
        return (INT_PTR)ShellExecuteW(NULL, _C("runas"), exe, StrUtl::skipCmdLineArg0(GetCommandLineW()), NULL, SW_SHOWNORMAL) > 32;
    }

    /// 現在プロセスの環境変数から取得.
    ///
    bool readProcEnv(Char const* name, String& out) {
        DWORD   size    = GetEnvironmentVariableW(name, NULL, 0);
        if (size == 0)
            return false;
        vector<Char> buf(size);
        GetEnvironmentVariableW(name, buf.data(), size);
        out.assign(buf.data());
        return true;
    }

    /// レジストリから環境変数を読み取り.
    ///
    bool readRegEnv(Char const* name, bool sys, String &out, DWORD* pType=nullptr) {
        Char const* sub = sys ? _C("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment") : _C("Environment");
        HKEY        h   = 0;

        if (RegOpenKeyExW(sys ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, sub, 0, KEY_READ, &h) != ERROR_SUCCESS)
            return false;

        DWORD       t    = 0;
        DWORD       size = 0;

        if ( RegQueryValueExW(h, name, 0, &t, NULL, &size) != ERROR_SUCCESS || (t != REG_SZ && t != REG_EXPAND_SZ) ) {
            RegCloseKey(h);
            return false;
        }

        vector<Char> buf(size / sizeof (Char) + 1);

        if (RegQueryValueExW(h, name, 0, NULL, (LPBYTE) &buf[0], &size) != ERROR_SUCCESS) {
            RegCloseKey(h);
            return false;
        }

        buf[size / sizeof (Char)] = 0;
        out.assign(&buf[0]);
        RegCloseKey(h);
        if (pType)
            *pType = t;
        return true;
    }

    /// レジストリへ環境変数を書き込み.
    ///
    bool writeRegEnv(Char const* name, bool sys, String const& val, DWORD type = 0) {
        Char const* sub = sys ? L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment" : L"Environment";
        HKEY        h   = 0;
        if (type == 0)
            type = REG_EXPAND_SZ;
        if (RegOpenKeyExW(sys ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, sub, 0, KEY_SET_VALUE, &h) != ERROR_SUCCESS)
            return false;

        if (RegSetValueExW(h, name, 0, type, (const BYTE *) val.c_str(), (DWORD)((val.size() + 1) * sizeof(Char))) != ERROR_SUCCESS) {
            RegCloseKey(h);
            return false;
        }
        RegCloseKey(h);
        SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"Environment", SMTO_ABORTIFHUNG, 5000, NULL);
        return true;
    }

    /// レジストリから環境変数を削除.
    ///
    int deleteRegEnv(Char const* name, bool sys) {
        Char const* sub = sys ? L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment" : L"Environment";
        HKEY        h   = 0;

        LONG rc = RegOpenKeyExW(sys ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, sub, 0, KEY_SET_VALUE, &h);
        if (rc != ERROR_SUCCESS)
            return -1;

        rc = RegDeleteValueW(h, name);
        RegCloseKey(h);
        if (rc == ERROR_SUCCESS) {
            SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"Environment", SMTO_ABORTIFHUNG, 5000, NULL);
            return 1;
        }
        return (rc == ERROR_FILE_NOT_FOUND) ? 0 : -1;
    }

    /// wchar_t の argv を utf8な char argv に変換.
    ///
    char** wcsArgvToUtf8Argv(int argc, wchar_t* argv[]) {
        char**  dst = (char**) calloc(1, (argc + 1) * sizeof(char*));
        if (dst) {
            for (int i = 0; i < argc; ++i) {
                dst[i] = _strdup(StrUtl::wcsToUtf8(argv[i]).c_str());
                //printf("%s\n", dst[i]);
            }
        }
        return dst;
    }
}

/// ファイルパス名の配列.
class FilePaths {
    StrVec      paths_;
public:
    FilePaths() {}

    /// 配列(vector) 実態を取り出す(参照のみ).
    StrVec const& operator()() const { return paths_; }

    /// 繋げて一つの文字列にして返す.
    ///
    String  join() const { return StrUtl::joinPaths(paths_); }

    /// 指定ディレクトリ(複数)の追加.
    /// @param append   false なら先頭に、true なら 最後に追加.
    void add(String const& paths, bool append = true) {
        add(StrUtl::splitPaths(paths), append);
    }

    /// 指定ディレクトリ(複数)の追加.
    /// @param append   false なら先頭に、true なら 最後に追加.
    void add(StrVec const& src, bool append = true) {
        if (!src.empty()) {
            if (!append)
                paths_.insert(paths_.begin(), src.begin(), src.end());
            else
                paths_.insert(paths_.end(), src.begin(), src.end());

        }
    }

    /// 指定ディレクトリ(複数)の削除.
    /// @param wc ワイルドカード指定.
    bool remove(String const& src, bool wc=false) {
        return remove(StrUtl::splitPaths(src), wc);
    }

    /// 指定ディレクトリ(複数)の削除.
    /// @param wc ワイルドカード指定.
    bool remove(StrVec const& src, bool wc=false) {
        if (wc)
            return StrUtl::removePathsWC(paths_, src);
        else
            return StrUtl::removePaths(paths_, src);
    }

    /// ダブり削除.
    ///
    bool removeDup() {
        return StrUtl::removeDupPaths(paths_);
    }

 #if 0
    bool contains(String const& path) const {
        return StrUtl::containsPaths(paths_, path);
    }

    bool containsWC(String const& path) const {
        return StrUtl::containsPathsWC(paths_, path);
    }

    void normalize() {
        StrUtl::normalizePaths(paths_);
    }
 #endif
};


/// プログラム.
class App {
    enum { Ok = 0, Er = 1 };
    enum { TGT_PRC = 0x01, TGT_USR = 0x02, TGT_SYS = 0x04 };
    enum { OP_NONE, OP_REMOVE, OP_PREPEND, OP_APPEND };

    string      var_u8name_ = "PATH";
    String      var_name_   = _C("PATH");

    string      bat_fname_;
    FILE*       bat_file_   = nullptr;

    String      prc_str_;
    String      usr_str_;
    String      sys_str_;

    FilePaths   prc_paths_;
    FilePaths   usr_paths_;
    FilePaths   sys_paths_;

    FilePaths   removes_;
    FilePaths   prepends_;
    FilePaths   appends_;

    DWORD       usr_reg_typ_= 0;
    DWORD       sys_reg_typ_= 0;

    uint8_t     targets_    = 0;        // TGT_PRC|TGT_USR|TGT_SYS
    uint8_t     op_type_    = OP_NONE;
    bool        do_list_    = false;
    bool        do_del_var_ = false;
    bool        skip_wait_  = false;
    bool        silent_     = false;

    DWORD       sav_cp_     = GetConsoleOutputCP();

public:
    /// デフォルト・コンストラクタ.
    ///
    App() {
        ::SetConsoleOutputCP(65001);
    }

    /// デストラクタ.
    ///
    ~App() {
        if (bat_file_)
            fclose(bat_file_);
        ::SetConsoleOutputCP(sav_cp_);
    }

    /// メイン.
    ///
    int wmain(int argc, wchar_t *argv[]) {
        if (argc < 2)
            return usage();

        if (parseArgs(argc, argv) != Ok)
            return Er;

        var_u8name_ = StrUtl::wcsToUtf8(var_name_);

        // 変数そのものの削除処理.
        if (do_del_var_)
            return doDeleteVar();

        // 引数指定のパス名からダブりを削除.
        removes_.removeDup();
        prepends_.removeDup();
        appends_.removeDup();

        // 環境変数の内容を取得.
        WinUtl::readProcEnv(var_name_.c_str(),       prc_str_);
        WinUtl::readRegEnv(var_name_.c_str(), false, usr_str_, &usr_reg_typ_);
        WinUtl::readRegEnv(var_name_.c_str(), true , sys_str_, &sys_reg_typ_);

        prc_paths_.add(prc_str_);
        usr_paths_.add(usr_str_);
        sys_paths_.add(sys_str_);

     #if !defined(NDEBUG)
        printPaths(removes_ , "removes" , "");
        printPaths(prepends_, "prepends", "");
        printPaths(appends_ , "appends" , "");
     #endif

        // 一覧表示のみ.
        if (do_list_) {
            return doList();
        }

        if (targets_ == 0) {
            eprintf("When deleting or adding, -e,--env, -u,--user, -s,--system options are required.\n");
            return Er;
        }

        // SYSTEM を編集するときには admin 権限が必要.
        if ((targets_ & TGT_SYS) && !WinUtl::isAdmin())
            return WinUtl::restartAdmin() ? Ok : Er;

        // 環境変数の編集.
        return edit();
    }

private:
    /// 説明.
    ///
    int usage() {
     #if defined(USE_JP)
        if (::GetUserDefaultLCID() == 1041) {
         #if 0
            DWORD  len = 0;
            WriteConsoleW(GetStdHandle(STD_ERROR_HANDLE), usage_jp, (DWORD)wcslen(usage_jp), &len, NULL);
         #else
            //eprintf("%s", usage_jp);
            fwrite(usage_jp, 1, sizeof(usage_jp)-1, stderr);
         #endif
        } else
     #endif
        {
            eprintf("%s", usage_en);
        }
        return Er;
    }

    /// コマンドライン引数取得.
    ///
    int parseArgs(int argc, wchar_t* argv[]) {
        char** u8argv = WinUtl::wcsArgvToUtf8Argv(argc, argv);
        if (!u8argv)
            return Er;
        cmd_line_args<> args(argc, u8argv);
        while (args.has_arg()) {
            if (args.prepare_get()) {  // option.
                if (args.get_opt2('y', "--yes", skip_wait_)) {

                } else if (args.get_opt2('l', "--list", do_list_)) {

                } else if (args.get_opt2('r', "--remove")) {
                    op_type_ = OP_REMOVE;
                } else if (args.get_opt2('p', "--prepend")) {
                    op_type_ = OP_PREPEND;
                } else if (args.get_opt2('a', "--append")) {
                    op_type_ = OP_APPEND;
                } else if (args.get_opt2('e', "--env")) {
                    targets_   |= TGT_PRC;
                } else if (args.get_opt2('u', "--user")) {
                    targets_   |= TGT_USR;
                } else if (args.get_opt2('s', "--system")) {
                    targets_   |= TGT_SYS;
                } else if (args.get_opt2('b', "--batch", bat_fname_)) {
                    targets_   |= TGT_PRC;
                } else if (args.get_opt("--var", var_u8name_)) {
                    if (!validVarName(var_u8name_)) {
                        eprintf("Invalid variable name : %s\n", var_u8name_.c_str());
                        return Er;
                    }
                    var_name_ = StrUtl::utf8ToWcs(var_u8name_);
                } else if (args.get_opt("--delete-var", do_del_var_)) {

                } else if (args.get_opt("--silent", silent_)) {

                } else if (args.get_opt2('h', "--help")) {
                    return usage();
                } else {
                    eprintf("Bad option %s\n\n", args.get_arg());
                    return usage();
                }
            } else if (*args.get_arg() == '@') {
                char const* fname = args.get_arg()+1;
                string      tmp;
                if (cmd_line_args_util::file_load(fname, tmp) == false) {
                    eprintf("%s : File open error.\n", fname);
                    return Er;
                }
                args.replace_response_str(tmp);
            } else { // file.
                switch (op_type_) {
                case OP_REMOVE : removes_.add( StrUtl::utf8ToWcs(args.get_arg())); break;
                case OP_PREPEND: prepends_.add(StrUtl::utf8ToWcs(args.get_arg())); break;
                case OP_APPEND : appends_.add( StrUtl::utf8ToWcs(args.get_arg())); break;
                default: eprintf("Files are only needed when using --remove, --prepend, or --append.\n"); return Er;
                }
            }
        }
        return Ok;
    }

    /// 一覧表示.
    ///
    int doList() {
        if (op_type_) {
            eprintf("-l,--list cannot be specified together with other operations.\n");
            return Er;
        }
        if (targets_ == 0)
            targets_ = TGT_PRC|TGT_USR|TGT_SYS;
        printEnv(targets_, "");
        return Ok;
    }

    /// 環境変数の削除.
    ///
    int doDeleteVar() {
        if (op_type_) {
            eprintf("--del-var cannot be specified together with other operations.\n");
            return Er;
        }
        if ((targets_ & (TGT_USR | TGT_SYS | TGT_PRC)) == 0) {
            eprintf("--delete-var requires --user, --system, or --env.\n");
            return Er;
        }
        if (targets_ & TGT_PRC) {
            bat_printf("set \"%s=\"\n", var_u8name_.c_str());
        }
        if (targets_ & TGT_USR) {
            if (delVar(var_name_, false) != Ok)
                return Er;
        }
        if (targets_ & TGT_SYS) {
            if (delVar(var_name_, true) != Ok)
                return Er;
        }
        return Ok;
    }

    /// 環境変数 name 削除.
    ///
    int delVar(String const& name, bool sys) {
        if (StrUtl::strEqu(name, _C("PATH"))) {
            eprintf("'PATH' cannot be deleted.\n");
            return Er;
        }
        if (sys && !WinUtl::isAdmin())
            return WinUtl::restartAdmin() ? Ok : Er;

        int rc = WinUtl::deleteRegEnv(name.c_str(), sys);
        if (rc < 0) {
            eprintf("Failed to delete variable.\n");
        } else if (rc == 0) {
            eprintf("Variable not found.\n");
        } else {
            eprintf("Deleted variable\n");
        }
        return (rc >= 0) ? Ok : Er;
    }

    /// PATH 内容の編集.
    ///
    int edit() {
        if (!silent_)
            printEnv(targets_, "### BEFORE ###\n");

        if (removes_().empty() && prepends_().empty() && appends_().empty()) {
            eprintf("No directories.\n");
            return Er;
        }

        if ((targets_ & (TGT_USR|TGT_SYS)) == (TGT_USR|TGT_SYS) && (!prepends_().empty() || !appends_().empty())) {
            eprintf("When adding a path, --user and --system cannot be specified together.\n");
            return Er;
        }

        if (targets_ & TGT_PRC) {
            prc_paths_.remove(removes_(), true);
            prc_paths_.remove(prepends_());
            prc_paths_.remove(appends_());
            prc_paths_.add(prepends_(), false);
            prc_paths_.add(appends_() , true);
        }
        if (targets_ & TGT_USR) {
            usr_paths_.remove(removes_(), true);
            usr_paths_.remove(prepends_());
            usr_paths_.remove(appends_());
            usr_paths_.add(prepends_(), false);
            usr_paths_.add(appends_() , true);
        }
        if (targets_ & TGT_SYS) {
            sys_paths_.remove(removes_(), true);
            sys_paths_.remove(prepends_());
            sys_paths_.remove(appends_());
            sys_paths_.add(prepends_(), false);
            sys_paths_.add(appends_() , true);
        }

        prc_paths_.removeDup();
        usr_paths_.removeDup();
        sys_paths_.removeDup();

        prc_str_    = prc_paths_.join();
        usr_str_    = usr_paths_.join();
        sys_str_    = sys_paths_.join();

        if (!silent_)
            printEnv(targets_, "\n### AFTER ###\n");

        if (!skip_wait_ && !silent_) {
            if (keyWait() == false) {
                eprintf("Stop.\n");
                return Er;
            }
        }

        bool rc0 = true, rc1 = true, rc2 = true;
        if (targets_ & TGT_PRC) {
            bat_printf("set \"%s=%s\"\n", var_u8name_.c_str(), StrUtl::wcsToUtf8(prc_str_).c_str());
        }
        if (targets_ & TGT_USR) {
            rc1 = WinUtl::writeRegEnv(var_name_.c_str(), false, usr_str_, usr_reg_typ_);
        }
        if (targets_ & TGT_SYS) {
            rc2 = WinUtl::writeRegEnv(var_name_.c_str(), true, sys_str_, sys_reg_typ_);
        }

        if (!rc0 | !rc1 | !rc2) {
            eprintf("\nFailed to write to registry.\n");
            return Er;
        }

        if (!silent_)
            eprintf("\nDone.\n");

        return Ok;
    }

    /// １キー入力待ち.
    ///
    bool keyWait() {
        eprintf("\nPress 'Y' or enter-key to continue. ");
        int c = fgetc(stdin);
        eprintf("\n\n");
        return (c == '\r' || c == '\n' || c == 'y' || c == 'Y');
    }

    /// 環境変数 の PATH 一覧表示.(全種)
    ///
    void printEnv(uint8_t targets, char const* title) {
        if (title && *title)
            printf("%s", title);
        if (targets & TGT_SYS)
            printPaths(sys_paths_, "SYSTEM Registry", var_u8name_.c_str());
        if (targets & TGT_USR)
            printPaths(usr_paths_, "USER Registry"  , var_u8name_.c_str());
        if (targets & TGT_PRC)
            printPaths(prc_paths_, "Current Process", var_u8name_.c_str());
    }

    /// 環境変数 の PATH 一覧表示.(1種類)
    ///
    void printPaths(FilePaths const& paths, char const* label, char const* vname) {
        printf("[%s] %s ... size: %u\n", label, vname, unsigned(paths().size()));
        for (size_t i = 0; i < paths().size(); ++i) {
            printf("\"%s\"\n", StrUtl::wcsToUtf8(paths()[i]).c_str() );
        }
        printf("\n");
    }

    /// 変数名チェック. 英数のみ.
    ///
    bool validVarName(string const& s) {
        if (s.empty())
            return false;
        unsigned c = s[0];
        if (c >= 0x80 || !(isalpha(char(c)) || c == '_'))
            return false;
        for (size_t i = 1; i < s.size(); ++i) {
            c = s[i];
            if (c >= 0x80 || !(isalnum(char(c)) || c == '_'))
                return false;
        }
        return true;
    }

    /// バッチファイルへ出力.
    ///
    void bat_printf(char const* fmt, ...) {
        if (!bat_file_ && !bat_fname_.empty()) {
            bat_file_ = fopen(bat_fname_.c_str(), "wt");
            if (!bat_file_) {
                eprintf("%s : [ERROR] File open error.\n", bat_fname_.c_str());
            }
        }
        va_list  args;
        va_start(args, fmt);
        if (bat_file_)
            vfprintf(bat_file_, fmt, args);
        else
            vfprintf(stdout, fmt, args);
        va_end(args);
    }

    /// エラー出力.
    ///
    static int eprintf(char const* fmt, ...) {
        va_list  args;
        va_start(args, fmt);
        int      n  = vfprintf(stderr, fmt, args);
        va_end(args);
        return n;
    }

};

/// main エントリ.
///
int wmain(int argc, wchar_t *argv[]) {
    return App().wmain(argc, argv);
}
