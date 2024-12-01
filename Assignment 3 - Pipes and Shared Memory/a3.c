#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


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

int main() {
    int request_pipe_in, response_pipe_out;
    if (0 != mkfifo("RESP_PIPE_10521", 0600)) {
        printf("ERROR\n");
        printf("cannot create the response pipe");
        return -1;
    }
    if (-1 == (request_pipe_in = open("REQ_PIPE_10521", O_RDONLY))) {
        printf("ERROR\n");
        printf("cannot open the request pipe");
        return -2;
    }
    if (-1 == (response_pipe_out = open("RESP_PIPE_10521", O_WRONLY))) {
        printf("ERROR\n");
        printf("cannot open the response pipe");
        return -3;
    }
    write_string_on_pipe(response_pipe_out, "CONNECT");
    printf("SUCCESS");
    
    char command[OS_ASSIG_3_MAXIMUM_STRING_MESSAGE_LENGTH + 1];
    do {
        read_string_from_pipe(request_pipe_in, command);
        
        if (0 == strcmp(command, "VARIANT")) {
            write_string_on_pipe(response_pipe_out, "VARIANT");
            write_number_on_pipe(response_pipe_out, 10521);
            write_string_on_pipe(response_pipe_out, "VALUE");
        }
    } while (0 != strcmp(command, "EXIT"));
    
    close(response_pipe_out);
    close(request_pipe_in);
    unlink("RESP_PIPE_10521");
    return 0;
}