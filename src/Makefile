all: prompt parser evaluator sexpr qexpr vars funcs cond

prompt: prompt.c
    gcc -std=c99 -Wall prompt.c -o prompt -ledit

parser: parser.c
	gcc -std=c99 -Wall parser.c mpc.c -o parser -ledit -lm

evaluator: evaluator.c
	gcc -std=c99 -Wall evaluator.c mpc.c -o evaluator -ledit -lm

sexpr: s_expressions.c
	gcc -std=c99 -Wall s_expressions.c mpc.c -o sexpr -ledit -lm

qexpr: q_expressions.c
	gcc -std=c99 -Wall q_expressions.c mpc.c -o qexpr -ledit -lm

vars: variables.c
	gcc -std=c99 -Wall variables.c mpc.c -o vars -ledit -lm

funcs: functions.c
	gcc -std=c99 -Wall functions.c mpc.c -o funcs -ledit -lm

cond: conditions.c
	gcc -std=c99 -Wall conditions.c mpc.c -o cond -ledit -lm
