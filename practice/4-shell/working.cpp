#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <limits>
#include <ios>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

static const std::string str_delims(" ><&|");
static std::set<char> delims;
static std::set<std::string> operators;
static std::set<std::string> expr_operators;

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
    expr_operators.insert("|");
    expr_operators.insert("&&");
    expr_operators.insert("||");
    expr_operators.insert("&");
}

struct Expression {
    Expression(const std::vector<std::string>& input);
    Expression(std::string token, Expression a) : token(token), args{ a } {}
    Expression(std::string token, Expression a, Expression b) : token(token), args{ a, b } {}

    void dump();
    int execute();
    void get_conv_v(std::vector<Expression>& v);

    char** argv;         // список из имени команды и аргументов
    uint argc;           // количество аргументов
    std::string infile;  // переназначенный файл стандартного ввода
    std::string outfile; // переназначенный файл стандартного вывода

    std::string token;
    std::vector<Expression> args;
    static std::set<pid_t> cur_pids;
};

Expression::Expression(const std::vector<std::string>& input) {
    argv = nullptr;
    argc = 0;
    auto it = input.begin();
    while (it != input.cend() && operators.find(*it) == operators.cend()) {
        argv = (char**)realloc(argv, (argc + 1) * sizeof(char *));
        argv[argc++] = strdup(it->c_str());
        it++;
    }
    argv = (char**)realloc(argv, (argc + 1) * sizeof(char *));
    argv[argc] = nullptr;
    while (it != input.cend()) {
        if (!it->compare("<")) {
            infile = *(++it);
            it++;
        } else if (!it->compare(">")) {
            outfile = *(++it);
            it++;
        }
    }
}

void Expression::dump() {
    // std::cout << "debug: Entered dump()" << std::endl;
    // if (args.size()) {
    //     std::cout << token << ' ' << args.size() << std::endl;
    // }
    switch (args.size()) {
        case 2: {
            args[0].dump();
            std::cout << token << std::endl;
            args[1].dump();
            // std::cout << "debug wtf1 " << args.size() << std::endl;
            return;
        }
        case 0:
            std::cout << "argv: ";
            for (uint idx = 0; idx < argc; idx++) {
                std::cout << argv[idx] << ' ';
            }
            std::cout << std::endl;
            if (infile.size()) {
                std::cout << "infile: " << infile << std::endl;
            }
            if (outfile.size()) {
                std::cout << "outfile: " << outfile << std::endl;
            }
        }
    // std::cout << "debug wtf" << std::endl;
}

