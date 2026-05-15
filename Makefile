genlex:
	mkdir -p intermediate
	flex -o intermediate/lexer.yy.c src/lexer.l 

build_lex: genlex
	gcc -o build/lexer intermediate/lexer.yy.c 

run_lex: build_lex
	build/lexer

test_lex: build_lex
	cat test/test.program | build/lexer

genh:
	yacc -Wcounterexamples -o intermediate/y.tab.h -d src/yacc.y

build_yacc: genh genlex
	yacc -o intermediate/y.tab.c src/yacc.y
	cp src/c/*.c src/c/*.h intermediate/
	mkdir -p build
	gcc -o build/yacc intermediate/*.c 

run_yacc: build_yacc
	build/yacc

test_yacc_nobuild:
	cat test/test.program | build/yacc

test_yacc: build_yacc
	cat test/test.program | build/yacc

