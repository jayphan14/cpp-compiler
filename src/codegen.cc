#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <deque>
#include <string>
#include <algorithm>
#include <set>
#include <memory>
#include <utility>
#include <cassert>

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

vector<string> split(const string& inp) {
    vector<string> res;
    istringstream iss(inp);
    string val;

    while (iss >> val) {
        res.push_back(val);
    }

    return res;
}

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
        vector<shared_ptr<Node>> childrens;
        Node(const string& kind,
             const string& value,
             const string& type,
             const vector<shared_ptr<Node>>& childrens)
             : kind(kind), value(value), type(type), childrens(childrens){};
        
    virtual void print() const {
        cout << kind << " " << value;
        if (!type.empty()) {
            cout << " : " << type;
        }
        cout << endl;
        for (const auto& child : childrens) {
            child->print();
        }
    }
};

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

    pair<string,string> getType(string &line) {
        size_t pos = line.find(":");

        if (pos != std::string::npos) {
            std::string currline = line.substr(0, pos-1);
            std::string type = line.substr(pos + 2);
            return std::pair<string,string>{currline, type};
        }

        return std::pair<string,string>{line, ""};
    }

    public:
        shared_ptr<Node> build(vector<string> &lines) {
            auto [currline, type] = getType(lines[index]);

            index += 1;
            auto vals = split(currline);
            
            if (allProductionRules.find(currline) == allProductionRules.end() || vals[1] == ".EMPTY") {
                return make_shared<Node>(vals[0], vals[1], type, vector<shared_ptr<Node>>{});
            }
            auto newNode = make_shared<Node>(vals[0], currline.substr(vals[0].size() + 1), type, vector<shared_ptr<Node>>{});
            vector<shared_ptr<Node>> parsedChildrens;


            for (int i = 0; i < vals.size() - 1; i++){
                parsedChildrens.push_back(build(lines));
            }

            newNode->childrens = parsedChildrens;
            return newNode;
        };
};

class CodeGenerator {
    string currProcedure = "";
    map<string, map<string, int>> varsInfo;
    map<string, int> numArgForProcedure;
    int currLabelCounter = 0;
    set<string> import;
    vector<string> mainOutput;
    shared_ptr<Node> getLeafNode(shared_ptr<Node> node)
    {
        if (node->kind == "expr") return getLeafNode(node->childrens[0]);
        if (node->kind == "term") return getLeafNode(node->childrens[0]);
        if (node->kind == "factor")
        {
            if (split(node->value).size() == 3) return getLeafNode(node->childrens[1]);
            
            return node->childrens[0];
        }

        return node;
    }
    void updateParamsOffset() {
        int numParams = varsInfo[currProcedure].size() - 1;
        for (auto &[key, value] : varsInfo[currProcedure]) {
            varsInfo[currProcedure][key] += numParams * 4;
        }
        varsInfo[currProcedure]["currVarsOffset"] = 0;
    }
    public:
        int getUnUsedReg(){ return 10; }

        vector<string> lis(int regNumber, string value) {
            stringstream ss1, ss2;
            ss1 << "lis $" << regNumber;
            ss2 << ".word " << value;
            return {ss1.str(), ss2.str()};
        }
        vector<string> code(int regNumber, string varName) {
            stringstream ss1;
            ss1 << "lw $" << regNumber << ", " << varsInfo[currProcedure][varName] << "($29)";
            return {ss1.str()};
        }
        vector<string> push(int regNumber) {
            stringstream ss1;
            ss1 << "sw $" << regNumber << ", -4( $30 )";
            return {ss1.str(), "sub $30 , $30 , $4"};
        }
        vector<string> pop(int regNumber) {
            stringstream ss1;
            ss1 <<  "lw $"<< regNumber<< ", -4( $30 )";
            return {"add $30 , $30 , $4", ss1.str()};
        }
        pair<int, vector<string>> pushArgsOnStack(shared_ptr<Node> node) {
            assert(node->kind == "arglist" && "this fn expects paramNode->kind to be 'arglist'"); 
            int totalArg = 0;
            vector<string> codes;

            for (auto &line : generateExprValueToReg(3, node->childrens[0])) {codes.push_back(line);};
            for (auto &line : push(3)) {codes.push_back(line);};
            totalArg += 1;


            if (node->childrens.size() >= 3) {
                auto [count, res] = pushArgsOnStack(node->childrens[2]);
                totalArg += count;
                for (auto &line : res) {codes.push_back(line);};
            }

            return {totalArg, codes};
        }

