#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>

int
main(int argc, char **argv)
{
    printf("COOL version 0.0.0.1\n");
    printf("Press ctrl+c to exit\n");
    
    for (;;) {
        char *input = readline("COOL> ");
        add_history(input);
        fprintf(stdout, "No, you're a %s\n", input);
        free(input);
    }

    return 0;
}
