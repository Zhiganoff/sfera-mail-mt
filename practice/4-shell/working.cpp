#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <cstring>
#include <cerrno>
#include <stdexcept>
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
    // static void conveyor(Command* cmd);

    char** argv;         // список из имени команды и аргументов
    uint argc;           // количество аргументов
    std::string infile;  // переназначенный файл стандартного ввода
    std::string outfile; // переназначенный файл стандартного вывода

    std::string token;
    std::vector<Expression> args;
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

                int saveIn = dup(0);
                size_t idx = 0;
                for (; idx < conv.size() - 1; idx++) {
                    pipe2(fd, O_CLOEXEC);
                    if (!fork()) {
                        signal(SIGINT, SIG_DFL);
                        dup2(fd[1], STDOUT_FILENO);
                        close(fd[0]);
                        if (conv[idx].args.size()) {
                            exit(conv[idx].execute());
                        } else {
                            execvp(conv[idx].argv[0], conv[idx].argv);
                        }
                    }
                    dup2(fd[0], STDIN_FILENO);
                    close(fd[1]);
                }
                if (!fork()) {
                    signal(SIGINT, SIG_DFL);
                    if (conv[idx].args.size()) {
                        exit(conv[idx].execute());
                    } else {
                        execvp(conv[idx].argv[0], conv[idx].argv);
                    }
                }
                close(fd[0]);
                close(fd[1]);
                int ex_status;
                while (wait(&ex_status) != -1);
                dup2(saveIn, 0);
                close(saveIn);
                // pipe(fd);
                // if (!fork()) {
                //     dup2(fd[1], 1);
                //     close(fd[0]);

                // }
                return ex_status;
            } else {
                int ex_status = args[0].execute();
                // std::cout << token << std::endl;
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
            pid_t pid;
            pid = fork();
            if (pid == 0) {
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
            wait(&ex_status);
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
    static std::vector<pid_t> pids;
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
    // while (count) {
    //     int ex_status;
    //     pid_t cp = wait(&ex_status);
    //     std::cerr << "Process " << cp << " exited: " << WEXITSTATUS(ex_status) << std::endl;
    // }
    pid_t p;
    int status;

    if ((p=waitpid(-1, &status, WNOHANG)) != -1)
    {
        if (p) {
            std::cerr << "Process " << p << " exited: " << WEXITSTATUS(status) << std::endl;
        }
    }
}

// void child_handler(int signum, siginfo_t * siginfo, void *code)  {
//     Parser::count++;
//     std::cout << "debug: Handled" << std::endl;
// }

void child_handler(int signum) {
    Parser::count++;    
}

void my_sigchld_handler(int sig)
{
    // pid_t p;
    // int status;

    // while ((p=waitpid(-1, &status, WNOHANG)) != -1)
    // {
    //    std::cerr << "Process " << p << " exited: " << WEXITSTATUS(status) << std::endl;
    // }
    Parser::count++;
}

int Parser::count = 0;
std::vector<pid_t> Parser::pids;

int main()
{
    fill_sets();
    std::string input;
    std::vector<std::string> tokens;
    signal(SIGINT, SIG_IGN);
    // struct sigaction sa;
    // memset(&sa, 0, sizeof(sa));
    // sa.sa_handler = my_sigchld_handler;
    // sigaction(SIGCHLD, &sa, NULL);
    while (std::getline(std::cin, input)) {
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
            // struct sigaction sa;
            // memset(&sa, 0, sizeof(sa));
            // sa.sa_handler = my_sigchld_handler;
            // sa.sa_flags = SA_RESTART;

            // sigaction(SIGCHLD, &sa, NULL);
            pid_t pid_back = fork();
            if (pid_back == 0) {
                // int fd;
                // signal (SIGINT, SIG_IGN);
                // fd = open("/dev/null", O_RDWR);
                // dup2(fd, 0);
                // dup2(fd, 1);
                // close(fd);
                signal(SIGINT, SIG_DFL);
                exit(e.execute());
                }
            std::cerr << "Spawned child process " << pid_back << std::endl;
        } else {
            e.execute();
        }
    }
    return 0;
}
