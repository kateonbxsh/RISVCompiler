genlex:
	flex -o intermediate/lexer.yy.c src/lexer.l 

build_lex: genlex
	gcc -o build/lexer intermediate/lexer.yy.c 

run_lex: build_lex
	build/lexer

test_lex: build_lex
	cat test/test.c | build/lexer

genh: 
	yacc -o intermediate/y.tab.h -d src/yacc.y

build_yacc: genh genlex
	yacc -o intermediate/y.tab.c src/yacc.y
	gcc -o build/yacc intermediate/y.tab.c intermediate/lexer.yy.c 

run_yacc: build_yacc
	build/yacc

test_yacc: build_yacc
	cat test/test.c | build/yacc



	