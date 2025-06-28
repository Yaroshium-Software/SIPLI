#pragma region includes
#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <vector>
#include <regex>
#include <string>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <exception>
#include <random>
#pragma endregion includes
using namespace std;
#pragma region constants
const string SIPL_VER = "0.2.1-pre2";
const string SIPLI_APPENDIX = "-002";
#pragma endregion constants

string operator*(string a, int x)
{
    string ret;
    for (int i = 0; i < x; i++)
    {
        ret += a;
    }
    return ret;
}

class AbstractValue
{
private:
    // todo!
};
class SiplExprType
{
public:
    const static int LITERAL = 0;
    const static int VARIABLE = 1;
    const static int GENERIC = 2;
};
class SiplExpr
{
public:
    int type;
    string data;
    AbstractValue evaluate()
    {
        // todo!
        return AbstractValue();
    }
};

class Interpreter
{
    class HelpEntry
    {
    public:
        string name, desc;
        HelpEntry(string name, string desc)
        {
            this->name = name;
            this->desc = desc;
        }
    };
    map<string, string> vars;
    map<int, string> labels;
    map<string, vector<string>> arrays;
    vector<string> lines;
    bool debug_verbose;
    const vector<HelpEntry> helpData{
        HelpEntry("PRNT [text]", "print text (supports $variables)"),
        HelpEntry("VAR [name] = [val]", "define a variable"),
        HelpEntry("INPT [varname]", "read stdin into variable"),
        HelpEntry("GOTO [label]", "jump to label"),
        HelpEntry(":label", "define a label"),
        HelpEntry("IF var [operand] val : [action]", "run action if condition met (Supported operands: == != < > <= >= )"),
        HelpEntry("RNG [max] [var] or RNG [min] [max] [var]", "writes a random value between [min] (default 0, inclusive) and [max] (exclusive!) into a variable"),
        HelpEntry("DMP", "dump program data (debug only)"),
        HelpEntry("HLP", "show help message"),
        HelpEntry("EXIT", "abort program execution"),
        HelpEntry("_comment", "ignored line")};

private:
    static bool starts_with(const string &s, const string &prefix)
    {
        return s.find(prefix) == 0;
    }

    int bounded_rand(int min, int max)
    {
        return min + (rand() % (max - min));
    }

    static string trim(const string &s)
    {
        size_t start = s.find_first_not_of(" \t\n\r");
        size_t end = s.find_last_not_of(" \t\n\r");
        return (start == string::npos) ? "" : s.substr(start, end - start + 1);
    }

