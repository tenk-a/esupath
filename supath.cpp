#include <windows.h>
#include <shellapi.h>
#include <cstdio>
#include <cctype>
#include <string>
#include <vector>
#include <algorithm>

#ifdef _MSC_VER
 #pragma comment(lib, "Advapi32.lib")
 #pragma comment(lib, "User32.lib")
 #pragma comment(lib, "Shell32.lib")
#endif

using namespace std;

class App {
    enum { Ok = 0, Er = 1 };
    DWORD   sav_cp_;

public:
    // 説明.
    int usage() {
        if (GetUserDefaultLCID() == 1041) {
            printf("Usage: supath [オプション] <dir...>\n"
                   "[オプション]\n"
                   " -l --list           PATH のディレクトリ一覧出力.\n"
                   " -p --prepend <dir...>  PATH の先頭にディレクトリ追加.\n"
                   " -a --append  <dir...>  PATH の最後にディレクトリ追加.\n"
                   " -r --remove  <dir...>  ディレクトリを削除.\n"
                   " -u --user              ユーザー環境変数(デフォルト)\n"
                   " -s --system            システム環境変数.\n"
                   "    --var <NAME>        対象環境変数を<NAME>に変更(デフォルト: PATH)\n"
                   "    --delete-var        --var で指定した環境変数をレジストリから削除\n"
                // "    --backup            変更前のPATH内容をテキストに出力.\n"
                   "\n"
                   "Windows レジストリの SYSTEM/USER 環境変数 PATH の表示/追加/削除.\n"
                   "注意: レジストリに対する操作のみで、現在のプロセスの PATH は非操作.\n"
            );
        } else {
            printf("Usage: supath [options] <dir...>\n"
                   "[option]\n"
                   " -l --list              list directories.\n"
                   " -p --prepend <dir...>  add directories \n"
                   " -a --append  <dir...>  add directories \n"
                   " -r --remove  <dir...>  remove directories \n"
                   " -u --user              user env.(default)\n"
                   " -s --system            system env.\n"
                   "    --var <NAME>        target env var (default: PATH)\n"
                   "    --delete-var        delete the variable --var's NAME from registry\n"
                // "    --backup            backup current value.\n"
                   "\n"
                   "Show/add/remove the PATH of the SYSTEM/USER\n"
                   "environment variables in the Windows Registry.\n"
                   "Note: This operates on the registry;\n"
                   "      it does not read the current process PATH.\n"
            );
        }
        return Er;
    }

    //
    App() : sav_cp_(GetConsoleOutputCP()) {
        SetConsoleOutputCP(65001);
    }

    //
    ~App() {
        SetConsoleOutputCP(sav_cp_);
    }

    // メイン.
    int wmain(int argc, wchar_t *argv[]) {
        if (argc < 2)
            return usage();

        bool    do_list       = false;
        bool    do_prepend    = false;
        bool    do_append     = false;
        bool    do_remove     = false;
        bool    sys           = false;
        bool    usr           = false;
        bool    backup        = false;
        bool    do_delete_var = false;
        wstring var_name      = L"PATH";
        wstring delete_var_name;
        vector<wstring> targets;

        for (int i = 1; i < argc; ++i) {
            std::wstring a = argv[i];

            if (!a.empty() && a[0] == L'-') {
                if (a == L"-l" || a == L"--list") {
                    do_list = true;
                } else if (a == L"-p" || a == L"--prepend") {
                    do_prepend = true;
                } else if (a == L"-a" || a == L"--append") {
                    do_append  = true;
                } else if (a == L"-r" || a == L"--remove") {
                    do_remove  = true;
                } else if (a == L"-u" || a == L"--user") {
                    usr = true;
                } else if (a == L"-s" || a == L"--system") {
                    sys     = true;
                } else if (a == L"--var" && i + 1 < argc) {
                    var_name = argv[++i];
                    if (!validVarName(var_name)) {
                        printf("Invalid variable name\n");
                        return Er;
                    }
                } else if ((a == L"--delete-var" || a == L"--delete_var")) {
                    do_delete_var = true;
                } else if (a == L"--backup") {
                    backup = true;                  // 隠し
                } else if (a == L"-?" || a == L"-h" || a == L"--help") {
                    return usage();
                } else {
                    printf("Unknown option: %s\n", wcsToUtf8(a).c_str());
                    return Er;
                }
            } else {
                appendPaths(targets, a);
            }
        }

        // 排他チェック（操作はどれか一つだけ）
        int op_count = 0;
        if (do_list)       ++op_count;
        if (do_prepend)    ++op_count;
        if (do_append)     ++op_count;
        if (do_remove)     ++op_count;
        if (do_delete_var) ++op_count;
        if (op_count != 1) {
            printf("Specify exactly one operation among --list/--prepend/--append/--remove/--delete-var\n");
            return Er;
        }

        // 変数そのものの削除処理.
        if (do_delete_var)
            return deleteVar(var_name, sys);

        // 現在値読み出し.
        wstring    cur;
        if ( !readEnv(var_name.c_str(), sys, cur) )
            cur.clear();

        if (backup)
            backupEnvToTxt(cur);

        // 一覧表示.
        if (do_list) {
            if (!sys && !usr) {
                wstring  env;
                if ( readEnv(var_name.c_str(), true, env) )
                    listPath(env, "SYSTEM");
            }
            listPath(cur, sys ? "SYSTEM" : "USER");
            return Ok;
        }

        normalizeTargets(targets);
        if (targets.empty()) {
            printf("No directories.\n");
            return Er;
        }

        vector<wstring> parts = splitPath(cur);

        // 追加（-p/-a）
        if (do_prepend || do_append) {
            vector<wstring> to_add;
            for (size_t t = 0; t < targets.size(); ++t) {
                const wstring &cand = targets[t];
                if (!contains(parts, cand) && !contains(to_add, cand)) {
                    to_add.push_back(cand);
                }
            }

            if (to_add.empty()) {
                printf("Already in variable\n");
                return Ok;
            }

            if (sys && !isAdmin())
                return restartAdmin() ? Ok : Er;

            if (do_append)
                parts.insert(parts.end(), to_add.begin(), to_add.end());
            else
                parts.insert(parts.begin(), to_add.begin(), to_add.end());

            wstring  newv = joinPath(parts);

            if (!writeEnv(var_name.c_str(), sys, newv)) {
                printf("Failed to write environment variable\n");
                return Er;
            }

            printf("Added\n");
            return Ok;
        }

        // 削除（-r）
        if (do_remove) {
            bool any_removed = false;
            for (size_t i = 0; i < parts.size();) {
                if (contains(targets, parts[i])) {
                    parts.erase(parts.begin() + i);
                    any_removed = true;
                } else {
                    ++i;
                }
            }

            if (!any_removed) {
                printf("Not found in variable\n");
                return Ok;
            }

            if (sys && !isAdmin())
                return restartAdmin() ? Ok : Er;

            wstring  newv = joinPath(parts);

            if (!writeEnv(var_name.c_str(), sys, newv)) {
                printf("Failed to write environment variable\n");
                return Er;
            }

            printf("Removed\n");
            return Ok;
        }

        return usage();
    }

private:

