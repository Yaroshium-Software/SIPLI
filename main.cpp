#pragma region includes
#include <iostream>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <regex>
#include <string>
#include <cstdlib>
#include <cmath>
#include <exception>
#pragma endregion includes
using namespace std;
#pragma region constants
const string SIPL_VER = "0.2.1";
#pragma endregion constants

class Interpreter
{
    unordered_map<string, string> vars;
    unordered_map<string, int> labels;
    unordered_map<string, vector<string>> arrays;
    vector<string> lines;
    bool debug_verbose;

private:
    static bool starts_with(const string &s, const string &prefix)
    {
        return s.find(prefix) == 0;
    }

    static string trim(const string &s)
    {
        size_t start = s.find_first_not_of(" \t\n\r");
        size_t end = s.find_last_not_of(" \t\n\r");
        return (start == string::npos) ? "" : s.substr(start, end - start + 1);
    }

    static vector<string> split(const string &str, char delimiter)
    {
        vector<string> result;
        stringstream ss(str);
        string item;
        while (getline(ss, item, delimiter))
        {
            result.push_back(item);
        }
        return result;
    }

    void handle_echo(const string &text)
    {
        regex var_pattern(R"(\$(\w+))");
        string result;
        sregex_iterator it(text.begin(), text.end(), var_pattern);
        sregex_iterator end;

        size_t last_pos = 0;
        while (it != end)
        {
            result += text.substr(last_pos, it->position() - last_pos);
            string varname = (*it)[1];
            result += vars.count(varname) ? vars[varname] : "$" + varname;
            last_pos = it->position() + it->length();
            ++it;
        }
        result += text.substr(last_pos);
        cout << result << endl;
    }
    void debug_print(string t)
    {
        if (this->debug_verbose)
            cout << "[DEBUG] " << t << endl;
    }

    string eval_expr(const string &s)
    {
        if (s.empty())
            return "";
        stringstream ss(s);
        vector<string> toks;
        string tok;
        while (ss >> tok)
            toks.push_back(tok);

        vector<int> stack;
        for (const string &t : toks)
        {
            if (isdigit(t[0]) || (t[0] == '-' && t.size() > 1))
            {
                stack.push_back(stoi(t));
            }
            else if (t[0] == '$')
            {
                string varname = t.substr(1);
                if (vars.count(varname))
                {
                    stack.push_back(stoi(vars[varname]));
                }
                else
                {
                    throw runtime_error("Undefined variable: " + varname);
                }
            }
            else if (vars.count(t))
            {
                stack.push_back(stoi(vars[t]));
            }
            else if (t == "+" || t == "-" || t == "*" || t == "/" || t == "%" || t == "**")
            {
                if (stack.size() < 2)
                    throw runtime_error("Not enough operands for: " + t);
                int b = stack.back();
                stack.pop_back();
                int a = stack.back();
                stack.pop_back();
                if (t == "+")
                    stack.push_back(a + b);
                else if (t == "-")
                    stack.push_back(a - b);
                else if (t == "*")
                    stack.push_back(a * b);
                else if (t == "/")
                    stack.push_back(b != 0 ? a / b : 0);
                else if (t == "%")
                    stack.push_back(a % b);
                else if (t == "**")
                    stack.push_back((int)pow(a, b));
            }
            else
            {
                throw runtime_error("Unsupported token: " + t);
            }
        }

        return stack.empty() ? "" : to_string(stack[0]);
    }

    void print_help()
    {
        cout << "SIPL token list\n\n"
             << "PRNT [text]                      - print text (supports $variables)\n"
             << "VAR [name] = [val]               - define a variable\n"
             << "INPT [varname]                   - read stdin into variable\n"
             << "GOTO [label]                     - jump to label\n"
             << ":label                           - define label\n"
             << "IF var [operand] val : [action]  - run action if condition met (Supported operands: == != < > <= >=)\n"
             << "HLP                              - show help message\n"
             << "DMP                              - dump program data (debug only)\n"
             << "_comment                         - ignored line\n\n";
    }

