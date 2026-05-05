#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <deque>
#include <string>
#include <algorithm>
#include <set>

using namespace std;

const std::string WLP4_CFG = R"END(.CFG
start BOF procedures EOF
procedures procedure procedures
procedures main
procedure INT ID LPAREN params RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE
main INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE
params .EMPTY
params paramlist
paramlist dcl
paramlist dcl COMMA paramlist
type INT
type INT STAR
dcls .EMPTY
dcls dcls dcl BECOMES NUM SEMI
dcls dcls dcl BECOMES NULL SEMI
dcl type ID
statements .EMPTY
statements statements statement
statement lvalue BECOMES expr SEMI
statement IF LPAREN test RPAREN LBRACE statements RBRACE ELSE LBRACE statements RBRACE
statement WHILE LPAREN test RPAREN LBRACE statements RBRACE
statement PRINTLN LPAREN expr RPAREN SEMI
statement PUTCHAR LPAREN expr RPAREN SEMI
statement DELETE LBRACK RBRACK expr SEMI
test expr EQ expr
test expr NE expr
test expr LT expr
test expr LE expr
test expr GE expr
test expr GT expr
expr term
expr expr PLUS term
expr expr MINUS term
term factor
term term STAR factor
term term SLASH factor
term term PCT factor
factor ID
factor NUM
factor NULL
factor LPAREN expr RPAREN
factor AMP lvalue
factor STAR factor
factor NEW INT LBRACK expr RBRACK
factor ID LPAREN RPAREN
factor ID LPAREN arglist RPAREN
factor GETCHAR LPAREN RPAREN
arglist expr
arglist expr COMMA arglist
lvalue ID
lvalue STAR factor
lvalue LPAREN lvalue RPAREN
)END";

std::ostream& operator<<(std::ostream& os, const std::map<std::string, std::string>& m) {
    os << "{";
    for (auto it = m.begin(); it != m.end(); ++it) {
        os << "\"" << it->first << "\": \"" << it->second << "\"";
        if (std::next(it) != m.end()) {
            os << ", ";
        }
    }
    os << "}";
    return os;
}

map<string, string> typeName{
    {"INT", "int"},
    {"INT STAR", "int*"}
};


set<string> getAllProductionRules(string s) {
    std::istringstream allRules(s);
    string line;
    set<string> allProductionRules;
            
    while (getline(allRules, line)) {
        allProductionRules.insert(line);
    }
    return allProductionRules;
}

set<string> allProductionRules = getAllProductionRules(WLP4_CFG);

class Node {
    public:
        string kind;
        string value;
        string type;
        vector<Node> childrens;
        Node(const string& kind,
             const string& value,
             const string& type,
             const vector<Node>& childrens)
             : kind(kind), value(value), type(type), childrens(childrens){};
        
};

bool isUpper(const string& str) {
    for (char ch : str) {
        if (isalpha(ch) && !isupper(ch)) {
            return false;
        }
    }
    return true;
}
class TreeBuilder {
    vector<string> split(const string& inp) {
        vector<string> res;
        istringstream iss(inp);
        string val;

        while (iss >> val) {
            res.push_back(val);
        }

        return res;
    }
    int index = 0;
    public:
        Node build(vector<string> &lines) {
            auto currline = lines[index];
            index += 1;
            auto vals = split(currline);
            
            if (allProductionRules.find(currline) == allProductionRules.end() || vals[1] == ".EMPTY") {
                return Node(vals[0], vals[1], "", vector<Node>{});
            }
            auto newNode = Node(vals[0], currline.substr(vals[0].size() + 1), "", vector<Node>{});
            vector<Node> parsedChildrens;


            for (int i = 0; i < vals.size() - 1; i++){
                parsedChildrens.push_back(build(lines));
            }

            newNode.childrens = parsedChildrens;
            return newNode;
        };
};

map <string, map<string, string>> symbolTypes;
map <string, vector<string>> procedureParams;

void assignRecursive (Node &node, string type) {
    auto kindsToUpdate = vector<string>{"expr", "term", "factor", "NUM", "NULL", "ID"};
    if (find(kindsToUpdate.begin(), kindsToUpdate.end(), node.kind) != kindsToUpdate.end()) {
        node.type = type;
    } 

    for (auto &child : node.childrens) {
        assignRecursive(child, type);
    } 
}

vector<string> getParams(Node &node);
vector<string> getArgs(Node &node);




