#include "MetaLexer.h"

#include "../LibIO.h"

namespace CoreLib
{
namespace Text
{
	MetaLexer::MetaLexer()
	{
	}

	MetaLexer::MetaLexer(String profile)
	{
		SetLexProfile(profile);
	}

	String MetaLexer::GetTokenName(int id)
	{
		return TokenNames[id];
	}

	int MetaLexer::GetRuleCount()
	{
		return TokenNames.Count();
	}

	void MetaLexer::SetLexProfile(String lex)
	{
		Errors.Clear();
		ParseLexProfile(lex);
		if (!Errors.Count())
			ConstructDFA();
		else
			dfa = 0;
	}

	bool IsWhiteSpace(wchar_t ch)
	{
		return (ch == L' ' || ch == L'\t' || ch == L'\n' || ch == L'\r' || ch == L'\v');
	}

	bool IsIdent(wchar_t ch)
	{
		return ((ch >=L'A' && ch <= L'Z') || (ch >= L'a' && ch<=L'z') || (ch>=L'0' && ch<=L'9')
			|| ch == L'_' || ch==L'#');
	}

	bool IsLetter(wchar_t ch)
	{
		return ((ch >=L'A' && ch <= L'Z') || (ch >= L'a' && ch<=L'z') || ch == L'_' || ch==L'#');
	}

	bool MetaLexer::ParseLexProfile(const CoreLib::String & lex)
	{
		LinkedList<LexProfileToken> tokens;
		int ptr = 0;
		int state = 0;
		StringBuilder curToken;
		while (ptr < lex.Length())
		{
			wchar_t curChar = lex[ptr];
			wchar_t nextChar = 0;
			if (ptr+1<lex.Length())
				nextChar = lex[ptr+1];
			switch (state)
			{
			case 0:
				{
					if (IsLetter(curChar))
						state = 1;
					else if (IsWhiteSpace(curChar))
						ptr ++;
					else if (curChar == L'{')
					{
						state = 2;
						ptr ++;
					}
					else if (curChar == L'=')
						state = 3;
					else if (curChar == L'/' && nextChar == L'/')
						state = 4;
					else
					{
						LexerError err;
						err.Position = ptr;
						err.Text = String(L"[Profile Error] Illegal character \'") + curChar + L"\'";
						Errors.Add(err);
						ptr ++;
					}
					curToken.Clear();
				}
				break;
			case 1:
				{
					if (IsIdent(curChar))
					{
						curToken.Append(curChar);
						ptr ++;
					}
					else
					{
						LexProfileToken tk;
						tk.str = curToken.ToString();
						tk.type = LexProfileToken::Identifier;
						tokens.AddLast(tk);
						state = 0;
					}
				}
				break;
			case 2:
				{
					if (curChar == L'}' && (nextChar == L'\r' || nextChar == L'\n' || nextChar == 0) )
					{
						LexProfileToken tk;
						tk.str = curToken.ToString();
						tk.type = LexProfileToken::Regex;
						tokens.AddLast(tk);
						ptr ++;
						state = 0;
					}
					else
					{
						curToken.Append(curChar);
						ptr ++;
					}
				}
				break;
			case 3:
				{
					LexProfileToken tk;
					tk.str = curChar;
					tk.type = LexProfileToken::Equal;
					tokens.AddLast(tk);
					ptr ++;
					state = 0;
				}
				break;
			case 4:
				{
					if (curChar == L'\n')
						state = 0;
					else
						ptr ++;
				}
			}
		}

		// Parse tokens
		LinkedNode<LexProfileToken> * l = tokens.FirstNode();
		state = 0;
		String curName, curRegex;
		try
		{
			TokenNames.Clear();
			Regex.Clear();
			while (l)
			{
				curName = ReadProfileToken(l, LexProfileToken::Identifier);
				l = l->GetNext();
				ReadProfileToken(l, LexProfileToken::Equal);
				l = l->GetNext();
				curRegex = ReadProfileToken(l, LexProfileToken::Regex);
				l = l->GetNext();
				TokenNames.Add(curName);
				Regex.Add(curRegex);
				if (curName[0] == L'#')
					Ignore.Add(true);
				else
					Ignore.Add(false);
			}
		}
		catch(int)
		{
			return false;
		}
		return true;
	}

	String MetaLexer::ReadProfileToken(LexProfileTokenNode*n, LexProfileToken::LexProfileTokenType type)
	{
		if (n && n->Value.type == type)
		{
			return n->Value.str;
		}
		else
		{
			String name = L"[Profile Error] ";
			switch (type)
			{
			case LexProfileToken::Equal:
				name = L"\'=\'";
				break;
			case LexProfileToken::Identifier:
				name = L"Token identifier";
				break;
			case LexProfileToken::Regex:
				name = L"Regular expression";
				break;
			}
			name = name + L" expected.";
			LexerError err;
			err.Text = name;
			err.Position = 0;
			Errors.Add(err);
			throw 0;
		}
	}

	DFA_Table * MetaLexer::GetDFA()
	{
		return dfa.operator->();
	}

