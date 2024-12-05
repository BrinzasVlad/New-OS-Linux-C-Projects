#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>


#define OS_ASSIG_3_MAXIMUM_STRING_MESSAGE_LENGTH 250

void write_string_on_pipe(int pipe_file_descriptor, char* string_to_write) {
    char terminator = '$';
    write(pipe_file_descriptor, string_to_write, strlen(string_to_write));
    write(pipe_file_descriptor, &terminator, 1);
}

void write_number_on_pipe(int pipe_file_descriptor, unsigned int number_to_write) {
    write(pipe_file_descriptor, &number_to_write, sizeof(unsigned int));
}

void read_string_from_pipe(int pipe_file_descriptor, char* read_destination) {
    char character_read;
    int index_in_string = 0;
    while (1 == read(pipe_file_descriptor, &character_read, 1)) {
        // Read message character by character, stop at '$'
        if ('$' == character_read) {
            read_destination[index_in_string] = '\0';
            return;
        }
        
        read_destination[index_in_string] = character_read;
        index_in_string++;
    }
}

unsigned int read_number_from_pipe(int pipe_file_descriptor) {
    unsigned int number_read;
    read(pipe_file_descriptor, &number_read, sizeof(unsigned int));
    
    return number_read;
}

void setup_pipes(int* request_pipe_in_file_descriptor, int* response_pipe_out_file_descriptor) {
    if (0 != mkfifo("RESP_PIPE_10521", 0600)) {
        printf("ERROR\n");
        printf("cannot create the response pipe");
        exit(-1);
    }
    if (-1 == (*request_pipe_in_file_descriptor = open("REQ_PIPE_10521", O_RDONLY))) {
        printf("ERROR\n");
        printf("cannot open the request pipe");
        exit(-2);
    }
    if (-1 == (*response_pipe_out_file_descriptor = open("RESP_PIPE_10521", O_WRONLY))) {
        printf("ERROR\n");
        printf("cannot open the response pipe");
        exit(-3);
    }
    
    write_string_on_pipe(*response_pipe_out_file_descriptor, "CONNECT");
    printf("SUCCESS");
}

void cleanup_pipes(int* request_pipe_in_file_descriptor, int* response_pipe_out_file_descriptor) {
    close(*response_pipe_out_file_descriptor);
    *response_pipe_out_file_descriptor = -1;

    close(*request_pipe_in_file_descriptor);
    *request_pipe_in_file_descriptor = -1;
    
    unlink("RESP_PIPE_10521");
}

char* setup_shared_memory(const char* shared_memory_name, unsigned int shared_memory_size) {
    char* memory_mapped_shared_memory = NULL;
    int shared_memory_file_descriptor = shm_open(shared_memory_name, O_RDWR | O_CREAT, 0644);
    
    if (-1 != shared_memory_file_descriptor) {
        int truncate_return_status = ftruncate(shared_memory_file_descriptor, shared_memory_size);
        
        if (-1 != truncate_return_status) {
            memory_mapped_shared_memory = (char*)mmap(NULL,
                                                      shared_memory_size,
                                                      PROT_READ | PROT_WRITE,
                                                      MAP_SHARED,
                                                      shared_memory_file_descriptor,
                                                      0);
            
            if (MAP_FAILED == memory_mapped_shared_memory) {
                // Set back to NULL, which is our default 'it didn't work' response
                memory_mapped_shared_memory = NULL;
            }
        }
        
        // Regardless of success, we can clean up the shared memory file descriptor
        close(shared_memory_file_descriptor);
    }
    
    return memory_mapped_shared_memory;
}

void cleanup_shared_memory(char* memory_mapped_shared_memory, const char* shared_memory_name, unsigned int shared_memory_size) {
    if (shared_memory_size > 0) {
        munmap(memory_mapped_shared_memory, shared_memory_size);
        shm_unlink(shared_memory_name);
    }
}

struct mapped_file {
    char* at; // Pointer used for accessing the actual memory-mapped file, like file.at[index]
    unsigned int size;
};