string currProcedure = "";
string getType(Node &node){
    if (node.kind == "NUM") {
        node.type = "INT";
        return "INT";
    }
    if (node.kind == "NULL") {
        node.type = "INT STAR";
        return "INT STAR";
    }
    if (node.kind == "ID") {
        auto varName = node.value;
        if (symbolTypes[currProcedure].find(varName) == symbolTypes[currProcedure].end()) {
            cerr << "ERROR: " << varName << "is not declared " << currProcedure << "procedure" << endl;
            return "";
        };
        node.type = symbolTypes[currProcedure][varName];
        return node.type;

    } 

    if (node.kind == "type") {
        return node.value;
    }
    // if ((node.kind == "ID") && (symbolTypes[currProcedure].find(node.value) != symbolTypes[currProcedure].end())){
    //     return symbolTypes[currProcedure][node.value]; 
    // } 
    
    if (node.kind == "expr") {
        if (node.childrens.size() == 1) {
            node.type = getType(node.childrens[0]);
            // assignRecursive(node, node.type);
        } 
        else if (node.value == "expr PLUS term") {
            auto addReturnType = std::map<std::pair<std::string, std::string>, std::string>{
                {{"INT", "INT"}, "INT"},
                {{"INT STAR", "INT"}, "INT STAR"},
                {{"INT", "INT STAR"}, "INT STAR"},
            };

            auto lhsType = getType(node.childrens[0]);
            auto rhsType = getType(node.childrens[2]);

            auto key = make_pair(lhsType, rhsType);
            if (addReturnType.find(key) == addReturnType.end()) {
                cerr << "Error: Type combination "<< lhsType << " , " << rhsType << " for expr PLUS term is not supported." << std::endl;
                return "";
            };
            node.type = addReturnType[{lhsType, rhsType}];
        }
        else if (node.value == "expr MINUS term") {
            auto subReturnType = std::map<std::pair<std::string, std::string>, std::string>{
                {{"INT", "INT"}, "INT"},
                {{"INT STAR", "INT"}, "INT STAR"},
                {{"INT STAR", "INT STAR"}, "INT"},
            };

            auto lhsType = getType(node.childrens[0]);
            auto rhsType = getType(node.childrens[2]);

            auto key = make_pair(lhsType, rhsType);
            if (subReturnType.find(key) == subReturnType.end()) {
                cerr << "Error: Type combination "<< lhsType << " , " << rhsType << " for expr MINUS term is not supported." << std::endl;
                return "";
            };
            node.type = subReturnType[{lhsType, rhsType}];

        } else {
            cerr << "getType only handle expr of size 1 and 3" << endl;
        }

        return node.type;
    }

    if (node.kind == "term") {
        if (node.childrens.size() == 1) {
            node.type = getType(node.childrens[0]);
            // assignRecursive(node, node.type);
        }
        else if (node.childrens.size() == 3) {
            auto starDivModReturnType = std::map<std::pair<std::string, std::string>, std::string>{
                {{"INT", "INT"}, "INT"},
            };

            auto lhsType = getType(node.childrens[0]);
            auto rhsType = getType(node.childrens[2]);

            auto key = make_pair(lhsType, rhsType);
            if (starDivModReturnType.find(key) == starDivModReturnType.end()) {
                cerr << "Error: Type combination "<< lhsType << " , " << rhsType << " for Plus and Sub is not supported." << std::endl;
                return "";
            };
            node.type = starDivModReturnType[key];
            // assignRecursive(node.childrens[0], lhsType);
            // assignRecursive(node.childrens[2], rhsType);


        } else {
            cerr << "getType only handle term of size 1 and 3" << endl;
        }

        return node.type;

    }

    
    if (node.kind == "factor") {
        if (node.value == "ID LPAREN RPAREN"){
            auto procName = node.childrens[0].value;
            if (procedureParams.find(procName) == procedureParams.end()) {
                cerr << "ERROR: procedure " << procName << "is not declared "<< endl;
                return "";
            }
            if (procedureParams[procName].size() != 0) {
                cerr << "ERROR: procedure " << procName << "is not supposed to be called with 0 params "<< endl;
                return "";
            }
            // special case if the function is also a variable:
            if (symbolTypes[currProcedure].find(procName) != symbolTypes[currProcedure].end()) {
                cerr << "ERROR: procedure " << procName << " cant be called because there is a variable with the same name "<< endl;
                return "";
            }
            node.type = "INT";
        }
        else if (node.value == "ID LPAREN arglist RPAREN") {
            auto procName = node.childrens[0].value;
            if (procedureParams.find(procName) == procedureParams.end()) {
                cerr << "ERROR: procedure " << procName << "is not declared "<< endl;
                return "";
            }

            vector<string> argTypes = getArgs(node.childrens[2]);
            if (argTypes != procedureParams[procName]) {
                cerr << "ERROR: procedure " << procName << " call has mismatch type/number of params-args "<< endl;
                return "";
            }
            // special case if the function is also a variable:
            if (symbolTypes[currProcedure].find(procName) != symbolTypes[currProcedure].end()) {
                cerr << "ERROR: procedure " << procName << " cant be called because there is a variable with the same name "<< endl;
                return "";
            }
            node.type = "INT";
        }
        else if (node.value == "GETCHAR LPAREN RPAREN") {
            node.type = "INT"; 
        }
        else if (node.value == "NUM") {
            node.type = getType(node.childrens[0]);
        }
        else if (node.value == "NULL") {
            node.type = getType(node.childrens[0]);
        }
        else if (node.value == "ID") {
            auto varName = node.childrens[0].value;
            if (symbolTypes[currProcedure].find(varName) == symbolTypes[currProcedure].end()) {
                cerr << "ERROR: " << varName << "is not declared in " << currProcedure << "procedure" << endl;
                return "";
            };
            node.type= symbolTypes[currProcedure][varName];

        } 
        else if (node.value == "LPAREN expr RPAREN") {
            node.type = getType(node.childrens[1]); 
        }

        else if (node.value == "AMP lvalue") {
            auto otherType = getType(node.childrens[1]);
            if (otherType != "INT") {
                cerr << "Error: Type " << otherType << "factor AMP lvalue is not supported" << std::endl;
                return "";
            };
            
            node.type = "INT STAR";
        }
        else if (node.value == "STAR factor") {
            auto otherType = getType(node.childrens[1]);
            if (otherType != "INT STAR") {
                cerr << "Error: Type " << otherType << "factor STAR factor is not supported" << std::endl;
                return "";
            };
            
            node.type = "INT";
        }
        else if (node.value == "NEW INT LBRACK expr RBRACK") {
            auto otherType = getType(node.childrens[3]);
            if (otherType != "INT") {
                cerr << "Error: Type " << otherType << "factor NEW INT LBRACK expr RBRACK is not supported" << std::endl;
                return "";
            };
            
            node.type = "INT STAR"; 
        }
        else {
            cerr << node.value << "Rule is not supported in factor" << endl; 
        }

        return node.type;
    }

    if (node.kind == "lvalue") {
        if (node.value == "ID"){
            // cout << "lvalue ID reached " << node.kind << " " << node.value << endl;
            node.type = getType(node.childrens[0]);
        }
        else if (node.value == "STAR factor"){
            auto otherType = getType(node.childrens[1]);
            if (otherType != "INT STAR") {
                cerr << "Error: Type " << otherType << "factor STAR factor is not supported" << std::endl;
                return "";
            };
            
            node.type = "INT"; 
        }
        else if (node.value == "LPAREN lvalue RPAREN"){
            node.type = getType(node.childrens[1]); 
        }
        else {
            cerr << node.value << "Rule is not supported in lvalue" << endl; 
        }
        return node.type;

    }

    // if (node.kind == "test") {}
    if (node.childrens.size() >= 1) return getType(node.childrens[0]);
    
    return "";

}

