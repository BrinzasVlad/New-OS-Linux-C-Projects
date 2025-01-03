#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <fcntl.h>


#define OS_ASSIG_1_LINE_SEPARATOR 0x0a

#define OS_ASSIG_1_VALID_SF 1
#define OS_ASSIG_1_WRONG_MAGIC 2
#define OS_ASSIG_1_WRONG_VERSION 3
#define OS_ASSIG_1_WRONG_NUMBER_OF_SECTIONS 4
#define OS_ASSIG_1_WRONG_SECTION_TYPE 5


#define OS_ASSIG_1_MAGIC_SIZE 4
#define OS_ASSIG_1_SECTION_NAME_SIZE 15
struct sf_section_header {
    char name[OS_ASSIG_1_SECTION_NAME_SIZE + 1];
    uint8_t type;
    uint32_t offset;
    uint32_t size;
};

struct sf_file_header {
    char magic[OS_ASSIG_1_MAGIC_SIZE + 1];
    uint16_t header_size;
    uint32_t version;
    uint8_t number_of_sections;
    struct sf_section_header sections[255];
    // We could also use a pointer, then allocate via malloc().
    // But since that would require usage of free() and section
    // headers are small, and there can be at most 255 sections
    // (maximum value for a uint8_t), and we're unlikely to have
    // more than one header at a time (never, in the current form
    // of this assignment), and 255 x 28 = 7140 bytes (~7 kB), I
    // chose this instead.
};

struct minimum_K_sections_with_exactly_L_lines {
    unsigned int sections;
    unsigned int lines;
};

struct directory_list_filter_options {
    char* name_ends_with;
    char* permissions_rwx;
    struct minimum_K_sections_with_exactly_L_lines min_sections_with_exact_lines;
};