        vector<string> generateParamDcl(shared_ptr<Node> node, int regNum) {
            assert(node->kind == "dcl" && "this fn expects paramNode->kind to be 'dcl'");
            auto varName = node->childrens[1]->value;

            varsInfo[currProcedure][varName] = varsInfo[currProcedure]["currVarsOffset"];
            varsInfo[currProcedure]["currVarsOffset"] -= 4;
            std::stringstream ss1;
            ss1  << "sw $" << regNum << " , -4($30)";
            return {ss1.str(), "sub $30 , $30 , $4"};
        };
        void mapParamsToOffsetTable_Helper(shared_ptr<Node> node){
            assert(node->kind == "paramlist" && "this fn expects node->kind to be 'param'");
            auto dclNode = node->childrens[0];
            auto varName = dclNode->childrens[1]->value;
            varsInfo[currProcedure][varName] = varsInfo[currProcedure]["currVarsOffset"];
            varsInfo[currProcedure]["currVarsOffset"] -= 4;

            if (node->childrens.size() >= 3) {
                mapParamsToOffsetTable_Helper(node->childrens[2]);
            }
        }
        void mapParamsToOffsetTable(shared_ptr<Node> node){
            assert(node->kind == "params" && "this fn expects node->kind to be 'params'");
            if (node->childrens.size() == 0) { return ;}
            mapParamsToOffsetTable_Helper(node->childrens[0]); 
        }

        vector<string> generateDcls(shared_ptr<Node> node){
            assert(node->kind == "dcls" && "this fn expects paramNode->kind to be 'dcls'");
            if (node->value == ".EMPTY") return {};
            
            vector<string> output;
            for (auto &line: generateDcls(node->childrens[0])) {
                output.push_back(line);
            }

            auto dclNode = node->childrens[1];
            auto varName = dclNode->childrens[1]->value;
            auto varValue = node->childrens[3]->value;

            varsInfo[currProcedure][varName] = varsInfo[currProcedure]["currVarsOffset"];
            varsInfo[currProcedure]["currVarsOffset"] -= 4;

            if (varValue == "NULL") {
                varValue = "1";
            }
            int unUsedReg = getUnUsedReg(); ; // MIGHT WANT TO MAKE THIS A FUNCTION IN THE FUTURE
        
            for (auto &line: lis(unUsedReg, varValue)) {
                output.push_back(line);
            }
            std::stringstream ss1;
            ss1  << "sw $" << unUsedReg << " , -4( $30 )";
            output.push_back(ss1.str());
            output.push_back("sub $30 , $30 , $4");

            return output;
        };

        // return asm codes to store [address]  in [reg]
        vector<string> getAddressFromLvalue(int reg, shared_ptr<Node> node){
            vector<string> codes;
            // assert((node->kind == "lvalue" || node->kind == "factor")&& "this fn expects paramNode->kind to be 'lvalue' or 'factor");
            // lvalue → ID
            if (node->childrens.size() == 1) {
                auto varName = node->childrens[0]->value;
                string offset = to_string(varsInfo[currProcedure][varName]);
                for (auto &line : lis(reg, offset)) {codes.push_back(line);};
                stringstream output;
                output << "add $" << reg << ", $" << reg << ", $29";
                codes.push_back(output.str());
                return codes;
            } 
            // lvalue → LPAREN lvalue RPAREN
            else if (node->value == "LPAREN lvalue RPAREN"){
                return getAddressFromLvalue(reg, node->childrens[1]);
            }
            // lvalue → STAR factor
            else if (node->childrens.size() == 2){
                for (auto &line: generateExprValueToReg(reg, node->childrens[1])) { codes.push_back(line);};
                return codes;
            }
            else {
                cerr << "NOT SUPPORTED FOR A7 : " << node->kind << "-> " << node->value << endl;
                return codes;
            }
        }
        
