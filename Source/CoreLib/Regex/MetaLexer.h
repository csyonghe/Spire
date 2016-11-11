#ifndef GX_META_LEXER_H
#define GX_META_LEXER_H

#include "Regex.h"

namespace CoreLib
{
	namespace Text
	{
		class LexToken
		{
		public:
			String Str;
			int TypeID;
			int Position;
		};

		class LazyLexToken
		{
		public:
			int TypeID;
			int Position;
			int Length;
		};

		typedef LinkedList<LexToken> LexStream;
	
		class LexerError
		{
		public:
			String Text;
			int Position;
		};

		struct LexProfileToken
		{
			String str;
			enum LexProfileTokenType
			{
				Identifier,
				Equal,
				Regex
			} type;
		};

		typedef LinkedList<LexProfileToken> LexProfileTokenStream;
		typedef LinkedNode<LexProfileToken> LexProfileTokenNode;

		class LexicalParseException : public Exception
		{
		public:
			int Position;
			LexicalParseException(String str, int pos) : Exception(str), Position(pos)
			{}
		};

		class LazyLexStream
		{
		private:
			RefPtr<DFA_Table> dfa; 
			List<bool> *ignoreSet;
		public:
			String InputText;
			LazyLexStream() = default;
			LazyLexStream(String text, const RefPtr<DFA_Table> & dfa, List<bool> *ignoreSet)
				: dfa(dfa), ignoreSet(ignoreSet), InputText(text)
			{}
			inline DFA_Table * GetDFA()
			{
				return dfa.Ptr();
			}
			inline List<bool> & GetIgnoreSet()
			{
				return *ignoreSet;
			}
			class Iterator
			{
			public:
				int ptr, state, lastTokenPtr;
				LazyLexStream * stream;
				LazyLexToken currentToken;
				bool operator != (const Iterator & iter) const
				{
					return lastTokenPtr != iter.lastTokenPtr;
				}
				bool operator == (const Iterator & iter) const
				{
					return lastTokenPtr == iter.lastTokenPtr;
				}
				LazyLexToken * operator ->()
				{
					return &currentToken;
				}
				LazyLexToken operator *()
				{
					return currentToken;
				}
				Iterator & operator ++();
				Iterator operator ++(int)
				{
					Iterator rs = *this;
					this->operator++();
					return rs;
				}
			};
			Iterator begin()
			{
				Iterator iter;
				iter.ptr = 0;
				iter.lastTokenPtr = 0;
				iter.state = dfa->StartState;
				iter.stream = this;
				++iter;
				return iter;
			}
			Iterator end()
			{
				Iterator iter;
				iter.ptr = InputText.Length();
				iter.lastTokenPtr = -1;
				iter.state = dfa->StartState;
				iter.stream = this;
				return iter;
			}
		};

		class MetaLexer : public Object
		{
		private:
			RefPtr<DFA_Table> dfa;
			List<String> Regex;
			List<String> TokenNames;
			List<bool> Ignore;
			String ReadProfileToken(LexProfileTokenNode*n, LexProfileToken::LexProfileTokenType type);
			bool ParseLexProfile(const String &lex);
			void ConstructDFA();
		public:
			int TokensParsed;
			List<LexerError> Errors;
			MetaLexer(String LexProfile);
			MetaLexer();
			DFA_Table * GetDFA();
			String GetTokenName(int id);
			int GetRuleCount();
			void SetLexProfile(String lex);
			bool Parse(String str, LexStream & stream);
			LazyLexStream Parse(String str)
			{
				return LazyLexStream(str, dfa, &Ignore);
			}
		};
	}
}

#endif