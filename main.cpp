// Include Libraries
#include <algorithm>
using std::all_of;
#include <fstream>
using std::ofstream;
using std::ifstream;
#include <iostream>
using std::cerr;
#include <string>
using std::string;
#include <vector>
using std::vector;
#include <assert.h>
#include <iomanip>
using std::setw;
using std::right;
using std::left;
using std::cout;
using std::endl;
using std::cin;

enum State {
	STARTING_STATE,				// 0
	IN_IDENTIFIER,				// 1
	END_OF_IDENTIFIER,			// 2 final state
	IN_NUMBER,					// 3
	IN_FLOAT,					// 4
	END_OF_NUMBER,				// 5 final state
	IN_COMMENT,					// 6
	END_OF_COMMENT,				// 7 final state
	SYMBOLS						// 8 final state
};

enum Input {
	LETTER,			// 0
	DIGIT,			// 1
	SPACE,			// 2
	EXCLAMATION,	// 3
	DOLLAR_SIGN,	// 4
	PERIOD,			// 5
	OTHER,			// 6
	BACKUP			// 7 not an input, but a flag that tells the lexer when to back up 
};

class StateMachine {

private:

	const int NUM_OF_STATES = 9;
	const int NUM_OF_INPUTS = 8;
	const int NUM_OF_FINAL_STATES = 4;
	const int final_states[4] = { END_OF_IDENTIFIER, END_OF_NUMBER, END_OF_COMMENT, SYMBOLS };
	int state_transition_table[9][8] =
	{
		{ 1, 3, 0, 6, 8, 4, 8, 0 },
		{ 1, 1, 2, 2, 1, 2, 2, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 1 },
		{ 5, 3, 5, 5, 5, 4, 5, 0 },
		{ 5, 4, 5, 5, 5, 5, 5, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 1 },
		{ 6, 6, 6, 7, 6, 6, 6, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	};

public:

	int check_input(int state, int input) {
		return state_transition_table[state][input];
	}

	bool should_back_up(int curr_state) {
		if (state_transition_table[curr_state][BACKUP] == 1)
			return 1;
		else
			return 0;
	}

	string getTokenName(int state, string lexeme) {

		if (state == END_OF_IDENTIFIER) {
			if (lexeme == "int" || lexeme == "float" || lexeme == "bool" || lexeme == "if" || lexeme == "else" ||
				lexeme == "then" || lexeme == "do" || lexeme == "while" || lexeme == "whileend" || lexeme == "do" ||
				lexeme == "doend" || lexeme == "for" || lexeme == "and" || lexeme == "or" || lexeme == "function") {
				return "KEYWORD";
			}
			else {
				return "IDENTIFIER";
			}
		}

		else if (state == END_OF_NUMBER) {
			return "NUMBER";
		}

		else if (state == END_OF_COMMENT) {
			return "COMMENT";
		}

		else if (state == SYMBOLS) {
			if (lexeme == "*" || lexeme == "+" || lexeme == "-" || lexeme == "=" || lexeme == "/" ||
				lexeme == ">" || lexeme == "<") {
				return "OPERATOR";
			}
			else if (lexeme == "'" || lexeme == "(" || lexeme == ")" || lexeme == "{" || lexeme == "}" ||
				lexeme == "[" || lexeme == "]" || lexeme == "," || lexeme == "." || lexeme == ":" || lexeme == ";" ||
				lexeme == "!") {
				return "SEPARATOR";
			}
			else
				return "OTHER";
		}
		else
			return "ERROR";
	}

	int char_to_input(char code) {
		if (isalpha(code))
			return LETTER;
		else if (isdigit(code))
			return DIGIT;
		else if (isspace(code))
			return SPACE;
		else if (code == '!')
			return EXCLAMATION;
		else if (code == '$')
			return DOLLAR_SIGN;
		else if (code == '.')
			return PERIOD;
		else
			return OTHER;
	}

	bool is_final_state(int curr_state) {
		for (int i = 0; i < NUM_OF_FINAL_STATES; i++) {
			if (curr_state == final_states[i])
				return 1;
		}
		return 0;
	}

};

struct tokens {
	string token;
	string lexeme;
	tokens(string tok, string lex) {
		token = tok, lexeme = lex;
	}
};

bool analyze_syntax(vector<tokens>&, ofstream&);
int string_to_index(string);
void print_rule(string, string, ofstream&);

bool analyze_syntax(vector<tokens>& token_vect, ofstream& output_file) {

	vector<vector<string>> predictive_table = {
	//			i    =     +     -      *     /      (       )      $ 
	/*S*/	{ "i=E", "", ""   , ""   , ""   , ""   , ""   , ""   , "" },
	/*E*/	{ "TQ" , "", ""   , ""   , ""   , ""   , "TQ" , ""   , "" },
	/*Q*/	{ ""   , "", "+TQ", "-TQ", ""   , ""   , ""   , "e"  , "e" },
	/*R*/	{ "FR" , "", ""   , ""   , ""   , ""   , "FR" , ""   , "" },
	/*T*/	{ ""   , "", "e"  , "e"  , "*FR", "/FR", ""   , "e"  , "e" },
	/*F*/	{ "i"  , "", ""   , ""   , ""   , ""   , "(E)", ""   , "" }
	};
	vector<string> stack;
	vector<tokens> string_line;

	for (vector<tokens>::iterator it = token_vect.begin();  it != token_vect.end(); it++)
	{
		if (it->lexeme != ";")
		{
			string_line.push_back(tokens(it->token, it->lexeme));
		}

		else 
		{
			stack.push_back("$");
			stack.push_back("S");

			string_line.push_back(tokens("$", "$"));

			vector<tokens>::iterator current = string_line.begin();
			string top_of_stack;
			string token;

			output_file << "------------------------------------------" << endl;
			output_file << "Token: " << left << setw(25) << current->token <<
				"Lexeme: " << current->lexeme << endl;

			while (!stack.empty())
			{
				top_of_stack = stack.back();
				if (current->token == "IDENTIFIER") { token = "i"; }
				else { token = current->lexeme; }

				if (top_of_stack == "i" || top_of_stack == "=" || top_of_stack == "+" ||
					top_of_stack == "-" || top_of_stack == "*" || top_of_stack == "/" ||
					top_of_stack == "(" || top_of_stack == ")" || top_of_stack == "$")
				{
					if (top_of_stack == token)
					{
						stack.pop_back();
						current++;

						if ((!stack.empty()) && current->lexeme != "$")
						{
							output_file << endl << "------------------------------------------" << endl;
							output_file << "Token: " << left << setw(25) << current->token <<
								"Lexeme: " << current->lexeme << endl;
						}
					}

					else
					{
						return false;
					}
				}

				else
				{

					assert(string_to_index(top_of_stack) != -1);
					assert(string_to_index(token) != -1);
					string prod_rule = predictive_table[string_to_index(top_of_stack)][string_to_index(token)];

					if (!prod_rule.empty())
					{
						print_rule(top_of_stack, prod_rule, output_file);

						stack.pop_back();
						while (!prod_rule.empty())
						{
							if (prod_rule != "e") {	stack.push_back(string(1, prod_rule.back())); }
							prod_rule.pop_back();
						}
					}

					else
					{
						return false;
					}
				}
			}

			output_file << endl << "------------------------------------------" << endl;
			output_file << "Token: " << left << setw(25) << it->token <<
				"Lexeme: " << it->lexeme << endl;
			output_file << "<Empty> -> <Epsilon>" << endl << endl;
			
			string_line.clear();
		}
	}

	return true;
}


int string_to_index(string word)
{
	if (word == "S" || word == "i") { return 0; }
	else if (word == "E" || word == "=") { return 1; }
	else if (word == "Q" || word == "+") { return 2; }
	else if (word == "T" || word == "-") { return 3; }
	else if (word == "R" || word == "*") { return 4; }
	else if (word == "F" || word == "/") { return 5; }
	else if (word == "(") { return 6; }
	else if (word == ")") { return 7; }
	else if (word == "$") { return 8; }
	else return -1;
}

void print_rule(string statement, string prod_rule, ofstream& output_file)
{
	if (statement == "S")
	{
		if (prod_rule == "i=E")
		{
			output_file << "<Statement> -> Identifier = <Expression>" << endl;
		}
	}

	else if (statement == "E")
	{
		if (prod_rule == "TQ")
		{
			output_file << "<Expression> -> <Term> <Expression Prime>" << endl;
		}
	}

	else if (statement == "Q")
	{
		if (prod_rule == "+TQ")
		{
			output_file << "<Expression Prime> -> + <Term> <Expression Prime>" << endl;
		}
		else if (prod_rule == "-TQ")
		{
			output_file << "<Expression Prime> -> - <Term> <Expression Prime>" << endl;
		}
		if (prod_rule == "e")
		{
			output_file << "<Expression Prime> -> <Epsilon>" << endl;
		}
	}

	else if (statement == "T")
	{
		if (prod_rule == "FR")
		{
			output_file << "<Term> -> <Factor> <Term Prime>" << endl;
		}
	}

	else if (statement == "R")
	{
		if (prod_rule == "*FR")
		{
			output_file << "<Term Prime> -> * <Factor> <Term Prime>" << endl;
		}
		else if (prod_rule == "/FR")
		{
			output_file << "<Term Prime> -> / <Factor> <Term Prime>" << endl;
		}
		else if (prod_rule == "e")
		{
			output_file << "<Term Prime> -> <Epsilon>" << endl;
		}
	}

	else if (statement == "F")
	{
		if (prod_rule == "i")
		{
			//cout << "<Factor> -> Identifier" << endl;
			output_file << "<Factor> -> Identifier" << endl;
		}
		else if (prod_rule == "(E)")
		{
			//cout << "<Factor> -> ( <Expression> )" << endl;
			output_file << "<Factor> -> ( <Expression> )" << endl;
		}
	}
}

int main()
{
	string line;
	vector<string> codeArray;
	ifstream sourceCode; 
	string file_name;

	sourceCode.open("./input.txt");
	if (sourceCode.is_open()) {
		while (getline(sourceCode, line)) {
			codeArray.push_back(line);
		}
	}	 
	else {
		cerr << "Could not open " << file_name << endl;
		return -1;
	}

	sourceCode.close();

	vector<tokens> token_lexeme;
	StateMachine FSM;
	int curr_state = 0;
	int lexeme_start = 0;

	for (int line = 0; line < codeArray.size(); line++) {
		for (int char_ = 0; char_ <= codeArray[line].length(); char_++) {

			if (curr_state == 0) {
				lexeme_start = char_;
			}

			int curr_input = FSM.char_to_input(codeArray[line][char_]);

			curr_state = FSM.check_input(curr_state, curr_input);

			if (FSM.is_final_state(curr_state)) {
				
				if (FSM.should_back_up(curr_state)) {
					char_--;
				}

				if (curr_state != 7) {
					string lex = "";
					for (int i = lexeme_start; i <= char_; i++) {
						lex += codeArray[line][i];
					}

					if (FSM.getTokenName(curr_state, lex) != "OTHER") {
						token_lexeme.push_back(tokens(FSM.getTokenName(curr_state, lex), lex));
					}
				}				

				curr_state = 0;
				}
			}
		}

	ofstream out("output.txt");

	if (!analyze_syntax(token_lexeme, out)) {
		cout << "Syntax error" << endl;
		out << "ERROR: syntax error found in the source code" << endl;
	}

	out.close();

	cout << "File has been generated." << endl;

	system ("notepad output.txt");

	return 0;

}