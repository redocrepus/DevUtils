#include <iostream>
#include <filesystem>
#include <Windows.h>
#include <regex>

using namespace std;
using namespace std::filesystem;

vector<string> gArgs;
vector<string> gInnocentDirs({ "docs", "src", "packages", "lib" });
bool gDryRun = false;
bool gYesToAll = false;

path currentExeFolderPath() {
    static path curdir;

    if (curdir != "")
        return curdir;

    char szExeFilePath[MAX_PATH]{};
    DWORD dwRet = GetModuleFileNameA(GetModuleHandle(NULL), szExeFilePath, sizeof szExeFilePath); // TODO: check return value

    curdir = ((path)szExeFilePath).parent_path();
    return curdir;
}

bool shouldBeDeleted(const path& p) {
    if (p == currentExeFolderPath()) // Don't delete yourself
        return false;
    string name = p.filename().string();
    bool ret = false;

    if (is_directory(p))
        ret =        name == ".vs"
            ||       name == "x64"
            ||       name == "x86"
            ||       name == "bin"
            ||       name == "obj"
            ||       regex_match(name, regex("^.*(debug|release).*$", regex_constants::icase))
            //||       regex_match(name, regex("^.*scad.*$", regex_constants::icase))
        ;
    else
        ret = (p.extension() == ".pdb" // VS db
            // TODO: || p.extension() == ".pyc" // compiled python ?
            );
        

    return ret;
}

bool isInnocentDir(const path& p) { // to be skipped
    if (!is_directory(p))
        throw runtime_error(p.string() + " is not a dir!!!");


    string name = p.filename().string();
    bool ret =
        find(gInnocentDirs.begin(), gInnocentDirs.end(), name) != gInnocentDirs.end() ||
        regex_match(name, regex("^scripts$", regex_constants::icase))                 ||
        regex_match(name, regex("^\.git.*$", regex_constants::icase))                 ||
        regex_match(name, regex("^.*data.*$", regex_constants::icase));

    return ret;
}

void clean(const path& p, bool yesToAll = false) {

    // cout << p << endl;

    string name = p.filename().string();
    if (shouldBeDeleted(p)) { // TODO: handle files and folders separately

        // path t = temp_directory_path() / name;
        // cout << "Moving " << (is_directory(p) ? " folder: " : "file: ") << p << " to " << t << endl;

        char ack = 'n';
        if (!yesToAll) {
            cout << "Remove " << (is_directory(p) ? " folder:\t " : "file:\t") << p << " ? (y/n)" << endl;
            cin >> ack;
        }
        else
            ack = 'y';

        if ((ack == 'Y') || (ack == 'y'))
            try {
                if (!gDryRun) {
                    cout << "Deleting " << (is_directory(p) ? " folder:\t" : "file:\t") << p << endl;
                    remove_all(p);
                }
                else
                    cout << "Would delete " << (is_directory(p) ? " folder:\t" : "file:\t") << p << endl;
                // rename(p, t);
            }
            catch (filesystem_error e) {
                cout << e.what() << endl;
            }
            
        return;
    }

    if (!is_directory(p))
        return;

    if (isInnocentDir(p)) {
        cout << "Skipping " << p << endl;
        return;
    }

    for (const auto& entry : directory_iterator(p)) {
        try {
            clean(entry.path(), yesToAll);
        }
        catch (exception e) {
            cout << e.what() << endl;
        }
    }
}

int main(int argc, char* argv[])
{

    gArgs = std::vector<std::string>(&argv[1], argv + argc);
    gYesToAll = (1 <= gArgs.size() && !gArgs[0].compare("--y"));
    gDryRun = (2 <= gArgs.size() && !gArgs[1].compare("--dry"));
    gInnocentDirs.insert(gInnocentDirs.end(), gArgs.begin(), gArgs.end());

    cout << "I'm in: " << currentExeFolderPath() << endl;
    clean(currentExeFolderPath(), gYesToAll);

    return 0;
}