#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <set>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

using std::cout;
using std::endl;
using std::vector;
using std::string;

static const string str_delims(" ><&|");
static std::set<char> delims;
static std::set<string> operators;

class Command {
public:
    Command(const vector<string>& tokens);
    ~Command();
    void dump();
    int execute();
    static void conveyor(Command* cmd);

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
        argv[argc++] = strdup(it->c_str());
        it++;
    }
    argv = (char**)realloc(argv, (argc + 1) * sizeof(char *));
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
    delete pipe; // следующая команда после “|”
    delete ifExecuted; // после &&
    delete ifNotExecuted;
}

void Command::dump() {
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
    size_t idx = 0, len = 0;
    while (input[idx] == ' ') {
        idx++;
    }
    while (idx != input.size()) {
        if (delims.find(input[idx]) == delims.cend()) {
            len = 1;
            while (idx + len != input.size() && delims.find(input[idx + len]) == delims.cend()){
                len++;
            }
        } else {
            len = 1;
            if (delims.find(input[idx + 1]) != delims.cend() && input[idx + 1] != ' ') {
                len++;
            }
        }
        // cout << "deb: " << idx << ' ' << len << endl;
        // cout << "res: " << input.substr(idx, len) << endl;
        tokens.push_back(input.substr(idx, len));
        idx += len;
        while (input[idx] == ' ') {
            idx++;
        }
    }
}

int Command::execute() {
    if (pipe) {
        Command::conveyor(this);
        return 0;
    }
    pid_t pid, pid_back = 0;
    pid = fork();
    if (pid == 0) {
        /* Перенаправление ввода/вывода */
        int fdIn, fdOut;
        if (infile.size()){
            if ((fdIn = open(infile.c_str(), O_RDONLY, 0666)) == -1) {
                cout << "Can't open " << infile << endl;
                return 0;
                }
            dup2(fdIn, STDIN_FILENO);
            close(fdIn);
        }
        if (outfile.size()) {
            fdOut = open(outfile.c_str(),  O_CREAT | O_TRUNC | O_WRONLY, 0666);
            dup2(fdOut, STDOUT_FILENO);
            close(fdOut);
        }
        /* Исполнение */
        if (backgrnd) {
            pid_back = fork();
            if (pid_back == 0) {
                int fd;
                signal (SIGINT, SIG_IGN);
                fd = open("/dev/null", O_RDWR);
                dup2(fd, 0);
                dup2(fd, 1);
                close(fd);
                execvp(argv[0], argv);
                }
            std::cerr << "Spawned child process " << pid_back << endl;
            exit(0);
            return 0;
        }
        if (argv) {
            execvp(argv[0], argv);
            perror(argv[0]);
            exit(1);
        }
    }
    int ex_status;
    // if (backgrnd) { 
    //     wait(&ex_status);
    //     std::cerr << "Process " << pid_back << " exited: " << WIFEXITED(ex_status) << endl;
    //     return ex_status;
    // }
    wait(&ex_status);
    if (ifExecuted)
    {
        if (WIFEXITED(ex_status)) {
            if (!WEXITSTATUS(ex_status)) {
                ex_status = ifExecuted->execute();
                // std::cerr << WEXITSTATUS(ex_status) << endl;
            }
        }
    } else if (ifNotExecuted) {
        if (!WIFEXITED(ex_status)) {
            ifNotExecuted->execute();
        } else if (WIFEXITED(ex_status) && WEXITSTATUS(ex_status)) {
            ifNotExecuted->execute();
        }
    }
    return ex_status;
}

void Command::conveyor(Command* cmd) {
    int saveIn = dup(0);
    int fd[2];
    while (cmd->pipe) {
        ::pipe(fd);
        if (!fork()) {
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);
            close(fd[0]);
            execvp(cmd->argv[0], cmd->argv);
            perror(cmd->argv[0]);
            exit(1);
        }
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        close(fd[1]);
        cmd = cmd->pipe;
    }
    if (!fork()) {
        execvp(cmd->argv[0], cmd->argv);
    }
    close(fd[0]);
    while (wait(NULL) != -1);
    dup2(saveIn, 0);
    close(saveIn);
}

int main() {
    fill_sets();
    string input;
    vector<string> tokens;
    while (std::getline(std::cin, input)) {
        tokenize(tokens, input);
        // for (const string& token : tokens) {
        //    cout << token << ' ' << token.size() << endl;
        // }
        Command cur_cmd(tokens);
        // cur_cmd.dump();
        cur_cmd.execute();
        // std::cerr << WEXITSTATUS(ex_status) << endl;
    }

    return 0;
}