    // PATH 一覧表示.
    void listPath(wstring const& env, char const* label) {
        printf("[%s %s]\n", label, "ENV");
        vector<wstring> v = splitPath(env);
        for (size_t i = 0; i < v.size(); ++i) {
            string u = wcsToUtf8(v[i]);
            printf( "\"%s\"\n", u.c_str() );
        }
    }

    // 環境変数 name 削除.
    int deleteVar(wstring const& name, bool sys) {
        if (nameEqu(name.c_str(), L"PATH")) {
            printf("PATH cannot be deleted.\n");
            return Er;
        }
        if (sys && !isAdmin())
            return restartAdmin() ? Ok : Er;

        if (!deleteEnv(name.c_str(), sys)) {
            printf("Failed to delete variable or not found\n");
            return Er;
        }
        printf("Deleted variable\n");
        return Ok;
    }

    // バックアップをUTF-8テキストで保存.
    void backupEnvToTxt(wstring const& env) {
        FILE *f = _wfopen(L"path_backup.txt", L"wb");
        if (f) {
            string u = wcsToUtf8(env);
            fwrite(u.c_str(), 1, u.size(), f);
            fclose(f);
            printf("Backup saved to path_backup.txt\n");
        }
    }

    // 管理者権限チェック.
    static bool isAdmin() {
        BYTE  sid_buf[SECURITY_MAX_SID_SIZE];
        DWORD sid_size = sizeof(sid_buf);
        BOOL  is_memb  = FALSE;
        return CreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, sid_buf, &sid_size)
            && CheckTokenMembership(NULL, sid_buf, &is_memb)
            && is_memb == TRUE;
    }

    // 管理者として再実行.
    static bool restartAdmin() {
        wchar_t exe[MAX_PATH];
        GetModuleFileNameW(NULL, exe, sizeof(exe)/sizeof(exe[0]));
        return (INT_PTR)ShellExecuteW(NULL, L"runas", exe, skipCmdLineArg0(GetCommandLineW()), NULL, SW_SHOWNORMAL) > 32;
    }

    // コマンドライン先頭引数(argv[0])をスキップ.
    static wchar_t const* skipCmdLineArg0(wchar_t const* cmdline) {
        wchar_t const* p = cmdline;
        if (p) {
            char  f = 0;
            while (*p && *p <= L' ')
                ++p;
            while (*p) {
                if (*p == L'"') {
                    f = !f;
                    ++p;
                    continue;
                }
                if (!f && *p && *p <= L' ')
                    break;
                ++p;
            }
            while (*p && *p <= L' ')
                ++p;
        }
        return p;
    }

    // レジストリから環境変数を読み取り.
    static bool readEnv(wchar_t const* name, bool sys, wstring &out) {
        wchar_t const*  sub = sys ? L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment" : L"Environment";
        HKEY            h   = 0;

        if (RegOpenKeyExW(sys ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, sub, 0, KEY_READ, &h) != ERROR_SUCCESS)
            return false;

        DWORD   t    = 0;
        DWORD   size = 0;

        if ( RegQueryValueExW(h, name, 0, &t, NULL, &size) != ERROR_SUCCESS || (t != REG_SZ && t != REG_EXPAND_SZ) ) {
            RegCloseKey(h);
            return false;
        }

        vector<wchar_t>    buf(size / sizeof (wchar_t) + 1);

        if (RegQueryValueExW(h, name, 0, NULL, (LPBYTE) &buf[0], &size) != ERROR_SUCCESS) {
            RegCloseKey(h);
            return false;
        }

        buf[size / sizeof (wchar_t)] = 0;
        out.assign(&buf[0]);
        RegCloseKey(h);
        return true;
    }

    // レジストリへ環境変数を書き込み.
    static bool writeEnv(wchar_t const* name, bool sys, wstring const& val) {
        wchar_t const*  sub = sys ? L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment" : L"Environment";
        HKEY            h   = 0;

        if (RegOpenKeyExW(sys ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, sub, 0, KEY_SET_VALUE, &h) != ERROR_SUCCESS)
            return false;

        if (RegSetValueExW(h, name, 0, REG_EXPAND_SZ, (const BYTE *) val.c_str(), (DWORD)((val.size() + 1) * sizeof(wchar_t))) != ERROR_SUCCESS) {
            RegCloseKey(h);
            return false;
        }
        RegCloseKey(h);
        SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"Environment", SMTO_ABORTIFHUNG, 5000, NULL);
        return true;
    }

    // レジストリから環境変数を削除.
    static bool deleteEnv(wchar_t const* name, bool sys) {
        wchar_t const*  sub = sys ? L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment" : L"Environment";
        HKEY            h   = 0;

        if (RegOpenKeyExW(sys ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, sub, 0, KEY_SET_VALUE, &h) != ERROR_SUCCESS)
            return false;

        LONG rc = RegDeleteValueW(h, name);
        RegCloseKey(h);
        SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"Environment", SMTO_ABORTIFHUNG, 5000, NULL);
        return (rc == ERROR_SUCCESS);
    }

    // ';' で分割、引用符除去して targets に追加.
    static void appendPaths(vector<wstring>& targets, wstring const& paths) {
        size_t i = 0;
        size_t j;
        if (paths.empty())
            return;
        while ((j = paths.find(L';', i)) != wstring::npos) {
            if (j > i) {
                wstring s = stripQuotes(paths.substr(i, j - i));
                if (!s.empty())
                    targets.push_back(s);
            }
            i = j + 1;
        }
        if (i < paths.size()) {
            wstring s = stripQuotes(paths.substr(i));
            if (!s.empty())
                targets.push_back(s);
        }
    }

    // ターゲット正規化.
    static void normalizeTargets(vector<wstring>& targets) {
        vector<wstring> t;
        for (size_t i = 0; i < targets.size(); ++i) {
            wstring s = stripQuotes(targets[i]);
            if (!s.empty())
                t.push_back(s);
        }
        targets.swap(t);
    }

    // table に同じものがある?
    static bool contains(vector<wstring> const& table, wstring const& s) {
        for (size_t i = 0; i < table.size(); ++i) {
            if (nameEqu(table[i], s.c_str()))
                return true;
        }
        return false;
    }

    // ';' 区切りで分割.
    static vector<wstring> splitPath(wstring const& path) {
        vector<wstring> v;
        size_t          i = 0;
        size_t          j;
        while ((j = path.find(L';', i)) != wstring::npos) {
            if (j > i)
                v.push_back(path.substr(i, j - i));
            i = j + 1;
        }
        if (i < path.size())
            v.push_back(path.substr(i));
        return v;
    }

    // ';' 結合.
    static wstring joinPath(const vector<wstring> &v) {
        wstring r;
        for (size_t i = 0; i < v.size(); ++i) {
            if (i)
                r += L";";
            r += v[i];
        }
        return r;
    }

    // 変数名チェック.
    static bool validVarName(wstring const& s) {
        if (s.empty())
            return false;
        wchar_t c = s[0];
        if (c >= 0x80 || !(isalpha(char(c)) || c == L'_'))
            return false;
        for (size_t i = 1; i < s.size(); ++i) {
            c = s[i];
            if (c >= 0x80 || !(isalnum(char(c)) || c == L'_'))
                return false;
        }
        return true;
    }

    // 大文字小文字無視で一致判定.
    static bool nameEqu(wstring const& a, wchar_t const* b) {
        return _wcsicmp(a.c_str(), b) == 0;
    }

    // 二重引用符全除去.
    static wstring stripQuotes(wstring const& s) {
        wstring r = s;
        r.erase(remove(r.begin(), r.end(), L'"'), r.end());
        return r;
    }

    // UTF-8 変換.
    static string wcsToUtf8(wstring const& ws) {
        if (ws.empty())
            return "";
        int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), int(ws.size()), NULL, 0, NULL, NULL);
        string r;
        r.reserve(len+1);
        r.resize(len);
        WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), int(ws.size()), &r[0], len, NULL, NULL);
        return r;
    }
};

// エントリ.
int wmain(int argc, wchar_t *argv[]) {
    return App().wmain(argc, argv);
}