    vector<string> split(const string &str, char delimiter)
    {
        vector<string> result;
        stringstream ss(str);
        string item;
        debug_print("Splitting str: " + str);
        while (getline(ss, item, delimiter))
        {
            result.push_back(item);
            debug_print("Got tok: " + item);
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

    void print_help()
    {
        int mnlen = 0;
        for (int i = 0; i < helpData.size(); i++)
        {
            HelpEntry entry = helpData[i];
            if (entry.name.size() > mnlen)
                mnlen = entry.name.size();
        }
        cout << "SIPL Token list\n\n";
        for (int i = 0; i < helpData.size(); i++)
        {
            HelpEntry entry = helpData[i];
            cout << entry.name << ((string) " " * (mnlen - entry.name.size() + 2)) << " - " << entry.desc << endl;
        }
    }

    void error(const string &msg, const string &line = "")
    {
        cerr << "[ERROR] " << msg;
        if (!line.empty())
            cerr << " | Line: \"" << line << "\"";
        cerr << endl;
    }

public:
    Interpreter(bool debug = 0)
    {
        this->debug_verbose = debug;
    }
    int addLines(string l){
        int ret=lines.size();
        for (string line : split(l, ';'))
        {
            if (line.empty())
                continue;
            if (line[0] == '_')
                continue;
            lines.push_back(line);
        }
        return ret;
    }
    int find_label(string name){
        debug_print("Checking label " + name);
        for(pair<int,string> label:labels){
            if(label.second==name) return label.first;
        }
        debug_print("Label not found");
        return -1;
    }
    string eval_expr(const string &s)
    {
        if (s.empty())
            return "";
        vector<string> toks = split(s, ' ');
        vector<int> stack;
        for (int ti = 0; ti < toks.size(); ti++)
        {
            string t = toks[ti];
            if (isdigit(t[0]) || (t[0] == '-' && t.size() > 1))
            {
                stack.push_back(stoi(t));
            }
            else if (t[0] == '$')
            {
                if (t[1] == '(')
                {
                    debug_print("Found possible expression definition;");
                    int sti = ti;
                    string expr = "";
                    bool found = false;
                    while (!found)
                    {
                        string tok = toks[sti];
                        for (int ci = 0; ci < tok.size(); ci++)
                        {
                            char c = tok[ci];
                            if (sti == ti && ci <= 1)
                                continue;
                            if (c == ')')
                            {
                                found = 1;
                                break;
                            }
                            else if (c == ';')
                            {
                                found = 0;
                                break;
                            }
                            else
                            {
                                expr += c;
                            }
                        }
                        expr += " ";
                        sti++;
                    }
                    if (!found)
                    {
                        error("Unmatched () in expr " + s);
                        return "";
                    }
                    else
                    {
                        ti = sti - 1;
                        return eval_expr(expr);
                    }
                }
                else
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
    int exec_line(string line, int &i)
    {
        try
        {
            if (line.empty() || line[0] == ':' || line[0] == '_')
            {
                debug_print("Line empty");
                return 1;
            }
            vector<string> arg = split(line, ' ');

            if (arg[0] == "PRNT")
            {
                handle_echo(trim(line.substr(4)));
            }
            else if (arg[0] == "VAR")
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
            else if (arg[0] == "GOTO")
            {
                string label = arg[1];
                int labelpos = find_label(label);
                if(labelpos==-1){
                    error("Label not found: " + label, line);
                }
                else{
                    debug_print("Label found at line " + to_string(i + 1) + ", jumping");
                    i = labelpos - 1;
                }
            }
            else if (arg[0] == "IF")
            {
                string rest = trim(line.substr(2));
                size_t colon = rest.find(':');
                if (colon == string::npos)
                {
                    error("Invalid IF syntax", line);
                }
                string condition = trim(rest.substr(0, colon));
                string action = trim(rest.substr(colon + 1));
                // TODO: remove and replace with expression handler
                regex comp_regex(R"((\w+)\s*(==|!=|<|>|<=|>=)\s*(\"?.+?\"?))");
                smatch match;
                if (regex_match(condition, match, comp_regex))
                {
                    debug_print("IF REGEX DUMP");
                    for (int i = 0; i < match.length(); i++)
                    {
                        string data = match[i];
                        debug_print("[" + to_string(i) + "] " + data);
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
            else if (arg[0] == "INPT")
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
            else if (arg[0] == "HLP")
            {
                print_help();
            }
            else if (arg[0] == "EXIT")
            {
                debug_print("Exiting.");
                i = lines.size();
                return -1;
            }
            else if (arg[0] == "DMP")
            {
                debug_print("PROGRAM DATA DUMP:");
                debug_print("## LABELS:");
                for (pair<int, string> label : labels)
                {
                    debug_print("- " + label.second + " at line " + to_string(label.first + 1));
                }
                debug_print("## VARIABLES:");
                for (pair<string, string> var : vars)
                {
                    debug_print("- " + var.first + " = " + var.second);
                }
                debug_print("## ARRAYS:");
                for (pair<string, vector<string>> arr : arrays)
                {
                    debug_print("- " + arr.first);
                    for (string v : arr.second)
                    {
                        debug_print("| - " + v);
                    }
                }
                cout << "P. S. If you see no output, you might have debug mode disabled.\n";
            }
            else if (arg[0] == "RNG")
            {
                int min_value = 0;
                int max_value = 0;
                string var_name = "";
                bool failed = false;
                if (arg.size() == 3)
                {
                    max_value = stoi(arg[1]);
                    var_name = arg[2];
                }
                else if (arg.size() == 4)
                {
                    min_value = stoi(arg[1]);
                    max_value = stoi(arg[2]);
                    var_name = arg[3];
                }
                else
                {
                    error("Not enough or too many arguments for RNG");
                    failed = true;
                }
                if (max_value <= 0)
                {
                    error("Maximum value for RNG should not be less than or equal to zero.");
                    failed = true;
                }
                if(min_value >= max_value){
                    error("The minimum value for RNG could not be larger than the maximum value.");
                }
                if (!failed)
                {
                    vars[var_name] = to_string(bounded_rand(min_value,max_value));
                }
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
        debug_print("Running in debug mode");
        lines = vector<string>();
        labels = map<int,string>();
        vars = map<string, string>();
        addLines(program);

        for (int i = 0; i < lines.size(); ++i)
        {
            string line = trim(lines[i]);
            if (!line.empty() && line[0] == ':')
            {
                string label = trim(line.substr(1));
                debug_print("Found label " + label + " at line " + to_string(i + 1));
                labels[i] = label;
            }
        }

        for (int i = 0; i < lines.size(); ++i)
        {
            string line = trim(lines[i]);
            debug_print("L " + to_string(i + 1) + " / " + to_string(lines.size()) + " : " + line);
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
    srand(time(0));
    string fpath;
    bool debug = 0;
    for (int argn = 1; argn < argc; argn++)
    {
        string sa = argv[argn];
        if (sa == "--debug" || sa == "-d")
        {
            debug = 1;
        }
        else if (sa == "-f" || sa == "--file")
        {
            if (argc <= argn + 1)
            {
                cerr << "Parameterized argument without parameter" << endl;
                return 1;
            }
            fpath = argv[argn + 1];
            argn++;
        }
        else if (sa == "-h" || sa == "--help")
        {
            cout << "SIPLI argument list\n"
                 << "-h | --help        - show this message\n"
                 << "-f | --file [file] - run file\n"
                 << "\n"
                 << "-d | --debug       - enable debug mode for extended debug debugging of debugger (obsolete (no))"
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
        Interpreter x(debug);
        x.run(buffer.str());
    }
    else
    {
        cout << "SIPLI version " << SIPL_VER << SIPLI_APPENDIX << "\nUse HLP for help or pass -h argument for parameter list.\n";
        Interpreter x(debug);
        while (1)
        {
            cout << ">>> ";
            string q;
            getline(cin, q);
            if (!x.run(q))
            {
                break;
            }
        }
    }

    return 0;
}