        vector<string> generateExprValueToReg(int regNum, shared_ptr<Node> node) {
            vector<string> output;
            set<string> threeChildrensWithOperations = {
                "expr PLUS term", 
                "expr MINUS term",
                "term STAR factor", 
                "term SLASH factor",
                "term PCT factor",
                "expr EQ expr",
                "expr NE expr",
                "expr LT expr",
                "expr LE expr",
                "expr GE expr",
                "expr GT expr" 
            };

            int tempReg = 6;
            
            if (threeChildrensWithOperations.find(node->value) != threeChildrensWithOperations.end()){
                for (auto &line: generateExprValueToReg(regNum, node->childrens[0])) { output.push_back(line);};
                for (auto &line: push(regNum)) { output.push_back(line);};
                for (auto &line: generateExprValueToReg(regNum, node->childrens[2])) { output.push_back(line);};
                for (auto &line: pop(tempReg)) { output.push_back(line);};

                auto values = split(node->value);
                stringstream out;
                for (auto &line : lis(21, "1")) {
                    output.push_back(line);
                }

                string compMipsFunction = "slt";
                if (node->childrens[0]->type == "int*") {
                   compMipsFunction = "sltu"; 
                }
                if (values[1] == "PLUS"){
                    if (node->childrens[0]->type == "int*") {
                        vector<string> output;
                        // code (expr2 )
                        output.push_back(";code (expr2 )");
                        for (auto &line: generateExprValueToReg(regNum, node->childrens[0])) {output.push_back(line);};
                        // push ($3)
                        output.push_back(";push ($3)");
                        for (auto &line: push(regNum)) {output.push_back(line);};
                        // code ( term )
                        output.push_back(";code ( term )"); 
                        for (auto &line: generateExprValueToReg(regNum, node->childrens[2])) {output.push_back(line);};
                        // mult $3 , $4 ; $4 always has the value 4

                        out.str("");
                        out << "mult $"<< regNum <<" , $" << 4;
                        output.push_back(out.str());
                        out.str(""); 
                        // mflo $3
                        
                        out.str("");
                        out << "mflo $"<< regNum;
                        output.push_back(out.str());
                        out.str(""); 
                        
                        // pop ($5) ; $5 <- expr2
                        output.push_back(";pop ($5) ; $5 <- expr2 ");  
                        int tempReg = getUnUsedReg();
                        if (tempReg == regNum) { tempReg += 1;}
                        for (auto &line: pop(tempReg)) {output.push_back(line);};
                        // add $3 , $5 , $3
                        out.str("");
                        out << "add $"<< regNum << ", $" << tempReg << ", $" << regNum;
                        output.push_back(out.str());
                        out.str("");
                        return output;
                    }
                    else if (node->childrens[2]->type == "int*") {
                        vector<string> output;
                        // code (expr2 )
                        for (auto &line: generateExprValueToReg(regNum, node->childrens[2])) {output.push_back(line);};
                        // push ($3)
                        for (auto &line: push(regNum)) {output.push_back(line);};
                        // code ( term )
                        for (auto &line: generateExprValueToReg(regNum, node->childrens[0])) {output.push_back(line);};
                        // mult $3 , $4 ; $4 always has the value 4
                        out.str("");
                        out << "mult $"<< regNum <<" , $" << 4;
                        output.push_back(out.str());
                        out.str(""); 
                        // mflo $3
                        out.str("");
                        out << "mflo $"<< regNum;
                        output.push_back(out.str());
                        out.str(""); 
                        
                        // pop ($5) ; $5 <- expr2
                        int tempReg = getUnUsedReg();
                        if (tempReg == regNum) { tempReg += 1;}
                        for (auto &line: pop(tempReg)) {output.push_back(line);};
                        // add $3 , $5 , $3
                        out.str("");
                        out << "add $"<< regNum << ", $" << tempReg << ", $" << regNum;
                        output.push_back(out.str());
                        out.str("");
                        return output;  
                    } else {
                        out << "add $"<< regNum <<" , $" << tempReg <<" , $" << regNum; 
                        output.push_back(out.str());
                    };
                }
                else if (values[1] == "MINUS"){
                    if (node->childrens[0]->type == "int*" && node->childrens[2]->type == "int*") {
                        vector<string> output;
                        // code (expr2 )
                        for (auto &line: generateExprValueToReg(regNum, node->childrens[0])) {output.push_back(line);};
                        // push ($3)
                        for (auto &line: push(regNum)) {output.push_back(line);};
                        // code ( term )
                        for (auto &line: generateExprValueToReg(regNum, node->childrens[2])) {output.push_back(line);};
                        // pop ($5) ; $5 <- expr2 
                        int tempReg = getUnUsedReg();
                        for (auto &line: pop(tempReg)) {output.push_back(line);};
                        
                        // sub $3 , $5 , $3
                        out.str("");
                        out << "sub $"<< regNum << ", $" << tempReg << ", $" << regNum;
                        output.push_back(out.str());
                        out.str("");

                        // div $3 , $4 ; $4 always has the value 4

                        out.str("");
                        out << "div $"<< regNum <<" , $" << 4;
                        output.push_back(out.str());
                        out.str(""); 
                        
                        // mflo $3
                        
                        out.str("");
                        out << "mflo $"<< regNum;
                        output.push_back(out.str());
                        out.str("");
                        return output;
                    }
                    else if (node->childrens[0]->type == "int*"){
                        vector<string> output;
                        // code (expr2 )
                        output.push_back(";code (expr2 )");
                        for (auto &line: generateExprValueToReg(regNum, node->childrens[0])) {output.push_back(line);};
                        // push ($3)
                        output.push_back(";push ($3)");
                        for (auto &line: push(regNum)) {output.push_back(line);};
                        // code ( term )
                        output.push_back(";code ( term )"); 
                        for (auto &line: generateExprValueToReg(regNum, node->childrens[2])) {output.push_back(line);};
                        // mult $3 , $4 ; $4 always has the value 4

                        out.str("");
                        out << "mult $"<< regNum <<" , $" << 4;
                        output.push_back(out.str());
                        out.str(""); 
                        // mflo $3
                        
                        out.str("");
                        out << "mflo $"<< regNum;
                        output.push_back(out.str());
                        out.str(""); 
                        
                        // pop ($5) ; $5 <- expr2
                        output.push_back(";pop ($5) ; $5 <- expr2 ");  
                        int tempReg = getUnUsedReg();
                        if (tempReg == regNum) { tempReg += 1;}
                        for (auto &line: pop(tempReg)) {output.push_back(line);};
                        // sub $3 , $5 , $3
                        out.str("");
                        out << "sub $"<< regNum << ", $" << tempReg << ", $" << regNum;
                        output.push_back(out.str());
                        out.str(""); 
                        return output;
                    }
                    else {
                        out << "sub $"<< regNum <<" , $" << tempReg <<" , $" << regNum; 
                        output.push_back(out.str());
                    }
  
                }
                else if (values[1] == "STAR"){
                    out << "mult $"<< regNum <<" , $" << tempReg;
                    output.push_back(out.str());
                    stringstream out2;
                    out2 << "mflo $" << regNum;
                    output.push_back(out2.str());
                }
                else if (values[1] == "SLASH"){
                    out << "div $"<< tempReg <<" , $" << regNum;
                    output.push_back(out.str());
                    stringstream out2;
                    out2 << "mflo $" << regNum;
                    output.push_back(out2.str());
                }
                else if (values[1] == "PCT"){
                    out << "div $"<< tempReg <<" , $" << regNum;
                    output.push_back(out.str());
                    stringstream out2;
                    out2 << "mfhi $" << regNum;
                    output.push_back(out2.str());
                }
                else if (values[1] == "LT"){
                    out << compMipsFunction << " $"<< regNum <<" , $" << tempReg <<" , $" << regNum; 
                    output.push_back(out.str()); 
                }
                else if (values[1] == "GT"){
                    out << compMipsFunction << " $"<< regNum <<" , $" << regNum <<" , $" << tempReg; 
                    output.push_back(out.str());
                }
                else if (values[1] == "LE"){
                    out << compMipsFunction << " $"<< regNum <<" , $" << regNum <<" , $" << tempReg;
                    output.push_back(out.str()); 
                    out.str("");
                    out << "sub $"<< regNum << ", $21, $" << regNum;
                    output.push_back(out.str()); 
                    out.str(""); 
                } else if (values[1] == "GE"){
                    out << compMipsFunction << " $"<< regNum <<" , $" << tempReg <<" , $" << regNum;
                    output.push_back(out.str()); 
                    out.str("");
                    out << "sub $"<< regNum << ", $21, $" << regNum;
                    output.push_back(out.str()); 
                    out.str(""); 
                } else if (values[1] == "EQ") {
                    int tempVar1 = 8;
                    int tempVar2 = 9;
                    out << compMipsFunction << " $"<< tempVar1 <<" , $" << tempReg <<" , $" << regNum;
                    output.push_back(out.str()); 
                    out.str("");

                    out << compMipsFunction << " $"<< tempVar2 <<" , $" << regNum <<" , $" << tempReg;
                    output.push_back(out.str()); 
                    out.str("");

                    out << "add $"<< regNum <<" , $" << tempVar1 <<" , $" << tempVar2;
                    output.push_back(out.str()); 
                    out.str("");

                    out << "sub $"<< regNum << ", $21, $" << regNum;
                    output.push_back(out.str()); 
                    out.str("");
                } else if (values[1] == "NE") {
                    int tempVar1 = 8;
                    int tempVar2 = 9;
                    out << compMipsFunction << " $"<< tempVar1 <<" , $" << tempReg <<" , $" << regNum;
                    output.push_back(out.str()); 
                    out.str("");

                    out << compMipsFunction << " $"<< tempVar2 <<" , $" << regNum <<" , $" << tempReg;
                    output.push_back(out.str()); 
                    out.str("");

                    out << "add $"<< regNum <<" , $" << tempVar1 <<" , $" << tempVar2;
                    output.push_back(out.str()); 
                    out.str("");
                }

                return output;
            }
            else if (node->kind == "NUM") {
                return lis(regNum, node->value);
            } 
            else if (node->kind == "NULL") {
                return lis(regNum, "1");
            } 
            else if (node->kind == "ID"){
                return code(regNum, node->value);
            }
            else if (node->value == "LPAREN expr RPAREN") {
                return generateExprValueToReg(regNum, node->childrens[1]);
            }
            else if (node->value == "GETCHAR LPAREN RPAREN"){
                stringstream out;
                int regNum2 = 5;
                for (auto &line : lis(regNum2, "0xffff0004")){ 
                    output.push_back(line);
                }
                out <<"lw $"<< regNum << " , 0($"<< regNum2 << ")"; //<< "E";
                output.push_back(out.str());
                out.str("");
                return output;
            }
            else if (node->value == "ID LPAREN arglist RPAREN") {
                auto procName = node->childrens[0]->value;
                for (auto &line : push(29)) {output.push_back(line);};
                for (auto &line : push(31)) {output.push_back(line);};
                auto [numArgs, codes]= pushArgsOnStack(node->childrens[2]);
                numArgForProcedure[currProcedure] = numArgs;
                for (auto &line : codes) {output.push_back(line);};
                for (auto &line : lis(20, "P" + procName)) {output.push_back(line);}; 
                output.push_back("jalr $20");
                for (int i = 0; i < numArgs; i++) {output.push_back("add $30, $30, $4");};
                for (auto &line : pop(31)) {output.push_back(line);};
                for (auto &line : pop(29)) {output.push_back(line);};
                return output;
            } 
            else if (node->value == "ID LPAREN RPAREN") {
                auto procName = node->childrens[0]->value;
                for (auto &line : push(29)) {output.push_back(line);};
                for (auto &line : push(31)) {output.push_back(line);};
                for (auto &line : lis(20, "P" + procName)) {output.push_back(line);}; 
                output.push_back("jalr $20");
                for (auto &line : pop(31)) {output.push_back(line);};
                for (auto &line : pop(29)) {output.push_back(line);};
                return output;
            }
            else if ((node->value == "AMP lvalue" && node->childrens[1]->value == "STAR factor") 
                    || (node->value == "STAR factor" && node->childrens[1]->value == "AMP lvalue")){
                return generateExprValueToReg(regNum, node->childrens[1]->childrens[1]); 
            }
            else if (node->kind == "factor" && node->value == "STAR factor"){
                for (auto &line : generateExprValueToReg(regNum, node->childrens[1])) {output.push_back(line);};
                stringstream out;
                out << "lw $" << regNum << ", " << 0 << "($"<< regNum <<")"; // << "D";
                output.push_back(out.str());
                // out.str("");
                // out << "lw $" << regNum << ", " << 0 << "($"<< regNum <<")"; // << "D";
                // output.push_back(out.str());
                return output;
            }
            else if (node->kind == "lvalue") {
                if (node->value == "ID") {
                    auto varName = node->childrens[0]->value;
                    stringstream output;
                    output << "lw $" << regNum << ", " << varsInfo[currProcedure][varName] << "($29)";
                    return {output.str()};
                } else if (node->value == "STAR factor") {
                    for (auto &line : generateExprValueToReg(regNum, node->childrens[1])) {output.push_back(line);};
                    stringstream out;
                    // out << "lw $" << regNum << ", 0($" << regNum << ")"; //<< "C";
                    // output.push_back(out.str());
                    return output;
                } else { // LPAREN lvalue RPAREN
                    return generateExprValueToReg(regNum, node->childrens[1]); 
                }   
            }
            else if (node->value == "lvalue BECOMES expr SEMI"){
                auto reg1 = getUnUsedReg();
                auto reg2 = getUnUsedReg() + 1;
                for (auto &line : generateExprValueToReg(reg1, node->childrens[0])) {output.push_back(line);};
                for (auto &line : generateExprValueToReg(reg2, node->childrens[2])) {output.push_back(line);};
                stringstream out;
                out << "sw $" << reg2 << ", 0($" << reg1 << ")";
                output.push_back(out.str()); 
                return output;
            }
            else if (node->value == "AMP lvalue"){
                for(auto line: getAddressFromLvalue(regNum, node->childrens[1])) {output.push_back(line);};
                return output;
                // if (node->childrens[1]->childrens[0]->kind == "ID") {
                //     auto variableName = node->childrens[1]->childrens[0]->value;
                //     for(auto line: lis(3, to_string(varsInfo[currProcedure][variableName]))) {output.push_back(line);};
                //     output.push_back("add $3, $29, $3");
                //     return output;
                // } else {
                //     return generateExprValueToReg(regNum, node->childrens[1]->childrens[1]);
                // }
            }
            else if (node->childrens.size() == 1) {
                return generateExprValueToReg(regNum, node->childrens[0]);
            }
            else if (node->value == "NEW INT LBRACK expr RBRACK") {
                import.insert("new");
                for (auto &line : push(1)){ 
                    output.push_back(line);
                }
                for (auto &line : push(31)){ 
                    output.push_back(line);
                }  
                for (auto &line : generateExprValueToReg(1, node->childrens[3])){ 
                    output.push_back(line);
                }
                for (auto &line : lis(7, "new")){ 
                    output.push_back(line);
                }

                output.push_back("jalr $7");

                for (auto &line : lis(7, "1")){ 
                    output.push_back(line);
                }

                output.push_back("bne $3, $0, 1");
                output.push_back("add $3, $0, $7");

                
                for (auto &line : pop(31)){ 
                    output.push_back(line);
                }
                for (auto &line : pop(1)){ 
                    output.push_back(line);
                }
                stringstream out;
                out << "add $"<< regNum << ", $3, $0";
                output.push_back(out.str());
                return output;
            }
            else {
                cerr << "This type is not supported in generateExprValueToReg " << node->kind << " " << node->value << endl;
                return {};
            }
            // cout << "THIS CASE IS NOT HANDLED " << node->kind << " " << node->value << endl;
            return {};
        };
        vector<string> generateStatement(shared_ptr<Node> node){  
            assert(node->kind == "statement" && "this fn expects paramNode->kind to be 'statement'");
            vector<string>output;
            int unUsedReg = getUnUsedReg();
            int addressReg = 5;
            stringstream store;

            if (node->value == "lvalue BECOMES expr SEMI"){
                for (auto &line : getAddressFromLvalue(addressReg, node->childrens[0])){ 
                    output.push_back(line);
                }
                for (auto &line : generateExprValueToReg(unUsedReg, node->childrens[2])){ 
                    output.push_back(line);
                }
    
                // lis(unUsedReg, value);
                store << "sw $" << unUsedReg << ", 0($" <<  addressReg << ")" << "; store expr to lvalue " ;
                output.push_back(store.str());
                return output;
            }
            else if (node->value == "IF LPAREN test RPAREN LBRACE statements RBRACE ELSE LBRACE statements RBRACE"){
                store << "beginIf" << currLabelCounter;
                auto beginIfLabel = store.str();
                store.str(""); 
                store << "beginElse" << currLabelCounter;
                auto beginElseLabel = store.str();
                store.str(""); 
                store << "endIf" << currLabelCounter;
                auto endIfLabel = store.str();
                store.str("");
                currLabelCounter += 1;

                output.push_back(beginIfLabel + ":");
                for (auto &line : generateExprValueToReg(unUsedReg, node->childrens[2])){ output.push_back(line);};

                store << "beq $" << unUsedReg << " , $0, " << beginElseLabel;
                output.push_back(store.str());
                store.str("");

                for (auto &line : generateStatements(node->childrens[5])){ output.push_back(line);};
                store << "beq $0, $0, " << endIfLabel;
                output.push_back(store.str());
                store.str(""); 

                output.push_back(beginElseLabel + ":");
                for (auto &line : generateStatements(node->childrens[9])){ output.push_back(line);}; 
                output.push_back(endIfLabel + ":");

                return output; 
            }
            else if (node->value == "WHILE LPAREN test RPAREN LBRACE statements RBRACE"){
                store << "while" << currLabelCounter;
                auto loopStartLabel = store.str();
                store.str("");
                store << "endwhile" << currLabelCounter;
                auto loopEndLabel = store.str();
                currLabelCounter += 1;
                output.push_back(loopStartLabel + ":");
                for (auto &line : generateExprValueToReg(unUsedReg, node->childrens[2])){ output.push_back(line);};
                store.str("");
                store << "beq $" << unUsedReg << ",$0, " << loopEndLabel;
                output.push_back(store.str());
                
                for (auto &line : generateStatements(node->childrens[5])){ output.push_back(line);};
                for (auto &line : generateExprValueToReg(unUsedReg, node->childrens[2])){ output.push_back(line);};
                
                store.str("");
                store << "bne $" << unUsedReg << ",$0, " << loopStartLabel;
                output.push_back(store.str());
                output.push_back(loopEndLabel + ":");
            

                return output;
            }
            else if (node->value == "PUTCHAR LPAREN expr RPAREN SEMI"){
                int unUsedReg2 = 5;
                for (auto &line : generateExprValueToReg(unUsedReg, node->childrens[2])){ 
                    output.push_back(line);
                }
                for (auto &line : lis(unUsedReg2, "0xffff000c")){ 
                    output.push_back(line);
                }
                store << "sw $" << unUsedReg <<" , 0($" << unUsedReg2 << ")";
                output.push_back(store.str());
                store.str("");
                return output;
            }
            else if (node->value == "PRINTLN LPAREN expr RPAREN SEMI"){
                import.insert("print");
                for (auto &line : push(1)){ 
                    output.push_back(line);
                }
                for (auto &line : generateExprValueToReg(3, node->childrens[2])){ 
                    output.push_back(line);
                }
                output.push_back("add $1 , $3 , $0");
                for (auto &line : push(31)){ 
                    output.push_back(line);
                }
                for (auto &line : lis(5, "print")){ 
                    output.push_back(line);
                }
                output.push_back("jalr $5");
                for (auto &line : pop(31)){ 
                    output.push_back(line);
                }
                for (auto &line : pop(1)){ 
                    output.push_back(line);
                }
                return output;
            }
            else if (node->value == "DELETE LBRACK RBRACK expr SEMI") {
                import.insert("delete");
                import.insert("alloc");
                for (auto &line : push(1)){ 
                    output.push_back(line);
                }
                for (auto &line : push(31)){ 
                    output.push_back(line);
                }  
                for (auto &line : getAddressFromLvalue(1, node->childrens[3])){ 
                    output.push_back(line);
                }

                for (auto &line : lis(26, "1")){ 
                    output.push_back(line);
                }

                for (auto &line : lis(7, "delete")){ 
                    output.push_back(line);
                }
                
                output.push_back("beq $1, $26, 1");
                output.push_back("jalr $7");
                
                for (auto &line : pop(31)){ 
                    output.push_back(line);
                }
                for (auto &line : pop(1)){ 
                    output.push_back(line);
                }
                return output;
            }
            else {
                cerr << "This type: " << node->kind << "->" << node->value << "is not supported" << endl;
            }
        }