    void error(const string &msg, const string &line = "")
    {
        cerr << "[ERROR] " << msg;
        if (!line.empty())
            cerr << " | Line: \"" << line << "\"";
        cerr << endl;
    }
    string normal_itoa(int a)
    {
        // TODO: remove
        string ret = "";
        if (a == 0)
            return "0";
        while (a > 0)
        {
            int c = a % 10;
            char cc = "0123456789"[c];
            ret = cc + ret;
            a = a / 10;
        }
        return ret;
    }

public:
    Interpreter(bool debug = 0)
    {
        this->debug_verbose = debug;
    }
    int exec_line(string line, int &i)
    {
        try
        {
            if (line.empty() || line[0] == ':' || line[0] == '_')
            {
                debug_print("Line empty");
                return 1;
            }

            if (starts_with(line, "PRNT"))
            {
                handle_echo(trim(line.substr(4)));
            }
            else if (starts_with(line, "VAR"))
            {
                string rest = trim(line.substr(3));
                size_t eq = rest.find('=');
                if (eq != string::npos)
                {
                    string name = trim(rest.substr(0, eq));
                    string expr = trim(rest.substr(eq + 1));
                    vars[name] = eval_expr(expr);
                }
                else
                {
                    error("Invalid VAR syntax", line);
                }
            }
            else if (starts_with(line, "GOTO"))
            {
                string label = trim(line.substr(4));
                debug_print("Checking label " + label);
                if (labels.count(label))
                {
                    if(labels[label] == i){
                        error("GOTO instruction jumped to self");
                        return 0;
                    }
                    else{
                        i = labels[label] - 1;
                        debug_print("Label found at line " + normal_itoa(i) + ", jumping");
                    }
                }
                else
                {
                    error("Label not found: " + label, line);
                }
            }
            else if (starts_with(line, "IF"))
            {
                string rest = trim(line.substr(2));
                size_t colon = rest.find(':');
                if (colon == string::npos)
                {
                    error("Invalid IF syntax", line);
                }
                string condition = trim(rest.substr(0, colon));
                string action = trim(rest.substr(colon + 1));

                regex comp_regex(R"((\w+)\s*(==|!=|<|>|<=|>=)\s*(\"?.+?\"?))");
                smatch match;
                if (regex_match(condition, match, comp_regex))
                {
                    debug_print("IF REGEX DUMP");
                    for (int i = 0; i < match.length(); i++)
                    {
                        string data = match[i];
                        debug_print("[" + normal_itoa(i) + "] " + data);
                    }
                    debug_print("END IF REGEX DUMP");
                    string var = match[1];
                    string op = match[2];
                    string val = match[3];

                    string var_val = vars.count(var) ? vars[var] : "";
                    bool cond_met = false;

                    if (val.front() == '"' && val.back() == '"')
                        val = val.substr(1, val.size() - 2);

                    if (op == "==")
                        cond_met = (var_val == val);
                    else if (op == "!=")
                        cond_met = (var_val != val);
                    else
                    {
                        int lhs = stoi(var_val);
                        int rhs = stoi(val);
                        if (op == "<")
                            cond_met = lhs < rhs;
                        else if (op == ">")
                            cond_met = lhs > rhs;
                        else if (op == "<=")
                            cond_met = lhs <= rhs;
                        else if (op == ">=")
                            cond_met = lhs >= rhs;
                    }

                    if (cond_met)
                    {
                        exec_line(action, i);
                    }
                }
                else
                {
                    error("Malformed condition in IF", line);
                    return 0;
                }
            }
            else if (starts_with(line, "INPT"))
            {
                string varname = trim(line.substr(4));
                if (varname.empty())
                {
                    error("Missing variable name for INPT", line);
                    return 0;
                }
                string value;
                if (!getline(cin, value))
                {
                    error("Failed to read input", line);
                    return 0;
                }
                vars[varname] = trim(value);
            }
            else if (starts_with(line, "HLP"))
            {
                print_help();
            }
            else if (starts_with(line, "EXIT"))
            {
                debug_print("Exiting.");
                i = lines.size();
                return -1;
            }
            else if (starts_with(line, "DMP"))
            {
                debug_print("PROGRAM DATA DUMP:");
                debug_print("## LABELS:");
                for (pair<string, int> label : labels)
                {
                    debug_print("- " + label.first + " at " + normal_itoa(label.second));
                }
                debug_print("## VARIABLES:");
                for (pair<string, string> var : vars)
                {
                    debug_print("- " + var.first + " = " + var.second);
                }
                cout<<"P. S. If you see no output, you might have debug mode disabled.\n";
            }
            else
            {
                cout << "Unknown command: " << line << endl;
                return 0;
            }
        }
        catch (exception &e)
        {
            error(string("Exception: ") + e.what(), line);
            return 0;
        }
        return 1;
    }
    bool run(const string &program)
    {
        debug_print("In debug mode");
        lines = split(program, ';');

        for (int i = 0; i < lines.size(); ++i)
        {
            debug_print("Checking for labels at line " + normal_itoa(i));
            string line = trim(lines[i]);
            debug_print(line);
            if (!line.empty() && line[0] == ':')
            {
                string label = trim(line.substr(1));
                debug_print("Found label " + label);
                labels[label] = i;
            }
        }

        for (int i = 0; i < lines.size(); ++i)
        {
            string line = trim(lines[i]);
            debug_print("L " + normal_itoa(i) + " / " + normal_itoa(lines.size()) + " : " + line);
            int res = exec_line(line, i);
            if (res == 0)
                break;
            else if (res == -1)
                return 0;
        }
        return 1;
    }
};

int main(int argc, char *argv[])
{
    string fpath;
    bool debug = 0;
    for (int argn = 1; argn < argc; argn++)
    {
        if ((string)argv[argn] == (string) "--debug")
        {
            debug = 1;
        }
        else if ((string)argv[argn] == (string) "-f")
        {
            if (argc <= argn + 1)
            {
                cerr << "Parameterized argument without parameter" << endl;
                return 1;
            }
            fpath = argv[argn + 1];
            argn++;
        }
        else if ((string)argv[argn] == (string) "-h")
        {
            cout << "SIPLI argument list\n"
                 << "-h - show this message\n"
                 << "-f [file] - run file\n"
                 << "\n"
                 << "--debug - enable debug mode for extended debug debugging of debugger (obsolete (no))"
                 << endl;
            return 0;
        }
        else
        {
            cout << "Unrecognized argument: " << argv[argn] << endl;
            return 0;
        }
    }
    if (!fpath.empty())
    {
        ifstream file(fpath);
        if (!file.is_open())
        {
            cerr << "Could not open file.\n";
            return 1;
        }
        stringstream buffer;
        buffer << file.rdbuf();
        Interpreter evaluator(debug);
        evaluator.run(buffer.str());
    }
    else
    {
        cout << "SIPLI version " << SIPL_VER << "\nUse HLP for help or pass -h argument for parameter list.\n";
        Interpreter evaluator(debug);
        while (1)
        {
            cout << ">>> ";
            string q;
            getline(cin, q);
            if (!evaluator.run(q))
            {
                break;
            }
        }
    }

    return 0;
}
