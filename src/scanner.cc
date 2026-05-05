#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>

const std::string ALPHABET    = ".ALPHABET";
const std::string STATES      = ".STATES";
const std::string TRANSITIONS = ".TRANSITIONS";
const std::string INPUT       = ".INPUT";
const std::string EMPTY       = ".EMPTY";



std::ostream& operator<<(std::ostream& os, const std::map<std::string, std::map<char, std::string>>& stateMap) {
    for (const auto& outerPair : stateMap) {
        os << outerPair.first << ":\n";  // Print the key of the outer map
        for (const auto& innerPair : outerPair.second) {
            os << "  " << innerPair.first << " -> " << innerPair.second << "\n";  // Print key-value pairs of the inner map
        }
    }
    return os;
}

bool isChar(std::string s) {
  return s.length() == 1;
}
bool isRange(std::string s) {
  return s.length() == 3 && s[1] == '-';
}

std::map<std::string, std::map<char, std::string>> stateMap;
std::set <std::string> acceptingStates;
std::set <std::string> allStates;
std::string initialState;

std::pair<std::string, int> dfs(int currIndex, std::string &s, std::map<std::string, std::map<char, std::string>> &stateMap, std::set<std::string> &acceptingStates, std::string &currentState){
  if (currIndex >= s.length()) return {currentState, acceptingStates.count(currentState) == 1 ? currIndex : -1};
  
  if (stateMap.count(currentState) == 0 || stateMap[currentState].count(s[currIndex]) == 0) {
    return {currentState, acceptingStates.count(currentState) == 1 ? currIndex : -1};
  }

  std::string nextState = stateMap[currentState][s[currIndex]];
  return dfs(currIndex + 1, s, stateMap, acceptingStates, nextState);
}

std::vector<std::pair<std::string, std::string>> SimplifiedMaximalMunch(std::string &s, std::map<std::string, std::map<char, std::string>> &stateMap, std::set<std::string> &acceptingStates, std::string &currentState,std::map<std::string, std::string> &acceptingStatesTokenTypes) { 
  std::vector<std::pair<std::string, std::string>> tokens;
  if (s == EMPTY | s.length() == 0) return tokens;
  int currIndex = 0;
  while (currIndex < s.length()) {
    auto [lastState, nextIndex] = dfs(currIndex, s, stateMap, acceptingStates, currentState);
    if ((nextIndex) == -1) {
      std::cerr << "ERROR" << std::endl;
      return tokens;
    }

    tokens.push_back({s.substr(currIndex, nextIndex - currIndex),acceptingStatesTokenTypes[lastState]});
    currIndex = nextIndex;
  }


  return tokens;
}

