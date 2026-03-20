build_lex:
	flex -o intermediate/lexer.yy.c src/lexer.l 
	gcc -o build/lexer intermediate/lexer.yy.c 

run_lex: build_lex
	build/lexer

test_lex: build_lex
	cat test/test.c | build/lexer 