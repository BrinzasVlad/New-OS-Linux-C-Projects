#include <unistd.h>
#include <sys/wait.h>
#include "a2_helper.h"

void process_8() {
    info(BEGIN, 8, 0);
    
    info(END, 8, 0);
}

void process_7() {
    info(BEGIN, 7, 0);
    
    info(END, 7, 0);
}

void process_6() {
    info(BEGIN, 6, 0);
    
    pid_t process_8_pid = fork();
    if (0 == process_8_pid) {
        process_8();
        return;
    }
    
    waitpid(process_8_pid, NULL, 0);
    info(END, 6, 0);
}

void process_5() {
    info(BEGIN, 5, 0);
    
    info(END, 5, 0);
}

void process_4() {
    info(BEGIN, 4, 0);
    
    pid_t process_7_pid = fork();
    if (0 == process_7_pid) {
        process_7();
        return;
    }
    
    waitpid(process_7_pid, NULL, 0);
    info(END, 4, 0);
}

void process_3() {
    info(BEGIN, 3, 0);
    
    pid_t process_4_pid, process_6_pid;
    
    process_4_pid = fork();
    if (0 == process_4_pid) {
        process_4();
        return;
    }
    
    process_6_pid = fork();
    if (0 == process_6_pid) {
        process_6();
        return;
    }
    
    waitpid(process_4_pid, NULL, 0);
    waitpid(process_6_pid, NULL, 0);
    info(END, 3, 0);
}

void process_2() {
    info(BEGIN, 2, 0);
    
    pid_t process_3_pid = fork();
    if (0 == process_3_pid) {
        process_3();
        return;
    }
    
    waitpid(process_3_pid, NULL, 0);
    info(END, 2, 0);
}

void process_1() {
    info(BEGIN, 1, 0);
    
    pid_t process_2_pid, process_5_pid;
    
    process_2_pid = fork();
    if (0 == process_2_pid) {
        process_2();
        return;
    }
    
    process_5_pid = fork();
    if (0 == process_5_pid) {
        process_5();
        return;
    }
    
    waitpid(process_2_pid, NULL, 0);
    waitpid(process_5_pid, NULL, 0);
    info(END, 1, 0);
}

int main() {
    // Note: normally we should try to handle errors encountered
    // when trying to start new processes or threads. However, for
    // this assignment in particular there isn't much we can do
    // if a such operation fails, save maybe for retrying, because
    // the assignment explicitly requires an exact process hierarchy.
    
    init(); // Call the tester and announce we are starting

    process_1();
    
    return 0;
}