// Locations in the program that you should modify to store the
// DFA information have been marked with four-slash comments:
//// (Four-slash comment)
int main() {

  std::map<std::string, std::string> acceptingStatesTokenTypes;
  acceptingStatesTokenTypes["LPAREN"] = "LPAREN";
  acceptingStatesTokenTypes["RPAREN"] = "RPAREN";
  acceptingStatesTokenTypes["LBRACE"] = "LBRACE";
  acceptingStatesTokenTypes["RBRACE"] = "RBRACE";
  acceptingStatesTokenTypes["LBRACK"] = "LBRACK";
  acceptingStatesTokenTypes["RBRACK"] = "RBRACK";
  acceptingStatesTokenTypes["PLUS"] = "PLUS";
  acceptingStatesTokenTypes["MINUS"] = "MINUS";
  acceptingStatesTokenTypes["STAR"] = "STAR";
  acceptingStatesTokenTypes["SLASH"] = "SLASH";
  acceptingStatesTokenTypes["PCT"] = "PCT";
  acceptingStatesTokenTypes["COMMA"] = "COMMA";
  acceptingStatesTokenTypes["SEMI"] = "SEMI";
  acceptingStatesTokenTypes["AMP"] = "AMP";
  acceptingStatesTokenTypes["BECOMES"] = "BECOMES";
  acceptingStatesTokenTypes["EQ"] = "EQ";
  acceptingStatesTokenTypes["NE"] = "NE";
  acceptingStatesTokenTypes["LT"] = "LT";
  acceptingStatesTokenTypes["GT"] = "GT";
  acceptingStatesTokenTypes["LE"] = "LE";
  acceptingStatesTokenTypes["GE"] = "GE";
  acceptingStatesTokenTypes["ZERO"] = "NUM";
  acceptingStatesTokenTypes["NUM"] = "NUM";
  acceptingStatesTokenTypes["RETURN"] = "RETURN";
  acceptingStatesTokenTypes["ID"] = "ID";
  acceptingStatesTokenTypes["ELSE"] = "ELSE";
  acceptingStatesTokenTypes["GETCHAR"] = "GETCHAR";
  acceptingStatesTokenTypes["NEW"] = "NEW";
  acceptingStatesTokenTypes["DELETE"] = "DELETE";
  acceptingStatesTokenTypes["NULL"] = "NULL";
  acceptingStatesTokenTypes["INT"] = "INT";
  acceptingStatesTokenTypes["IF"] = "IF";
  acceptingStatesTokenTypes["PRINTLN"] = "PRINTLN";
  acceptingStatesTokenTypes["PUTCHAR"] = "PUTCHAR";
  acceptingStatesTokenTypes["WAIN"] = "WAIN";
  acceptingStatesTokenTypes["WHILE"] = "WHILE";
  acceptingStatesTokenTypes["INT_IF_1"] = "ID";

  std::istringstream in(R"(
.ALPHABET
a-z
0-9
A-Z
$
, ( ) { } [ ]
+ - * / % = ! < > & ;
.
.STATES
start
LPAREN!
RPAREN!
LBRACE!
RBRACE!
LBRACK!
RBRACK!
PLUS!
MINUS!
STAR!
SLASH!
PCT!
COMMA!
SEMI!
AMP!
BECOMES!
EQ!
NE!
NE_1
LT!
GT!
LE!
GE!
ZERO!
NUM!
RETURN1
RETURN2
RETURN3
RETURN4
RETURN5
RETURN!
ID!
ELSE1
ELSE2
ELSE3
ELSE!
GETCHAR1
GETCHAR2
GETCHAR3
GETCHAR4
GETCHAR5
GETCHAR6
GETCHAR!
NEW1
NEW2
NEW!
DELETE1
DELETE2
DELETE3
DELETE4
DELETE5
DELETE!
NULL1
NULL2
NULL3
NULL!
INT!
IF!
INT_IF_1!
INT2
PRINTLN_PUTCHAR1
PRINTLN2
PRINTLN3
PRINTLN4
PRINTLN5
PRINTLN6
PRINTLN!
PUTCHAR2
PUTCHAR3
PUTCHAR4
PUTCHAR5
PUTCHAR6
PUTCHAR!
WAIN_WHILE_1
WAIN2
WAIN3
WAIN!
WHILE2
WHILE3
WHILE4
WHILE!
.TRANSITIONS
start ( LPAREN
start ) RPAREN
start { LBRACE
start } RBRACE
start [ LBRACK
start ] RBRACK
start + PLUS
start - MINUS
start * STAR
start / SLASH
start % PCT
start , COMMA
start ; SEMI
start & AMP
start = BECOMES
BECOMES = EQ
start ! NE_1
NE_1 = NE
start < LT
LT = LE
start > GT
GT = GE
start 0 ZERO
start 1-9 NUM
NUM 0-9 NUM
start r RETURN1
RETURN1 e RETURN2
RETURN1 a-d ID
RETURN1 f-z ID
RETURN1 A-Z ID
RETURN2 t RETURN3
RETURN2 a-s ID
RETURN2 u-z ID
RETURN2 A-Z ID
RETURN3 u RETURN4
RETURN3 a-t ID
RETURN3 v-z ID
RETURN3 A-Z ID
RETURN4 r RETURN5
RETURN4 a-q ID
RETURN4 s-z ID
RETURN4 A-Z ID
RETURN5 n RETURN
RETURN5 a-m ID
RETURN5 o-z ID
RETURN5 A-Z ID
RETURN a-z A-Z 0-9 ID
start a ID
start b ID
start c ID
start f ID
start g ID
start h ID
start j ID
start k ID
start l ID
start m ID
start n ID
start o ID
start q ID
start s ID
start t ID
start u ID
start v ID
start x ID
start y ID
start z ID
start A-M ID
start O-Z ID
ID a-z A-Z 0-9 ID
start e ELSE1
ELSE1 l ELSE2
ELSE1 a-k ID
ELSE1 m-z ID
ELSE1 A-Z ID
ELSE2 s ELSE3
ELSE2 a-r ID
ELSE2 t-z ID
ELSE2 A-Z ID
ELSE3 e ELSE
ELSE3 a-d ID
ELSE3 f-z ID
ELSE3 A-Z ID
ELSE a-z A-Z 0-9 ID 
start g GETCHAR1
GETCHAR1 e GETCHAR2
GETCHAR1 a-d ID
GETCHAR1 f-z ID
GETCHAR1 A-Z ID
GETCHAR2 t GETCHAR3
GETCHAR2 a-s ID
GETCHAR2 u-z ID
GETCHAR2 A-Z ID
GETCHAR3 c GETCHAR4
GETCHAR3 a-b ID
GETCHAR3 d-z ID
GETCHAR3 A-Z ID
GETCHAR4 h GETCHAR5
GETCHAR4 a-g ID
GETCHAR4 i-z ID
GETCHAR4 A-Z ID
GETCHAR5 a GETCHAR6
GETCHAR5 b-z ID
GETCHAR5 A-Z ID
GETCHAR6 r GETCHAR
GETCHAR6 a-q ID
GETCHAR6 s-z ID
GETCHAR6 A-Z ID
GETCHAR a-z A-Z 0-9 ID 
start n NEW1
NEW1 e NEW2
NEW1 a-d ID
NEW1 f-z ID
NEW1 A-Z ID
NEW2 w NEW
NEW2 a-v ID
NEW2 x-z ID
NEW2 A-Z ID
NEW a-z A-Z 0-9 ID 
start d DELETE1
DELETE1 e DELETE2
DELETE1 a-d ID
DELETE1 f-z ID
DELETE1 A-Z ID
DELETE2 l DELETE3
DELETE2 a-k ID
DELETE2 m-z ID
DELETE2 A-Z ID
DELETE3 e DELETE4
DELETE3 a-d ID
DELETE3 f-z ID
DELETE3 A-Z ID
DELETE4 t DELETE5
DELETE4 a-s ID
DELETE4 u-z ID
DELETE4 A-Z ID
DELETE5 e DELETE
DELETE5 a-d ID
DELETE5 f-z ID
DELETE5 A-Z ID
DELETE a-z A-Z 0-9 ID
start N NULL1
start A-M ID
start O-Z ID
NULL1 U NULL2
NULL1 A-T ID
NULL1 V-Z ID
NULL1 a-z ID
NULL2 L NULL3
NULL2 A-K ID
NULL2 M-Z ID
NULL2 a-z ID
NULL3 L NULL
NULL3 A-K ID
NULL3 M-Z ID
NULL3 a-z ID
NULL a-z A-Z 0-9 ID
start i INT_IF_1
INT_IF_1 f IF
IF a-z A-Z 0-9 ID
INT_IF_1 n INT2
INT_IF_1 a-e ID
INT_IF_1 g-m ID
INT_IF_1 o-z ID
INT_IF_1 A-Z ID
INT2 t INT
INT2 a-s ID
INT2 u-z ID
INT2 A-Z ID
INT a-z A-Z 0-9 ID
start p PRINTLN_PUTCHAR1
PRINTLN_PUTCHAR1 r PRINTLN2
PRINTLN2 i PRINTLN3
PRINTLN3 n PRINTLN4
PRINTLN4 t PRINTLN5
PRINTLN5 l PRINTLN6
PRINTLN6 n PRINTLN
PRINTLN a-z A-Z 0-9 ID
PRINTLN_PUTCHAR1 u PUTCHAR2
PUTCHAR2 t PUTCHAR3
PUTCHAR3 c PUTCHAR4
PUTCHAR4 h PUTCHAR5
PUTCHAR5 a PUTCHAR6
PUTCHAR6 r PUTCHAR
PUTCHAR a-z A-Z 0-9 ID
start w WAIN_WHILE_1
WAIN_WHILE_1 a WAIN2
WAIN2 i WAIN3
WAIN3 n WAIN
WAIN a-z A-Z 0-9 ID
WAIN_WHILE_1 h WHILE2
WHILE2 i WHILE3
WHILE3 l WHILE4
WHILE4 e WHILE
WHILE a-z A-Z 0-9 ID
.INPUT
    )");
  std::string s;

  std::getline(in, s); // Alphabet section (skip header)
  // Read characters or ranges separated by whitespace
  while(in >> s) {
    if (s == STATES) { 
      break; 
    } else {
      if (isChar(s)) {
        // //// Variable 's[0]' is an alphabet symbol
      } else if (isRange(s)) {
        for(char c = s[0]; c <= s[2]; ++c) {
          //// Variable 'c' is an alphabet symbol
        }
      } 
    }
  }
  
  std::getline(in, s); // States section (skip header)
  // Read states separated by whitespace
  while(in >> s) {
    if (s == TRANSITIONS) { 
      break; 
    } else {
      static bool initial = true;
      bool accepting = false;
      if (s.back() == '!' && !isChar(s)) {
        accepting = true;
        s.pop_back();
      }
      //// Variable 's' contains the name of a state
      if (initial) {
        //// The state is initial
        initial = false;
        initialState = s;
      }
      if (accepting) {
        //// The state is accepting
        acceptingStates.insert(s);
      }
      allStates.insert(s);
    }
  };


  for (auto &state: allStates) {
    stateMap[state] = std::map<char, std::string> {};
  }


  std::getline(in, s); // Transitions section (skip header)
  // Read transitions line-by-line
  while(std::getline(in, s)) {
    if (s == INPUT) { 
      // Note: Since we're reading line by line, once we encounter the
      // input header, we will already be on the line after the header
      break; 
    } else {
      std::string fromState, symbols, toState;
      std::istringstream line(s);
      std::vector<std::string> lineVec;
      while(line >> s) {
        lineVec.push_back(s);
      }
      fromState = lineVec.front();
      toState = lineVec.back();
      for(int i = 1; i < lineVec.size()-1; ++i) {
        std::string s = lineVec[i];
        if (isChar(s)) {
          symbols += s;
        } else if (isRange(s)) {
          for(char c = s[0]; c <= s[2]; ++c) {
            symbols += c;
          }
        }
      }
      // std::cout << "good " << fromState << " " << toState << std::endl;

      for ( char c : symbols ) {
        //// There is a transition from 'fromState' to 'toState' on 'c'
        stateMap[fromState][c] = toState;
      }
    }
  }


  std::string currLine;


  while (std::getline(std::cin, currLine)) {
    size_t commentIndex = currLine.find("//");
    if (commentIndex != std::string::npos) {
      currLine.erase(commentIndex);
    }

    std::istringstream iss(currLine);

    while(iss >> s) {
      auto tokens = SimplifiedMaximalMunch(s, stateMap, acceptingStates, initialState, acceptingStatesTokenTypes);
      for (auto &[token, type] : tokens) {
        if (type == "NUM") {
          try {
            std::size_t pos{};
            const int i{std::stoi(token, &pos)};
             std::cout << type << " " << token << std::endl;
          } catch (std::out_of_range const& ex){
            std::cerr << "ERROR " << "out of range" << std::endl;
          }
        } else {
          std::cout << type << " " << token << std::endl;
        }
      }
    }
  }

  return 0; 
}