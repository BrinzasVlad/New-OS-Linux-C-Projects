#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>

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

/**
 * Returns the value of a parameter given via command line argument (e.g.
 * path=test_root/test_dir/) as a string. If the parameter does not appear
 * in the list of command line arguments, returns NULL.
 * @param argc the total number of command line arguments, as received by main()
 * @param argv the array of command line arguments, as received by main()
 * @param parameter_name the name of the parameter to find (e.g. "path")
 * @return the value as a string (e.g. "test_root/test_dir/"), or NULL
 */
char* get_parameter_value(int argc, char* argv[], char* parameter_name) {
    for (int i = 0; i < argc; ++i) {
        size_t parameter_name_length = strlen(parameter_name);
        if (0 == strncmp(argv[i], parameter_name, parameter_name_length)) {
            char* parameter_value = argv[i] + parameter_name_length + 1; // +1 for the '=' character
            return parameter_value;
        }
    }
    
    return NULL;
}

void print_directory_contents(char* path, bool recursive) {
    DIR* current_directory = opendir(path);
    
    struct dirent* entry;
    for (entry = readdir(current_directory); NULL != entry; entry = readdir(current_directory)) {
        // Skip 'this directory' and 'parent directory' entries
        if (0 == strcmp(entry->d_name, ".") || 0 == strcmp(entry->d_name, "..")) continue;
        
        // Generate full name for entry, including path so far
        size_t full_name_size = strlen(path) + 1 + strlen(entry->d_name) + 1; // +1 for '/' and for '\0'
        char* entry_full_name = (char*)malloc(full_name_size);
        // char entry_full_name[256]; // would've worked on most systems too
        sprintf(entry_full_name, "%s/%s", path, entry->d_name);
        
        printf("%s\n", entry_full_name);
        
        if (recursive) {
            if (DT_DIR == entry->d_type) {
                print_directory_contents(entry_full_name, recursive);
            }
            else if (DT_UNKNOWN == entry->d_type) {
                // Some filesystems do not support d_type properly
                // So we will test manually via stat()
                struct stat entry_stat_buffer;
                if (0 == stat(entry_full_name, &entry_stat_buffer) && S_ISDIR(entry_stat_buffer.st_mode)) {
                    print_directory_contents(entry_full_name, recursive);
                }
            }
        }
        
        free(entry_full_name);
    }
    
    closedir(current_directory);
}

int main(int argc, char* argv[]) {
    
    if (received_option(argc, argv, "variant")) {
        printf("10521");
    }
    else if (received_option(argc, argv, "list")) {
        bool recursive = received_option(argc, argv, "recursive");
        
        char* path = get_parameter_value(argc, argv, "path");
        
        // TODO: read filtering options
        
        // Test that path is valid and points to a directory
        struct stat target_directory_stat_buffer;
        if (0 == stat(path, &target_directory_stat_buffer) && S_ISDIR(target_directory_stat_buffer.st_mode)) {
            printf("SUCCESS\n");
            print_directory_contents(path, recursive);
        }
        else {
            printf("ERROR\n");
            printf("invalid directory path");
        }
    }
    else {
        // If no implemented option is found, display an error message
        printf("ERROR\n");
        printf("invalid option");
    }
    
    return 0;
}