// Object you can use to initialize directory_list_filter_options entities
// so that they have no filters enabled at start.
const struct directory_list_filter_options NO_FILTERS = {
    .name_ends_with = NULL,
    .permissions_rwx = NULL,
    .min_sections_with_exact_lines = {
        .sections = 0,
        .lines = 0
    }
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
 * Attempts to extract the file header data from a file.
 * No validation is performed; the returned result should be
 * validated separately to make sure a correct SF file was scanned.
 *
 * If the file is too short to read a complete header from, this
 * function will fill in as much data as available, and the rest will
 * be returned initialized to 0 (for numerical fields) or "" (for
 * string fields).
 *
 * @param file_descriptor a valid file descriptor, as returned by open()
 * @return an sf_file_header structure containing the header data
 */
struct sf_file_header extract_file_header(int file_descriptor) {
    const off_t initial_offset = lseek(file_descriptor, 0, SEEK_CUR); // Save current offset to restore when done
    
    struct sf_file_header header; // Structs are 0-initialized by default
    
    // Simply read assuming that the bytes exist and can be read
    // If read() fails for any reason, or returns less bytes than
    // needed (e.g. because of end-of-file), then the function will
    // simply deliver an incomplete result as announced, and the
    // validating function will detect this issue
    //
    // It's probably workable to do a single read() simply scooping
    // up the entire start of the file into header, but that's both
    // quite untransparent and does not account for struct padding,
    // so I've chosen to read the fields one by one.
    lseek(file_descriptor, 0, SEEK_SET);
    read(file_descriptor, &(header.magic), OS_ASSIG_1_MAGIC_SIZE);
    header.magic[OS_ASSIG_1_MAGIC_SIZE] = '\0'; // Add terminator, since read() does not
    read(file_descriptor, &(header.header_size), sizeof(header.header_size));
    read(file_descriptor, &(header.version), sizeof(header.version));
    read(file_descriptor, &(header.number_of_sections), sizeof(header.number_of_sections));
    if (header.number_of_sections > 0) {
        for (int i = 0; i < header.number_of_sections; ++i) {
            read(file_descriptor, &(header.sections[i].name), OS_ASSIG_1_SECTION_NAME_SIZE);
            header.sections[i].name[OS_ASSIG_1_SECTION_NAME_SIZE] = '\0'; // Add terminator since read() does not
            read(file_descriptor, &(header.sections[i].type), sizeof(header.sections[i].type));
            read(file_descriptor, &(header.sections[i].offset), sizeof(header.sections[i].offset));
            read(file_descriptor, &(header.sections[i].size), sizeof(header.sections[i].size));
        }
    }
    
    lseek(file_descriptor, initial_offset, SEEK_SET); // Set file descriptor back to initial position
    return header;
}

/**
 * Validates a given SF file using its header.
 * Returns one of the following codes, depending on the result:
 * - OS_ASSIG_1_VALID_SF if file is valid
 * - OS_ASSIG_1_WRONG_MAGIC if magic is invalid
 * - OS_ASSIG_1_WRONG_VERSION is version is invalid
 * - OS_ASSIG_1_WRONG_NUMBER_OF_SECTIONS if number of sections is invalid
 * - OS_ASSIG_1_WRONG_SECTION_TYPE if a section has an invalid type
 *
 * @param header an sf_file_header structure, as returned by extract_file_header()
 * @return a code as specified above
 */
int validate_sf_file(struct sf_file_header header) {
    if (0 != strcmp("9Q6d", header.magic)) return OS_ASSIG_1_WRONG_MAGIC;
    if (header.version < 107 || header.version > 144) return OS_ASSIG_1_WRONG_VERSION;
    if (header.number_of_sections != 2 &&
        (header.number_of_sections < 5 || header.number_of_sections > 17)) return OS_ASSIG_1_WRONG_NUMBER_OF_SECTIONS;
    
    for (int i = 0; i < header.number_of_sections; ++i) {
        if(header.sections[i].type != 33 &&
           header.sections[i].type != 28 &&
           header.sections[i].type != 38 &&
           header.sections[i].type != 54) return OS_ASSIG_1_WRONG_SECTION_TYPE;
    }
    
    return OS_ASSIG_1_VALID_SF;
}

/**
 * Prints an sf_file_header structure using the
 * format required in the assignment.
 * @param header the sf_file_header to be printed
 */

void print_sf_file_header(struct sf_file_header header) {
    printf("version=%" PRIu32 "\n", header.version);
    printf("nr_sections=%" PRIu8 "\n", header.number_of_sections);
    for (int i = 0; i < header.number_of_sections; ++i) {
        printf("section%d: %s", i+1, header.sections[i].name);
        printf(" %" PRIu8 " %" PRIu32 "\n", header.sections[i].type, header.sections[i].size);
    }
}

/**
 * Extracts the specified line from the specified section
 * from a valid SF file. If the line is not found (e.g.
 * because the section ends before reaching it), NULL
 * will be returned.
 * 
 * The returned line is malloc()-allocated, and should be
 * free()-d once done operating on.
 *
 * @param file_descriptor an open file descriptor corresponding
 *        to a valid SF file
 * @param header the header of the SF file, as returned by
 *        extract_file_header()
 * @param section_number the section from which to extract the
 *        line, where first section is section number 1
 * @param line_number the number of the line to extract, where
 *        first line is line number 1
 * @return a string containing the line contents, or NULL
 */
char* extract_line_from_sf_file(int file_descriptor, struct sf_file_header header, uint8_t section_number, uint64_t line_number) {
    // Read file in chunks, to minimize waiting for read();
    const static unsigned long CHUNK_SIZE = 65536; // 2^16 bytes
    
    // Store original file offset so we can restore it at the end
    int initial_offset = lseek(file_descriptor, 0, SEEK_CUR);
    
    
    section_number -= 1; // Working with 0-indexed sections
    uint32_t section_bytes_left = header.sections[section_number].size;
    uint32_t section_start = header.sections[section_number].offset + header.sections[section_number].size;
    // Sections are written backwards, so it starts at the "end".
    
    uint32_t result_line_size = 100; // Start with a reasonable line size
    char* result_line = malloc(result_line_size);
    uint32_t result_line_write_index = 0;
    
    lseek(file_descriptor, section_start, SEEK_SET);
    char current_chunk[CHUNK_SIZE];
    unsigned int current_line = 1;
    // If target line is line 1, we'll always find something
    // Otherwise, it's possible it doesn't exist
    bool reached_target_line = (line_number == 1);
    
    while (section_bytes_left > 0 && current_line <= line_number) {
        // Use signed long because else `-bytes_to_read` evaluates silly and we need to cast
        long bytes_to_read = (section_bytes_left > CHUNK_SIZE) ? CHUNK_SIZE : section_bytes_left;
        lseek(file_descriptor, -bytes_to_read, SEEK_CUR); // Move to start of chunk
        read(file_descriptor, current_chunk, bytes_to_read);
        lseek(file_descriptor, -bytes_to_read, SEEK_CUR); // Counteract read() moving pointer forward
        section_bytes_left -= bytes_to_read;
        
        for (long long i = bytes_to_read - 1; i >= 0; --i) { // Go backwards because sections are written in reverse
            if (OS_ASSIG_1_LINE_SEPARATOR == current_chunk[i]) {
                current_line++;
                if (current_line == line_number) reached_target_line = true;
                if (current_line > line_number) break; // Target line fully read, we can stop
                
                continue; // Don't include line separator of previous line in result line
            }
            
            if (current_line == line_number) {
                // If we'd overflow the result line, reallocate it bigger
                if (result_line_write_index >= result_line_size) {
                    result_line_size *= 2;
                    result_line = realloc(result_line, result_line_size);
                    // TODO: we should maybe error-check memory allocation operations
                }
                
                result_line[result_line_write_index] = current_chunk[i];
                result_line_write_index++;
            }
        }
    }
    
    // Restore file offset to original position, just in case
    lseek(file_descriptor, initial_offset, SEEK_SET);
    
    if (reached_target_line) {
        // Extremely unlikely, but make sure we won't overflow the line
        if (result_line_write_index == result_line_size) {
            result_line = realloc(result_line, result_line_size + 1);
        }
        result_line[result_line_write_index] = '\0'; // Add string terminator
        return result_line;
    }
    else {
        free(result_line);
        return NULL;
    }
}

/**
 * Tests whether a given file (specified via file name + path)
 * is a valid SF file that has at least K sections that have
 * exactly L lines each.
 * @param file_name the file to be tested
 * @param criteria a structure defining the number of sections K
 *        and the number of lines L to be tested for
 * @return true if the file matches, false if not
 */
bool test_sections_with_lines(char* file_name, struct minimum_K_sections_with_exactly_L_lines criteria) {
    int file_descriptor = open(file_name, O_RDONLY);
    if (-1 == file_descriptor) {
        // File couldn't be opened, assume invalid
        return false;
    }
    
    struct sf_file_header header = extract_file_header(file_descriptor);
    if (header.number_of_sections < criteria.sections) {
        // File has fewer total sections than need to match
        close(file_descriptor);
        return false;
    }
    
    if (OS_ASSIG_1_VALID_SF != validate_sf_file(header)) {
        // It's an invalid SF file
        close(file_descriptor);
        return false;
    }
    
    unsigned int matching_sections = 0;
    for (int i = 0; i < header.number_of_sections; ++i) {
        unsigned int lines_in_section = 1;
        
        // Read file in chunks, to minimize waiting for read();
        const static unsigned long CHUNK_SIZE = 65536; // 2^16 bytes
        char current_chunk[CHUNK_SIZE];
        uint32_t section_bytes_left = header.sections[i].size;
        
        lseek(file_descriptor, header.sections[i].offset, SEEK_SET);
        while (section_bytes_left > 0 && lines_in_section < criteria.lines) {
            unsigned long bytes_to_read = (section_bytes_left > CHUNK_SIZE) ? CHUNK_SIZE : section_bytes_left;
            read(file_descriptor, current_chunk, bytes_to_read);
            section_bytes_left -= bytes_to_read;
            
            for (int j = 0; j < bytes_to_read; ++j) {
                // Use line separator to check line count, ignore everything else
                if (OS_ASSIG_1_LINE_SEPARATOR == current_chunk[j]) {
                    lines_in_section++;
                    if (lines_in_section > criteria.lines) {
                        // Section doesn't match (too many lines), we can stop
                        break;
                    }
                }
            }
        }
        
        if (lines_in_section == criteria.lines) {
            matching_sections++;
            if (matching_sections >= criteria.sections) {
                // File has AT LEAST enough matching sections, can return true
                close(file_descriptor);
                return true;
            }
        }
    }
    
    // If we checked all sections and didn't find enough matching ones, file doesn't match
    close(file_descriptor);
    return false;
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
 * - if filter_options.min_sections_with_exact_lines has field sections
 *   different from zero, will only display elements that are valid SF
 *   files with at least min_sections_with_exact_lines.sections sections
 *   having exactly min_sections_with_exact_lines.lines lines each. (e.g.
 *   only valid SF files with at least 2 sections of exactly 13 lines)
 * Note that if the recursive flag is true, the function will recurse into
 * sub-directories even if they do not match the filter criteria (and hence
 * are not printed).
 *
 * @param path the path to the directory where the listing starts
 * @param recursive whether to recursively list subdirectories
 * @param filter_options a structure defining filtering options, as explained
 *        above
 * @return nothing; the results are printed to STDOUT
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
        bool sections_with_lines_matches = 0 == filter_options.min_sections_with_exact_lines.sections
                                       || ( S_ISREG(entry_stat_buffer.st_mode)
                                         && test_sections_with_lines(entry_full_name, filter_options.min_sections_with_exact_lines));
        if(name_ending_matches && permissions_match && sections_with_lines_matches) {
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
        
        struct directory_list_filter_options filter_options = NO_FILTERS;
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
    else if (received_option(argc, argv, "parse")) {
        char* path = get_parameter_value(argc, argv, "path");
        
        int file_descriptor = open(path, O_RDONLY);
        if (-1 == file_descriptor) {
            printf("ERROR\n");
            printf("invalid file path");
        }
        else {
            struct sf_file_header header = extract_file_header(file_descriptor);
            int validity_status = validate_sf_file(header);
            if (OS_ASSIG_1_VALID_SF == validity_status) {
                printf("SUCCESS\n");
                print_sf_file_header(header);
            }
            else {
                printf("ERROR\n");
                switch (validity_status) {
                    case OS_ASSIG_1_WRONG_MAGIC:
                        printf("wrong magic");
                        break;
                    case OS_ASSIG_1_WRONG_VERSION:
                        printf("wrong version");
                        break;
                    case OS_ASSIG_1_WRONG_NUMBER_OF_SECTIONS:
                        printf("wrong sect_nr");
                        break;
                    case OS_ASSIG_1_WRONG_SECTION_TYPE:
                        printf("wrong sect_types");
                        break;
                }
            }
            close(file_descriptor);
        }
    }
    else if (received_option(argc, argv, "extract")) {
        char* path = get_parameter_value(argc, argv, "path");
        uint8_t section_number = (uint8_t) atoi(get_parameter_value(argc, argv, "section"));
        uint64_t line_number = (uint64_t) atoll(get_parameter_value(argc, argv, "line"));
        
        int file_descriptor = open(path, O_RDONLY);
        if (-1 == file_descriptor) {
            printf("ERROR\n");
            printf("invalid file");
        }
        else {
            struct sf_file_header header = extract_file_header(file_descriptor);
            if (OS_ASSIG_1_VALID_SF != validate_sf_file(header)) {
                printf("ERROR\n");
                printf("invalid file");
            }
            else {
                if (section_number > header.number_of_sections) {
                    printf("ERROR\n");
                    printf("invalid section");
                }
                else {
                    char* line = extract_line_from_sf_file(file_descriptor, header, section_number, line_number);
                    
                    if (NULL == line) {
                        printf("ERROR\n");
                        printf("invalid line");
                    }
                    else {
                        printf("SUCCESS\n");
                        printf("%s", line);
                        free(line);
                    }
                }
            }
            close(file_descriptor);
        }
    }
    else if (received_option(argc, argv, "findall")) {
        char* path = get_parameter_value(argc, argv, "path");
        const int EXACT_LINES = 13;
        const int MIN_NUMBER_SECTIONS_WITH_LINES = 2;
        const bool RECURSIVE = true;
        
        struct directory_list_filter_options filter_options = NO_FILTERS;
        filter_options.min_sections_with_exact_lines.sections = MIN_NUMBER_SECTIONS_WITH_LINES;
        filter_options.min_sections_with_exact_lines.lines = EXACT_LINES;
        
        // Test that path is valid and points to a directory
        struct stat target_directory_stat_buffer;
        if (0 == stat(path, &target_directory_stat_buffer) && S_ISDIR(target_directory_stat_buffer.st_mode)) {
            printf("SUCCESS\n");
            print_directory_contents(path, RECURSIVE, filter_options);
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