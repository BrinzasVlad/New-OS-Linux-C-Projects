#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/**
 * Tests to see whether a given option was included in the command line arguments
 * @param argc the total number of command line arguments, as received by main()
 * @param argv the array of command line arguments, as received by main()
 * @param option_to_test_for the option to search for
 * @return true if the option was included, false otherwise
 */
bool received_option(int argc, char* argv[], char* option_to_test_for) {
    for (int i = 0; i < argc; ++i) {
        if (0 == strcmp(argv[i], option_to_test_for)) return true;
    }
    
    return false;
}

int main(int argc, char* argv[]) {
    
    if (received_option(argc, argv, "variant")) {
        printf("10521");
    }
    else {
        // If no implemented option is found, display an error message
        printf("ERROR\n");
        printf("invalid option");
    }
    
    return 0;
}