        vector<string> generateStatements(shared_ptr<Node> node){
            assert(node->kind == "statements" && "this fn expects paramNode->kind to be 'statements'");
            if (node->childrens.size() <= 1) return {};
            vector<string> output;
            for (auto &line: generateStatements(node->childrens[0])) {
                output.push_back(line);
            }

            for (auto &line: generateStatement(node->childrens[1])) {
                output.push_back(line);
            } 

            return output;            
        };

        vector<string> generateReturn(shared_ptr<Node> node) {
            return generateExprValueToReg(3, node);
        }

        vector<string> generateCleanup() {
            vector<string> output;
            output.push_back("; begin epilogue");
            for (int i = 0; i < varsInfo[currProcedure].size() -1; ++i) {
                output.push_back("add $30 , $30 , $4");
            }
            
            return output;
        }
        // pop local variables     ; only non-parameters
        vector<string> generateCleanupForProc() {
            vector<string> output;
            output.push_back("; begin epilogue");
            for (auto &[k,v] : varsInfo[currProcedure]) {
                if (k != "currVarsOffset" && v <= 0){
                    output.push_back("add $30 , $30 , $4");
                }
            }

            return output;
        }

        vector<string> generate(shared_ptr<Node> node) {
            vector<string> mipsOutput = {};
            import.insert("init");

            if (node->kind == "main") {
                vector<string> heapSetUp;
                for (auto &line : push(2)){ 
                    heapSetUp.push_back(line);
                }
                for (auto &line : push(31)){ 
                    heapSetUp.push_back(line);
                };

                if (node->childrens[3]->type == "int"){
                    heapSetUp.push_back("add $2, $0, $0");
                }
                for (auto &line : lis(7, "init")){ 
                    heapSetUp.push_back(line);
                }
                heapSetUp.push_back("jalr $7");

                for (auto &line : pop(31)){ 
                    heapSetUp.push_back(line);
                }
                for (auto &line : pop(2)){ 
                    heapSetUp.push_back(line);
                }

                currProcedure = "main";
                varsInfo[currProcedure] = map<string,int>{};
                varsInfo[currProcedure]["currVarsOffset"] = 0;
                auto set4 = lis(4, "4");
                // generate params declarations
                auto param1Declarations = generateParamDcl(node->childrens[3], 1);
                auto param2Declarations = generateParamDcl(node->childrens[5], 2);
                // update params offset
                updateParamsOffset();
                
                auto set$29 = vector<string> {"sub $29 , $30 , $4"};
                // generate normal declarations
                auto normalDeclations = generateDcls(node->childrens[8]);
                normalDeclations.push_back("; end set up and declartions");
                normalDeclations.push_back("");

                // generate statments
                auto statements = generateStatements(node->childrens[9]);
                statements.push_back("; end statements");
                statements.push_back("");
                // return
                auto returns = generateReturn(node->childrens[11]);
                returns.push_back("; end return");
                normalDeclations.push_back("");
                // clean up
                auto cleanups = generateCleanup();
                
                vector<vector<string>> combinedOutputs = {
                    // std::move(heapSetUp),
                    std::move(set4),
                    std::move(param1Declarations),
                    std::move(param2Declarations),
                    std::move(set$29),
                    std::move(normalDeclations),
                    std::move(statements),
                    std::move(returns),
                    std::move(cleanups)
                };

                for (auto &v: combinedOutputs) {
                    for (auto &line: v) {
                        mainOutput.push_back(line);
                    }
                }

                mainOutput.push_back("jr $31");
                return {}; 
            }

            else if (node->kind == "procedure"){
                string currProcedureLabel = "P" + node->childrens[1]->value;
                currProcedure = currProcedureLabel;
                varsInfo[currProcedure] = map<string,int>{};
                varsInfo[currProcedure]["currVarsOffset"] = 0;
                // generate params declarations
                mapParamsToOffsetTable(node->childrens[3]);
                // update params offset
                updateParamsOffset();

                auto currProcedureLabelStart = vector<string> {currProcedureLabel + ":"};
                auto set$29 = vector<string> {"sub $29 , $30 , $4"};
                auto normalDeclations = generateDcls(node->childrens[6]);
                vector<string> preserveRegisters;
                auto registersToSave = vector<int> {getUnUsedReg(), getUnUsedReg() + 1};
                for (auto &reg: registersToSave) {
                    for (auto &line: push(reg)) {
                        preserveRegisters.push_back(line);
                    }
                }
                // generate statments
                auto statements = generateStatements(node->childrens[7]);
                // return
                auto returns = generateReturn(node->childrens[9]);
                // pop preserved registers
                vector<string> popPreservedRegisters;
                for (auto &reg: registersToSave) {
                    for (auto &line: pop(reg)) {
                        popPreservedRegisters.push_back(line);
                    }
                }
                // clean up
                auto cleanups = generateCleanupForProc();
                vector<vector<string>> combinedOutputs = {
                    std::move(currProcedureLabelStart),
                    std::move(set$29),
                    std::move(normalDeclations),
                    std::move(preserveRegisters),
                    std::move(statements),
                    std::move(returns),
                    std::move(popPreservedRegisters),
                    std::move(cleanups)
                };

                for (auto &v: combinedOutputs) {
                    for (auto &line: v) {
                        mipsOutput.push_back(line);
                    }
                }
                mipsOutput.push_back("jr $31");
            }

            for (auto &child : node->childrens) {
                auto childResult = generate(child);
                for (auto &line: childResult) {
                    mipsOutput.push_back(line);
                }
            }
            return mipsOutput;
        };

        vector<string> generateImport() {
            vector<string> mipsOutput = {};
            for (auto &importlib : import) {
                mipsOutput.push_back(".import " + importlib);
            }
            return mipsOutput;
        }
        vector<string> getMainOutput() {return mainOutput;}; 
};


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
    
    auto cg = CodeGenerator();

    auto codes = cg.generate(root);
    auto imports = cg.generateImport();
    auto mainOutput = cg.getMainOutput();


    for (auto &line : imports) {
        cout << line << endl;
    }
    for (auto &line : mainOutput) {
        cout << line << endl;
    }
    for (auto &line : codes) {
        cout << line << endl;
    }
}