int Expression::execute() {
    // std::cout << "debug: Entered execute()" << std::endl;
    // if (args.size()) {
    //     std::cout << token << ' ' << args.size() << std::endl;
    // }
    static int fd[2];
    switch (args.size()) {
        case 2: {
            if (!token.compare("|")) {
                std::vector<Expression> conv;
                get_conv_v(conv);
                // for (auto& expr : conv) {
                //     expr.dump();
                // }
                pid_t ch_pid;
                std::set<pid_t> set_pids;
                int saveIn = dup(0);
                size_t idx = 0;
                for (; idx < conv.size() - 1; idx++) {
                    pipe2(fd, O_CLOEXEC);
                    if (!(ch_pid = fork())) {
                        signal(SIGINT, SIG_DFL);
                        dup2(fd[1], STDOUT_FILENO);
                        close(fd[0]);
                        if (conv[idx].args.size()) {
                            exit(conv[idx].execute());
                        } else {
                            execvp(conv[idx].argv[0], conv[idx].argv);
                        }
                    }
                    set_pids.insert(ch_pid);
                    dup2(fd[0], STDIN_FILENO);
                    close(fd[1]);
                }
                pid_t last_pid;
                if (!(last_pid = fork())) {
                    signal(SIGINT, SIG_DFL);
                    if (conv[idx].args.size()) {
                        exit(conv[idx].execute());
                    } else {
                        execvp(conv[idx].argv[0], conv[idx].argv);
                    }
                }
                // std::cout << "debug: " << ch_pid << ' ' << last_pid << std::endl;
                set_pids.insert(last_pid);
                cur_pids.insert(set_pids.begin(), set_pids.end());
                close(fd[0]);
                close(fd[1]);
                int ex_status, ret_status = 0;
                while (set_pids.size()) {
                    for (auto p : set_pids) {
                        ch_pid = waitpid(p, &ex_status, WNOHANG);
                        if (ch_pid != 0 && ch_pid != -1) {
                            set_pids.erase(ch_pid);
                            cur_pids.erase(ch_pid);
                            if (ch_pid == last_pid) {
                                ret_status = ex_status;
                                // std::cout << "ret status: " << ret_status << std::endl;
                            }
                            // std::cout << "Exited: pid = " << ch_pid << " st = " << WEXITSTATUS(ex_status) << std::endl;
                        }
                    }
                }
                dup2(saveIn, 0);
                close(saveIn);
                // pipe(fd);
                // if (!fork()) {
                //     dup2(fd[1], 1);
                //     close(fd[0]);

                // }
                return ret_status;
            } else {
                int ex_status = args[0].execute();
                // std::cout << "exited" << WEXITSTATUS(ex_status) << std::endl;
                if (!token.compare("&&"))
                {
                    // std::cout << ex_status << std::endl;
                    if (WIFEXITED(ex_status)) {
                        if (!WEXITSTATUS(ex_status)) {
                            return args[1].execute();
                        }
                    }
                } else {
                    // std::cout << ex_status << std::endl;
                    if (!WIFEXITED(ex_status) || (WIFEXITED(ex_status) && WEXITSTATUS(ex_status))) {
                        return args[1].execute();
                    }
                }
                // std::cout << ex_status << std::endl;
                return ex_status;
            }
        }
        case 0: {
            pid_t ch_pid;
            ch_pid = fork();
            if (ch_pid == 0) {
                signal(SIGINT, SIG_DFL);
                /* Перенаправление ввода/вывода */
                int fdIn, fdOut;
                if (infile.size()) {
                    if ((fdIn = open(infile.c_str(), O_RDONLY, 0666)) == -1) {
                        std::cout << "Can't open " << infile << std::endl;
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
                if (argv) {
                    execvp(argv[0], argv);
                    perror(argv[0]);
                    exit(1);
                }
            }
            int ex_status;
            Expression::cur_pids.insert(ch_pid);
            waitpid(ch_pid, &ex_status, 0);
            Expression::cur_pids.erase(ch_pid);
            // std::cout << ex_status << std::endl;
            return ex_status;
        }
        default:
            return 1;
    }
}

void Expression::get_conv_v(std::vector<Expression>& vec) {
    switch (args.size()) {
        case 2: {
            if (token.compare("|")) {
                vec.push_back(*this);
            } else {
                args[0].get_conv_v(vec);
                args[1].get_conv_v(vec);       
            }
            return;
        }
        case 0:
            vec.push_back(*this);
        }
}

class Parser {
public:
    explicit Parser(std::string& input);
    Expression parse();

    static void handle_signals();

    bool background;       // True, если команда подлежит выполнению в фоновом режиме
    static int count;
    static std::set<pid_t> set_backgr;
private:
    std::string parse_token();
    Expression parse_simple_expression();
    Expression parse_binary_expression();

    std::vector<std::string> tokens;
};

Parser::Parser(std::string& input) {
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
        tokens.push_back(input.substr(idx, len));
        idx += len;
        while (input[idx] == ' ') {
            idx++;
        }
    }
    if (tokens.size() && !tokens.back().compare("&")) {
        background = true;
        tokens.pop_back();
    } else {
        background = false;
    }
}

Expression Parser::parse_simple_expression() {
    auto it = tokens.begin();
    while (it != tokens.cend() && expr_operators.find(*it) == expr_operators.cend()) {
        it++;
    }
    std::vector<std::string> argv(tokens.begin(), it);
    // std::cout << "debug7" << std::endl;
    // for (const auto& t : argv) {
    //     std::cout << t << ' ';
    // }
    // std::cout << std::endl;
    tokens.erase(tokens.begin(), it);
    return Expression(argv);
}

Expression Parser::parse_binary_expression() {
    // std::cout << "debug5" << std::endl;
    Expression left_expr = parse_simple_expression();
    // std::cout << "debug " << tokens.size() << std::endl;
    // left_expr.dump();
    while (tokens.size()) {
        std::string op = tokens[0];
        tokens.erase(tokens.begin());
        // std::cout << "dubug op: " << op << std::endl;
        // std::cout << "debug tokens.size(): " << tokens.size() << std::endl;

        Expression right_expr = parse_simple_expression();
        left_expr = Expression(op, left_expr, right_expr);
    }
    // std::cout < <s"debug7" << std::endl;
    return left_expr;
}

Expression Parser::parse() {
    return parse_binary_expression();
}

void Parser::handle_signals() {
    pid_t ch_pid;
    int ex_status;

    for (auto p : set_backgr) {
        ch_pid = waitpid(p, &ex_status, WNOHANG);
        if (ch_pid != 0 && ch_pid != -1) {
            set_backgr.erase(ch_pid);
            std::cerr << "Process " << ch_pid << " exited: " << WEXITSTATUS(ex_status) << std::endl;
        }
        // std::cout << "waitpid: " << ch_pid << std::endl;
    }
}

void sigint_handler(int sig)
{
    for (auto p : Parser::set_backgr) {
        kill(p, SIGINT);
        // std::cout << "waitpid: " << ch_pid << std::endl;
    }
    for (auto p : Expression::cur_pids) {
        kill(p, SIGINT);
    }
    Parser::set_backgr.clear();
    Expression::cur_pids.clear();
    signal(SIGINT, sigint_handler);
    // std::cout << "Hey, whi i'll die?" << std::endl;
}

std::set<pid_t> Parser::set_backgr;
std::set<pid_t> Expression::cur_pids;

int main()
{
    fill_sets();
    std::string input;
    std::vector<std::string> tokens;
    signal(SIGINT, sigint_handler);
    setvbuf(stdin, NULL, _IONBF, 0);
    while (!std::cin.eof()) {
        if (isatty(STDIN_FILENO)) {
            std::cout << "Enter new command:" << std::endl;
        }
        std::getline(std::cin, input);
        if (!input.compare("")) {
            continue;
        }
        // std::cout << "input: " << input << std::endl;
        Parser::handle_signals();
        // std::cout << "debug1" << std::endl;
        Parser p(input);
        // std::cout << "debug2" << std::endl;
        Expression e = p.parse();
        // std::cout << "debug3" << std::endl;
        // e.dump();
        // std::cout << "debug: handled" << std::endl;
        if (p.background) {
            pid_t pid_back = fork();
            if (pid_back == 0) {
                signal(SIGINT, SIG_DFL);
                int ex_status = e.execute();
                exit(WEXITSTATUS(ex_status));
                }
            std::cerr << "Spawned child process " << pid_back << std::endl;
            Parser::set_backgr.insert(pid_back);
            // Parser::pids.push_back(pid_back);
        } else {
            // std::cout << "I'll execute" << std::endl;
            e.execute();
        }
    }
    // std::cout << "Out of cycle" << std::endl;
    return 0;
}