struct mapped_file setup_mapped_file(const char* file_path) {
    char* mapped_file_pointer = NULL;
    unsigned int mapped_file_size = 0;
    
    int file_descriptor = open(file_path, O_RDWR);
    if (-1 != file_descriptor) {
        struct stat stat_info;
        fstat(file_descriptor, &stat_info);
        
        mapped_file_size = stat_info.st_size;
        
        mapped_file_pointer = (char*)mmap(NULL,
                                          mapped_file_size,
                                          PROT_READ,
                                          MAP_PRIVATE,
                                          file_descriptor,
                                          0);
        
        if (MAP_FAILED == mapped_file_pointer) {
            // Set values back to default to indicate failure
            mapped_file_pointer = NULL;
            mapped_file_size = 0;
        }
        
        close(file_descriptor);
    }
    
    struct mapped_file structure_to_return;
    structure_to_return.at = mapped_file_pointer;
    structure_to_return.size = mapped_file_size;
    
    return structure_to_return;
}

void cleanup_mapped_file(struct mapped_file* memory_mapped_file) {
    if (NULL != memory_mapped_file->at) {
        munmap(memory_mapped_file->at, memory_mapped_file->size);
        memory_mapped_file->at = NULL;
        memory_mapped_file->size = 0;
    }
}

int main() {
    int request_pipe_in, response_pipe_out;
    setup_pipes(&request_pipe_in, &response_pipe_out);
    
    const char SHARED_MEMORY_NAME[] = "/RmFxwC4";
    char* shared_memory_region = NULL;
    unsigned int shared_memory_size = 0;
    struct mapped_file memory_mapped_file = { NULL, 0 };
    
    char command[OS_ASSIG_3_MAXIMUM_STRING_MESSAGE_LENGTH + 1];
    while (1) {
        read_string_from_pipe(request_pipe_in, command);
        
        if (0 == strcmp(command, "EXIT")) {
            cleanup_shared_memory(shared_memory_region, SHARED_MEMORY_NAME, shared_memory_size);
            cleanup_mapped_file(&memory_mapped_file);
            cleanup_pipes(&request_pipe_in, &response_pipe_out);
            break;
        }
        else {
            // Always echo non-EXIT commands
            write_string_on_pipe(response_pipe_out, command);
        
            if (0 == strcmp(command, "VARIANT")) {
                write_number_on_pipe(response_pipe_out, 10521);
                write_string_on_pipe(response_pipe_out, "VALUE");
            }
            
            if (0 == strcmp(command, "CREATE_SHM")) {
                shared_memory_size = read_number_from_pipe(request_pipe_in);
                shared_memory_region = setup_shared_memory(SHARED_MEMORY_NAME, shared_memory_size);
                
                if (NULL == shared_memory_region) {
                    write_string_on_pipe(response_pipe_out, "ERROR");
                    shared_memory_size = 0;
                }
                else write_string_on_pipe(response_pipe_out, "SUCCESS");
            }
            
            if (0 == strcmp(command, "WRITE_TO_SHM")) {
                unsigned int offset = read_number_from_pipe(request_pipe_in);
                unsigned int value = read_number_from_pipe(request_pipe_in);
                
                if (offset + sizeof(unsigned int) > shared_memory_size){
                    write_string_on_pipe(response_pipe_out, "ERROR");
                }
                else {
                    *(unsigned int*)(shared_memory_region + offset) = value;
                    write_string_on_pipe(response_pipe_out, "SUCCESS");
                }
            }
            
            if (0 == strcmp(command, "MAP_FILE")) {
                char file_name[OS_ASSIG_3_MAXIMUM_STRING_MESSAGE_LENGTH + 1];
                read_string_from_pipe(request_pipe_in, file_name);
                
                memory_mapped_file = setup_mapped_file(file_name);
                
                if (NULL == memory_mapped_file.at) write_string_on_pipe(response_pipe_out, "ERROR");
                else write_string_on_pipe(response_pipe_out, "SUCCESS");
            }
        }
    }
    
    return 0;
}