add_library(CoreLib_Regex 
 MetaLexer.cpp
 MetaLexer.h
 Regex.cpp
 Regex.h
 RegexDFA.cpp
 RegexDFA.h
 RegexNFA.cpp
 RegexNFA.h
 RegexParser.cpp
 RegexTree.cpp
 RegexTree.h
)

target_link_libraries(CoreLib_Regex CoreLib_Basic)