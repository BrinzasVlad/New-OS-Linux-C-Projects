#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>

// 01000 is outside of the 0000 - 0777 range for file permissions
# define OS_A1_INVALID_PERMISSIONS 01000
struct directory_list_filter_options {
    char* name_ends_with;
    char* permissions_rwx;
};

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

/**
 * Test whether the permissions contained in the inode data of
 * of the given stat structure match the permissions given as
 * argument.
 * @param statbuf a pointer to a stat structure as returned by stat()
 * @param permissions target permission, in RWX format (e.g. 'rwxrw-r--')
 * @return true if the permissions match, false if they do not
 */
bool test_permissions_match(struct stat *statbuf, char *permissions) {
     char file_perm[10];
     file_perm[9] = '\0';

     if (statbuf->st_mode & S_IRUSR) // Check owner's READ
        file_perm[0] = 'r';
	 else
        file_perm[0] = '-';

	if (statbuf->st_mode & S_IWUSR) // Check owner's WRITE
        file_perm[1] = 'w';
	else
        file_perm[1] = '-';

	if (statbuf->st_mode & S_IXUSR) // Check owner's EXECUTE
		file_perm[2] = 'x';
	else
		file_perm[2] = '-';

	if (statbuf->st_mode & S_IRGRP) // Check group's READ
		file_perm[3] = 'r';
	else
		file_perm[3] = '-';

	if (statbuf->st_mode & S_IWGRP) // Check group's WRITE
		file_perm[4] = 'w';
	else
		file_perm[4] = '-';

	if (statbuf->st_mode & S_IXGRP) // Check group's EXECUTION
		file_perm[5] = 'x';
	else
		file_perm[5] = '-';

	if (statbuf->st_mode & S_IROTH) // Check others' READ
		file_perm[6] = 'r';
	else
		file_perm[6] = '-';

	if (statbuf->st_mode & S_IWOTH) // Check others' WRITE
		file_perm[7] = 'w';
	else
		file_perm[7] = '-';

	if (statbuf->st_mode & S_IXOTH) // Check others' EXECUTION
		file_perm[8] = 'x';
	else
		file_perm[8] = '-';

    if(0 == strcmp(file_perm, permissions))
        return true;
    else
        return false;
}

/**
 * Tests whether the given string ends with the given ending. For
 * example, "stone horse" ends with "orse", but does not end with
 * "norse".
 * @param string_to_test the string to be tested
 * @param ending the ending to test for
 * @return true if the string ends with the ending, false otherwise
 */
bool string_ends_with(char* string_to_test, char* ending) {
    size_t string_len = strlen(string_to_test);
    size_t ending_len = strlen(ending);
    char* ending_of_string = string_to_test + string_len - ending_len;
    
    if (0 == strcmp(ending_of_string, ending)) return true;
    else return false;
}

/**
 * Prints the full name (file name + path until there, for example
 * "test_root/test_dir/test_file.txt") of all elements in the target
 * directory. If the recursive flag is true, recursively prints any
 * elements in sub-directories encountered as well.
 *
 * Additional filtering options are available through the filter_options
 * argument:
 * - if filter_options.name_ends_with is not NULL, will only display
 *   elements whose filename ends with the given sequence
 * - if filter_options.permissions_mask is not NULL, will only display
 *   elemenets whose permissions (as given by stat()) match the given ones
 * Note that if the recursive flag is true, the function will recurse into
 * sub-directories even if they do not match the filter criteria (and hence
 * are not printed).
 */
void print_directory_contents(char* path, bool recursive, struct directory_list_filter_options filter_options) {
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
        
        // Call stat() to get entry info for recursion / filtering
        struct stat entry_stat_buffer;
        stat(entry_full_name, &entry_stat_buffer); // TODO: unspecified what to do if stat() returns an error
        
        // Extracting the conditions misses out on if shortcutting (unless optimizer kicks in),
        // but I like the added readability.
        bool name_ending_matches = NULL == filter_options.name_ends_with
                                || string_ends_with(entry->d_name, filter_options.name_ends_with);
        bool permissions_match = NULL == filter_options.permissions_rwx
                              || test_permissions_match(&entry_stat_buffer, filter_options.permissions_rwx);
        if(name_ending_matches && permissions_match) {
            printf("%s\n", entry_full_name);
        }
        
        if (recursive && S_ISDIR(entry_stat_buffer.st_mode)) {
            print_directory_contents(entry_full_name, recursive, filter_options);
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
        
        struct directory_list_filter_options filter_options;
        filter_options.name_ends_with = get_parameter_value(argc, argv, "name_ends_with");
        filter_options.permissions_rwx = get_parameter_value(argc, argv, "permissions");
        // TODO: validate that permissions are formatted correctly?
        
        // Test that path is valid and points to a directory
        struct stat target_directory_stat_buffer;
        if (0 == stat(path, &target_directory_stat_buffer) && S_ISDIR(target_directory_stat_buffer.st_mode)) {
            printf("SUCCESS\n");
            print_directory_contents(path, recursive, filter_options);
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