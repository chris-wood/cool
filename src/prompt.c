#include <stdio.h>

static char input[2048];

int
main(int argc, char **argv)
{
    printf("COOL version 0.0.0.1\n");
    printf("Press ctrl+c to exit\n");
    
    for (;;) {
        fprintf(stdout, "COOL> ");
        fgets(input, 2048, stdin);
        fprintf(stdout, "No, you're a %s", input);
    }

    return 0;
}