	void MetaLexer::ConstructDFA()
	{
		RegexParser parser;
		NFA_Graph nfa;
		NFA_Node * node = nfa.CreateNode();
		nfa.SetStartNode(node);
		for (int i=0; i<Regex.Count(); i++)
		{
			RefPtr<RegexNode> tree = parser.Parse(Regex[i]);
			if (tree)
			{
				NFA_Graph cNfa;
				cNfa.GenerateFromRegexTree(tree.operator->(), true);
				cNfa.SetTerminalIdentifier(i);
				nfa.CombineNFA(&cNfa);
				NFA_Translation * trans = nfa.CreateTranslation();
				trans->NodeDest = cNfa.GetStartNode();
				trans->NodeSrc = node;
				trans->NodeDest->PrevTranslations.Add(trans);
				trans->NodeSrc->Translations.Add(trans);
			}
			else
			{
				LexerError err;
				err.Position = 0;
				err.Text = L"Illegal regex for \"" + String(TokenNames[i]) + L"\"";
				Errors.Add(err);
				return;
			}
		}
		nfa.PostGenerationProcess();
		DFA_Graph dfaGraph;
		dfaGraph.Generate(&nfa);
		dfa = new DFA_Table();
		dfaGraph.ToDfaTable(dfa.operator ->());
	}

	LazyLexStream::Iterator & LazyLexStream::Iterator::operator ++()
	{
		auto &str = stream->InputText;
		auto sDfa = stream->GetDFA();
		auto & ignore = stream->GetIgnoreSet();
		if (lastTokenPtr == str.Length())
		{
			lastTokenPtr = -1;
			return *this;
		}

		int lastAcceptState = -1;
		int lastAcceptPtr = -1;
		while (ptr < str.Length())
		{
			if (sDfa->Tags[state]->IsFinal)
			{
				lastAcceptState = state;
				lastAcceptPtr = ptr;
			}
			Word charClass = (*sDfa->CharTable)[str[ptr]];
			if (charClass == 0xFFFF)
			{
				ptr++;
				continue;
			}
			int nextState = sDfa->DFA[state][charClass];
			if (nextState >= 0)
			{
				state = nextState;
				ptr++;
			}
			else
			{
				if (lastAcceptState != -1)
				{
					state = lastAcceptState;
					ptr = lastAcceptPtr;
					
					
					if (!ignore[sDfa->Tags[state]->TerminalIdentifiers[0]])
					{
						currentToken.Length = ptr - lastTokenPtr;
						currentToken.TypeID = sDfa->Tags[state]->TerminalIdentifiers[0];
						currentToken.Position = lastTokenPtr;
						state = sDfa->StartState;
						lastTokenPtr = ptr;
						lastAcceptState = -1;
						lastAcceptPtr = -1;
						break;
					}
					state = sDfa->StartState;
					lastTokenPtr = ptr;
					lastAcceptState = -1;
					lastAcceptPtr = -1;
				}
				else
				{
					ptr++;
					lastAcceptState = lastAcceptPtr = -1;
					lastTokenPtr = ptr;
					state = sDfa->StartState;
					continue;
				}
			}
		}
		if (ptr == str.Length())
		{
			if (sDfa->Tags[state]->IsFinal &&
				!ignore[sDfa->Tags[state]->TerminalIdentifiers[0]])
			{
				currentToken.Length = ptr - lastTokenPtr;
				currentToken.TypeID = sDfa->Tags[state]->TerminalIdentifiers[0];
				currentToken.Position = lastTokenPtr;
			}
			else
			{
				currentToken.Length = 0;
				currentToken.TypeID = -1;
				currentToken.Position = lastTokenPtr;
			}
			lastTokenPtr = ptr;
		}
		
		return *this;
	}

	bool MetaLexer::Parse(String str, LexStream & stream)
	{
		TokensParsed = 0;
		if (!dfa)
			return false;
		int ptr = 0;
		int lastAcceptState = -1;
		int lastAcceptPtr = -1;
		int lastTokenPtr = 0;
		int state = dfa->StartState;
		while (ptr<str.Length())
		{
			if (dfa->Tags[state]->IsFinal)
			{
				lastAcceptState = state;
				lastAcceptPtr = ptr;
			}
			Word charClass = (*dfa->CharTable)[str[ptr]];
			if (charClass == 0xFFFF)
			{
				LexerError err;
				err.Text = String(L"Illegal character \'") + str[ptr] + L"\'";
				err.Position = ptr;
				Errors.Add(err);
				ptr++;
				continue;
			}
			int nextState = dfa->DFA[state][charClass];
			if (nextState >= 0)
			{
				state = nextState;
				ptr++;
			}
			else
			{
				if (lastAcceptState != -1)
				{
					state = lastAcceptState;
					ptr = lastAcceptPtr;
					if (!Ignore[dfa->Tags[state]->TerminalIdentifiers[0]])
					{
						LexToken tk;
						tk.Str = str.SubString(lastTokenPtr, ptr-lastTokenPtr);
						tk.TypeID = dfa->Tags[state]->TerminalIdentifiers[0];
						tk.Position = lastTokenPtr;
						stream.AddLast(tk);
					}
					TokensParsed ++;
					lastTokenPtr = ptr;
					state = dfa->StartState;
					lastAcceptState = -1;
					lastAcceptPtr = -1;
				}
				else
				{
					LexerError err;
					err.Text = L"Illegal token \'" +
						str.SubString(lastTokenPtr, ptr-lastTokenPtr) + L"\'";
					err.Position = ptr;
					Errors.Add(err);
					ptr++;
					lastAcceptState = lastAcceptPtr = -1;
					lastTokenPtr = ptr;
					state = dfa->StartState;
					continue;
				}
			}
		}

		if (dfa->Tags[state]->IsFinal &&
			!Ignore[dfa->Tags[state]->TerminalIdentifiers[0]])
		{
			LexToken tk;
			tk.Str = str.SubString(lastTokenPtr, ptr-lastTokenPtr);
			tk.TypeID = dfa->Tags[state]->TerminalIdentifiers[0];
			stream.AddLast(tk);
			TokensParsed ++;
		}
		return (Errors.Count() == 0);
	}
}
}