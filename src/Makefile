prompt:
    gcc -std=c99 -Wall prompt.c -o prompt -ledit

parser:
	gcc -std=c99 -Wall parser.c mpc.c -o parser -ledit -lm

evaluator:
	gcc -std=c99 -Wall evaluator.c mpc.c -o evaluator -ledit -lm

expr:
	gcc -std=c99 -Wall s_expressions.c mpc.c -o s_expressions -ledit -lm