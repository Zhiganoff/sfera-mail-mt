#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <set>

using std::cout;
using std::endl;
using std::vector;
using std::string;

static const string str_delims(" ><&|");
static std::set<char> delims;
static std::set<string> operators;

/*typedef struct cmd_inf {
    char ** argv; // список из имени команды и аргументов
    char *infile; // переназначенный файл стандартного ввода
    char *outfile; // переназначенный файл стандартного вывода
    char *outfile2; // дозапись
    int first; // для корректного выхода из рекурсии
    int backgrnd; // =1, если команда подлежит выполнению в фоновом режиме
    struct cmd_inf* psubcmd; // команды для запуска в дочернем shell
    struct cmd_inf* pipe; // следующая команда после “|”
    struct cmd_inf* ifExecuted; // после &&
    struct cmd_inf* ifNotExecuted; // после ||
    struct cmd_inf* next; // следующая после “;” (или после “&”)
} cmd_inf;*/

class Command {
public:
    Command(const vector<string>& tokens);
    ~Command();
    void dump();

    char** argv; // список из имени команды и аргументов
    uint argc; // количество аргументов
    string infile; // переназначенный файл стандартного ввода
    string outfile; // переназначенный файл стандартного вывода
    bool backgrnd; // True, если команда подлежит выполнению в фоновом режиме
    Command* pipe; // следующая команда после “|”
    Command* ifExecuted; // после &&
    Command* ifNotExecuted; // после ||
};

Command::Command(const vector<string>& tokens) {
    argv = nullptr;
    argc = 0;
    // infile = "";
    // outfile = "";
    backgrnd = false;
    pipe = nullptr;
    ifExecuted = nullptr;
    ifNotExecuted = nullptr;
    auto it = tokens.begin();
    while (it != tokens.cend() && operators.find(*it) == operators.cend()) {
        argv = (char**)realloc(argv, (argc + 1) * sizeof(char *));
        argv[argc] = strdup(it->c_str());
        it++;
        argc++;
    }
    argv[argc] = nullptr;
    while (it != tokens.cend()) {
        if (!it->compare("<")) {
            infile = *(++it);
            it++;
        } else if (!it->compare(">")) {
            outfile = *(++it);
            it++;
        } else if (!it->compare("&")) {
            backgrnd = true;
            it++;
        } else if (!it->compare("|")) {
            pipe = new Command(vector<string>(++it, tokens.cend()));
            it = tokens.cend();
        } else if (!it->compare("&&")) {
            ifExecuted = new Command(vector<string>(++it, tokens.cend()));
            it = tokens.cend();
        } else if (!it->compare("||")) {
            ifNotExecuted = new Command(vector<string>(++it, tokens.cend()));
            it = tokens.cend();
        }
    }
}

Command::~Command() {
    for (uint idx = 0; idx < argc; idx++) {
        free(argv[idx]);
    }
    free(argv);
}

void Command::dump() {
    cout << "debug" << endl;
    cout.flush();
    cout << "argv: ";
    for (uint idx = 0; idx < argc; idx++) {
        cout << argv[idx] << ' ';
    }
    cout << endl;
    if (infile.size()) {
        cout << "infile: " << infile << endl;
    }
    if (outfile.size()) {
        cout << "outfile: " << outfile << endl;
    }
    cout << backgrnd << endl;
    if (pipe) {
        cout << "pipe:" << endl;
        pipe->dump();
    }
    if (ifExecuted) {
        cout << "ifExecuted:" << endl;
        ifExecuted->dump();
    }
    if (ifNotExecuted) {
        cout << "ifNotExecuted:" << endl;
        ifNotExecuted->dump();
    }
    // cout <<
}

void fill_sets() {
    for (const char& c : str_delims) {
        delims.insert(c);
    }
    operators.insert("|");
    operators.insert("&&");
    operators.insert("||");
    operators.insert(">");
    operators.insert("<");
    operators.insert("&");
}

void tokenize(vector<string>& tokens, const string& input) {
    tokens.clear();
    size_t idx = input.find_first_not_of(' ');
    size_t len;
    string token;
    while (idx != input.size()) {
        while (input[idx] == ' ') {
            idx++;
        }
        len = 1;
        while (idx + len != input.size() && delims.find(input[idx + len]) == delims.cend()){
            len++;
        }
        tokens.push_back(input.substr(idx, len));
        idx += len;
        if (idx != input.size() && input[idx] != ' ') {
            len = 1;
            while (delims.find(input[idx + len]) != delims.cend() &&
                   input[idx + len] != ' ') {
                len++;
            }
            tokens.push_back(input.substr(idx, len));
            idx += len;
        }
    }
}

int main() {
    fill_sets();
    string input;
    vector<string> tokens;
    while (true) {
        //std::cin >> input;
        std::getline(std::cin, input);
        tokenize(tokens, input);
        // for (const string& token : tokens) {
        //     cout << token << ' ' << token.size() << endl;
        // }
        Command cur_cmd(tokens);
        cout << "debug" << endl;
        cout.flush();
        cur_cmd.dump();
    }

    return 0;
}
