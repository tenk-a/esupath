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
    //
    int usage() {
        if (GetUserDefaultLCID() == 1041) {
            printf("Usage: supath [オプション] <dir>\n"
                   "[オプション]\n"
                   " -l --list           PATH のディレクトリ一覧出力.\n"
                   " -p --prepend <dir>  PATH の先頭にディレクトリ追加.\n"
                   " -a --append  <dir>  PATH の最後にディレクトリ追加.\n"
                   " -r --remove  <dir>  ディレクトリを削除.\n"
                   " -u --user           ユーザー環境変数(デフォルト)\n"
                   " -s --system         システム環境変数.\n"
                   "    --var <NAME>     対象環境変数を<NAME>に変更(デフォルト: PATH)\n"
                // "    --backup         変更前のPATH内容をテキストに出力.\n"
                   "\n"
                   "Windows レジストリの SYSTEM/USER 環境変数 PATH の表示/追加/削除.\n"
                   "注意: レジストリに対する操作のみで、現在のプロセスの PATH は非操作.\n"
            );
        } else {
            printf("Usage: supath [options] <dir>\n"
                   "[option]\n"
                   " -l --list           list directories.\n"
                   " -p --prepend <dir>  add directory at the front.\n"
                   " -a --append  <dir>  add directory at the end.\n"
                   " -r --remove  <dir>  remove directory.\n"
                   " -u --user           user env.(default)\n"
                   " -s --system         system env.\n"
                   "    --var <NAME>     target env var (default: PATH)\n"
                // "    --backup         backup current value.\n"
                   "\n"
                   "Show/add/remove the PATH of the SYSTEM/USER\n"
                   "environment variables in the Windows Registry.\n"
                   "Note: This operates on the registry;\n"
                   "      it does not read the current process PATH.\n"
            );
        }
        return Er;
    }

    App() : sav_cp_(GetConsoleOutputCP()) {
        SetConsoleOutputCP(65001);
    }
    ~App() {
        SetConsoleOutputCP(sav_cp_);
    }

    //
    int wmain(int argc, wchar_t *argv[]) {
        if (argc < 2)
            return usage();

        bool    do_list     = false;
        bool    do_prepend  = false;
        bool    do_append   = false;
        bool    do_remove   = false;
        bool    sys         = false;
        bool    usr         = false;
        bool    backup      = false;
        wstring var_name    = L"PATH";
        wstring target;

        for (int i = 1; i < argc; ++i) {
            std::wstring a = argv[i];

            if (a[0] == L'-') {
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
                    if ( !validVarName(var_name) ) {
                        printf("Invalid variable name\n");
                        return Er;
                    }
                } else if (a == L"--backup") {
                    backup = true;                  // 隠し
                } else if (a == L"-?" || a == L"-h" || a == L"--help") {
                    return usage();
                } else {
                    printf("Unknown option: %s\n", wcsToUtf8(a).c_str());
                    return Er;
                }
            } else if (target.empty()) {
                target = a;
            } else {
                printf("Too many arguments: %s\n", wcsToUtf8(a).c_str());
                return Er;
            }
        }

        if (do_prepend && do_append) {
            printf("Specify only one of --prepend or --append\n");
            return Er;
        }

        wstring    cur;
        if ( !readEnv(var_name.c_str(), sys, cur) )
            cur.clear();

        if (backup)
            backupEnvToTxt(cur);

        if (do_list) {
            if (!sys && !usr) {
                wstring  env;
                if ( readEnv(var_name.c_str(), true, env) )
                    listPath(env, "SYSTEM");
            }
            listPath(cur, sys ? "SYSTEM" : "USER");
            return Ok;
        }

        if (target.empty()) {
            printf("No directory.\n");
            return Er;
        }

        target = stripQuotes(target);

        vector<wstring>   parts;
        splitPath(cur, parts);

        if (do_prepend || do_append) {
            bool    ex     = false;
            for (size_t i = 0; i < parts.size(); ++i) {
                if (_wcsicmp( parts[i].c_str(), target.c_str() ) == 0) {
                    ex = true;
                    break;
                }
            }
            if (ex) {
                printf("Already in variable\n");
                return Ok;
            }
            if (sys && !isAdmin())
                return restartAdmin() ? Ok : Er;

            if (do_append)
                parts.push_back(target);
            else
                parts.insert(parts.begin(), target);

            wstring  newv = joinPath(parts);

            if (!writeEnv(var_name.c_str(), sys, newv)) {
                printf("Failed to write environment variable\n");
                return Er;
            }

            printf("Added\n");
            return Ok;
        }

        if (do_remove) {
            bool    ex = false;
            for (size_t i = 0; i < parts.size();) {
                if (_wcsicmp(parts[i].c_str(), target.c_str()) == 0) {
                    parts.erase(parts.begin() + i);
                    ex = true;
                } else {
                    ++i;
                }
            }

            if (!ex) {
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

    //
    void listPath(wstring const& val, char const* label) {
        printf("[%s %s]\n", label, "ENV");
        vector<wstring> v;
        splitPath(val, v);

        for (size_t i = 0; i < v.size(); ++i) {
            string u = wcsToUtf8(v[i]);
            printf( "\"%s\"\n", u.c_str() );
        }
    }

    //
    void backupEnvToTxt(wstring const& val) {
        FILE *f = _wfopen(L"path_backup.txt", L"wb");

        if (f) {
            string u = wcsToUtf8(val);
            fwrite(u.c_str(), 1, u.size(), f);
            fclose(f);
            printf("Backup saved to path_backup.txt\n");
        }
    }

    //
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

    //
    static bool isAdmin() {
        BOOL                      admin = FALSE;
        PSID                      sid   = NULL;
        SID_IDENTIFIER_AUTHORITY  nt    = SECURITY_NT_AUTHORITY;
        AllocateAndInitializeSid(&nt, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &sid);
        if (sid) {
            CheckTokenMembership(NULL, sid, &admin);
            FreeSid(sid);
        }
        return admin != FALSE;
    }

    //
    static bool restartAdmin() {
        wchar_t exe[MAX_PATH];
        GetModuleFileNameW(NULL, exe, sizeof(exe)/sizeof(exe[0]));
        return (INT_PTR)ShellExecuteW(NULL, L"runas", exe, skipCmdLineArg0(GetCommandLineW()), NULL, SW_SHOWNORMAL) > 32;
    }

    //
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

    //
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

    //
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
        SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM) L"Environment", SMTO_ABORTIFHUNG, 5000, NULL);
        return true;
    }

    //
    static void splitPath(wstring const& path, vector<wstring>& v) {
        size_t  i = 0;
        size_t  j;
        while ((j = path.find(L';', i)) != wstring::npos) {
            if (j > i)
                v.push_back( path.substr(i, j - i) );
            i = j + 1;
        }
        if (i < path.size())
            v.push_back( path.substr(i) );
    }

    //
    static wstring joinPath(const vector<wstring> &v) {
        wstring r;
        for (size_t i = 0; i < v.size(); ++i) {
            if (i)
                r += L";";
            r += v[i];
        }
        return r;
    }

    //
    static wstring stripQuotes(wstring const& s) {
        wstring r = s;
        r.erase(remove(r.begin(), r.end(), L'"'), r.end());
        return r;
    }

    //
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


//
int wmain(int argc, wchar_t *argv[]) {
    return App().wmain(argc, argv);
}