vector<string> getParamsFromParamList(Node &node){
    vector<string> paramTypes{};
    paramTypes.push_back(getType(node.childrens[0]));

    if (node.childrens.size() >= 3){
        for (auto &t : getParamsFromParamList(node.childrens[2])) {
            paramTypes.push_back(t);
        }
    }
    return paramTypes;
}
vector<string> getParams(Node &node) {
    vector<string> paramTypes{};
    if (node.value == ".EMPTY") {
        return paramTypes;
    }
    return getParamsFromParamList(node.childrens[0]);
};
vector<string> getArgs(Node &node){
    vector<string> argTypes{};
    argTypes.push_back(getType(node.childrens[0]));
    if (node.childrens.size() == 1) {
        return argTypes;
    }
    for (auto &t : getArgs(node.childrens[2])) {
        argTypes.push_back(t);
    }
    return argTypes;
}; 

void checkAndAssign(Node &node) {
    // cout << node.kind << " " << node.value << endl;
    vector<int> childrenToSkip{};
    if (node.kind == "procedure") {
        // Check if procedure has already been declared
        auto newProcedure = node.childrens[1].value;
        if (symbolTypes.find(newProcedure) != symbolTypes.end()){
            cerr << "ERROR: " << newProcedure << " procedure has already been declared" << endl;
        }
        symbolTypes[newProcedure] = map<string, string>{};
        currProcedure = newProcedure;
        procedureParams[currProcedure] = getParams(node.childrens[3]); 
        // Update procedure name

        childrenToSkip.push_back(1); // index 1 will be skip: i.e: the ID
    }
    if (node.kind == "main") {
        auto newProcedure = "wain";
        if (symbolTypes.find(newProcedure) != symbolTypes.end()){
            cerr << "ERROR: " << newProcedure << " procedure has already been declared" << endl;
        }
        symbolTypes[newProcedure] = map<string, string>{};
        // Update procedure name
        currProcedure = newProcedure;
        childrenToSkip.push_back(1); // index 1 will be skip: i.e: the ID
        currProcedure = "wain";
    }
    if (node.kind == "ID") {
        auto varName = node.value;
        if (symbolTypes[currProcedure].find(varName) == symbolTypes[currProcedure].end()) {
            cerr << "ERROR: " << varName << " is not declared " << currProcedure << " procedure" << endl;
        };

    }
    if (node.kind == "dcl") {
        auto varType = getType(node.childrens[0]);
        auto varName = node.childrens[1].value;
        node.childrens[1].type = varType;

        if (symbolTypes[currProcedure].find(varName) != symbolTypes[currProcedure].end()) {
            cerr << "ERROR: " << varName << "is already defined in " << currProcedure << "procedure" << endl;
        }
        symbolTypes[currProcedure][varName] = varType;
        
    }

    if (node.kind == "dcls") {
        if (node.childrens.size() >= 4) {
            auto lhsType = getType(node.childrens[1]);
            auto rhsType = getType(node.childrens[3]);
            if (lhsType != rhsType) {
                cerr << "ERROR : lhs of dcl is not the same type as rhs" << endl;
            };
        }
    }
    if (node.kind == "test"){
        auto lhsType = getType(node.childrens[0]);
        auto rhsType = getType(node.childrens[2]);
        if (lhsType != rhsType) {
            cerr << "ERROR : lhs and rhs of a test is not of same type" << endl;
        }; 
    }
    if (node.kind == "factor"){
        set<string> functionCallRules{
            "ID LPAREN RPAREN",
            "ID LPAREN arglist RPAREN",
        };
        if (functionCallRules.find(node.value) != functionCallRules.end()){
            childrenToSkip.push_back(0);
        }
    }
    if (node.kind == "statement") {
        if (node.value == "lvalue BECOMES expr SEMI") {
            auto lhsType = getType(node.childrens[0]);
            auto rhsType = getType(node.childrens[2]);
            if (lhsType != rhsType) {
                cerr << "ERROR : lhs and rhs of "<< node.kind << " -> " << node.value << " is not of same type" << endl;
            };  
        } 
        else if (node.value == "PRINTLN LPAREN expr RPAREN SEMI" || node.value == "PUTCHAR LPAREN expr RPAREN SEMI") {
            auto exprType = getType(node.childrens[2]);
            if (exprType != "INT") {
                cerr << "ERROR : putchar and println only take in int type, not " << exprType << endl; 
            }
        }
        else if (node.value == "DELETE LBRACK RBRACK expr SEMI") {
            auto exprType = getType(node.childrens[3]);
            if (exprType != "INT STAR") {
                cerr << "ERROR : delete only take in int star type, not " << exprType << endl; 
            }
        }
    }
    auto _ = getType(node);


    for (int i = 0; i < node.childrens.size(); i++) {
        if (find(childrenToSkip.begin(), childrenToSkip.end(), i) == childrenToSkip.end()){
            checkAndAssign(node.childrens[i]);
        }
    }
    if (node.kind == "procedure") {
        // Check return
        if (getType(node.childrens[9]) != "INT") {
            cerr << "ERROR: The return expression of " << currProcedure << " is not int type." << endl; 
        }
    }

    if (node.kind == "main") {        
        // check if second param is not int type:
        if (getType(node.childrens[5]) != "INT") {
            cerr << "ERROR: The second parameter of wain is not int type." << endl;
        }
        
        if (getType(node.childrens[11]) != "INT") {
            cerr << "ERROR: The return expression is not int type." << endl; 
        }
    }


}

void print(Node root) {
    cout << root.kind << " " << root.value;

    if (root.type != "") {
        cout << " : " << typeName[root.type];
    };

    cout << endl;

    for (auto &child : root.childrens) {
        print(child);
    }
}


int main() {
    string line;
    vector<string> inputLines;
    while (getline(cin, line)) {
        if (line.empty()) continue;
        
        istringstream iss(line);
        string type, token;

        inputLines.push_back(line);
    }

    auto tb = TreeBuilder();
    auto root = tb.build(inputLines);

    checkAndAssign(root);
    print(root);
}