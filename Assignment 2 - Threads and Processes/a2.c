#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "a2_helper.h"

struct os_assig_2_thread_data {
    int process_id;
    int thread_id;
};

void* generic_thread(void* thread_info_as_void) {
    struct os_assig_2_thread_data data = *(struct os_assig_2_thread_data*)thread_info_as_void;
    
    info(BEGIN, data.process_id, data.thread_id);
    
    // do_work();
    
    info(END, data.process_id, data.thread_id);
    
    return NULL;
}

void process_8() {
    info(BEGIN, 8, 0);
    
    info(END, 8, 0);
}

struct process_6_process_7_synchronization {
    pthread_mutex_t guard;
    pthread_cond_t process_6_thread_5_exited_cond;
    bool process_6_thread_5_exited;
    pthread_cond_t process_7_thread_1_exited_cond;
    bool process_7_thread_1_exited;
};

void initialize_process_6_and_7_sync(struct process_6_process_7_synchronization* sync) {
    pthread_mutexattr_t mutex_attributes;
    pthread_mutexattr_init(&mutex_attributes);
    pthread_mutexattr_setpshared(&mutex_attributes, PTHREAD_PROCESS_SHARED);
    
    pthread_condattr_t condition_attributes;
    pthread_condattr_init(&condition_attributes);
    pthread_condattr_setpshared(&condition_attributes, PTHREAD_PROCESS_SHARED);
    
    pthread_mutex_init(&sync->guard, &mutex_attributes);
    pthread_cond_init(&sync->process_6_thread_5_exited_cond, &condition_attributes);
    pthread_cond_init(&sync->process_7_thread_1_exited_cond, &condition_attributes);
    sync->process_6_thread_5_exited = false;
    sync->process_7_thread_1_exited = false;
}

struct process_6_process_7_synchronization* get_process_6_and_7_sync() {
    int shared_memory_file_descriptor = shm_open("OS_ASSIG_2_P6_P7_SYNC_SHM", O_RDWR | O_CREAT | O_EXCL, 0600);
    bool this_process_is_creator = false;
    
    if (shared_memory_file_descriptor > 0) {
        // Only one process should do initialization
        // So whichever process first manages the O_EXCL open does it
        this_process_is_creator = true;
        ftruncate(shared_memory_file_descriptor, sizeof(struct process_6_process_7_synchronization));
    }
    else {
        shared_memory_file_descriptor = shm_open("OS_ASSIG_2_P6_P7_SYNC_SHM", O_RDWR, 0600);
    }
    
    struct process_6_process_7_synchronization* sync =
        (struct process_6_process_7_synchronization*)mmap(NULL,
                                                          sizeof(struct process_6_process_7_synchronization),
                                                          PROT_READ | PROT_WRITE, MAP_SHARED,
                                                          shared_memory_file_descriptor,
                                                          0);
    close(shared_memory_file_descriptor);
    
    if (this_process_is_creator) {
        initialize_process_6_and_7_sync(sync);
    }
    
    return sync;
}

void cleanup_process_6_and_7_sync(struct process_6_process_7_synchronization* sync) {
    munmap(sync, sizeof(struct process_6_process_7_synchronization));
    shm_unlink("OS_ASSIG_2_P6_P7_SYNC_SHM");
}

struct process_7_synchronization {
    pthread_mutex_t guard;
    pthread_cond_t thread_5_started_cond;
    bool thread_5_started;
    pthread_cond_t thread_2_exited_cond;
    bool thread_2_exited;
};

void initialize_process_7_sync(struct process_7_synchronization* sync) {
    pthread_mutex_init(&sync->guard, NULL);
    pthread_cond_init(&sync->thread_5_started_cond, NULL);
    sync->thread_5_started = false;
    pthread_cond_init(&sync->thread_2_exited_cond, NULL);
    sync->thread_2_exited = false;
}

void* process_7_thread_1(void* process_6_and_7_sync_as_void) {
    struct process_6_process_7_synchronization* process_6_and_7_sync =
        (struct process_6_process_7_synchronization*)process_6_and_7_sync_as_void;
    
    // Wait for T6.5 to end
    pthread_mutex_lock(&process_6_and_7_sync->guard);
        while (!process_6_and_7_sync->process_6_thread_5_exited) {
            pthread_cond_wait(&process_6_and_7_sync->process_6_thread_5_exited_cond, &process_6_and_7_sync->guard);
        }
    pthread_mutex_unlock(&process_6_and_7_sync->guard);
    
    info(BEGIN, 7, 1);
    
    // do_work();
    
    // Terminate and notify T6.3
    info(END, 7, 1);
    pthread_mutex_lock(&process_6_and_7_sync->guard);
        process_6_and_7_sync->process_7_thread_1_exited = true;
        pthread_cond_signal(&process_6_and_7_sync->process_7_thread_1_exited_cond);
    pthread_mutex_unlock(&process_6_and_7_sync->guard);
    
    return NULL;
}

void* process_7_thread_2(void* process_7_sync_as_void) {
    struct process_7_synchronization* process_7_sync = (struct process_7_synchronization*)process_7_sync_as_void;
    
    // Wait for T7.5 to start
    pthread_mutex_lock(&process_7_sync->guard);
        while (!process_7_sync->thread_5_started) {
            pthread_cond_wait(&process_7_sync->thread_5_started_cond, &process_7_sync->guard);
        }
    pthread_mutex_unlock(&process_7_sync->guard);
    
    info(BEGIN, 7, 2);
    
    // do_work();
    
    // Terminate and notify T7.5
    info(END, 7, 2);
    pthread_mutex_lock(&process_7_sync->guard);
        process_7_sync->thread_2_exited = true;
        pthread_cond_signal(&process_7_sync->thread_2_exited_cond);
    pthread_mutex_unlock(&process_7_sync->guard);
    
    return NULL;
}

void* process_7_thread_5(void* process_7_sync_as_void) {
    struct process_7_synchronization* process_7_sync = (struct process_7_synchronization*)process_7_sync_as_void;
    
    // Start and notify T7.2
    info(BEGIN, 7, 5);
    pthread_mutex_lock(&process_7_sync->guard);
        process_7_sync->thread_5_started = true;
        pthread_cond_signal(&process_7_sync->thread_5_started_cond);
    pthread_mutex_unlock(&process_7_sync->guard);
    
    // do_work();
    
    // Wait for T7.2 to end
    pthread_mutex_lock(&process_7_sync->guard);
        while (!process_7_sync->thread_2_exited) {
            pthread_cond_wait(&process_7_sync->thread_2_exited_cond, &process_7_sync->guard);
        }
    pthread_mutex_unlock(&process_7_sync->guard);
    
    info(END, 7, 5);
    
    return NULL;
}

void process_7() {
    info(BEGIN, 7, 0);
    
    struct process_7_synchronization process_7_sync;
    initialize_process_7_sync(&process_7_sync);
    
    struct process_6_process_7_synchronization* process_6_and_7_sync = get_process_6_and_7_sync();
    
    pthread_t threads[5];
    struct os_assig_2_thread_data thread_arguments[5];
    // The above is a bit inefficient, since T7.1, T7.2, and T7.5 don't use theirs
    // But the space waste is minor enough to ignore (and might be optimized out anyway)
    for (int thread_number = 1; thread_number <= 5; ++thread_number) {
        int thread_index = thread_number - 1;
        switch (thread_number) {
            case 1:
                pthread_create(&threads[thread_index], NULL, process_7_thread_1, process_6_and_7_sync);
                break;
            case 2:
                pthread_create(&threads[thread_index], NULL, process_7_thread_2, &process_7_sync);
                break;
            case 5:
                pthread_create(&threads[thread_index], NULL, process_7_thread_5, &process_7_sync);
                break;
            default:
                thread_arguments[thread_index].process_id = 7;
                thread_arguments[thread_index].thread_id = thread_number;
                pthread_create(&threads[thread_index], NULL, generic_thread, &thread_arguments[thread_index]);
        }
        
    }
    
    for (int i = 0; i < 5; ++i) {
        pthread_join(threads[i], NULL);
    }
    
    cleanup_process_6_and_7_sync(process_6_and_7_sync);
    
    info(END, 7, 0);
}

void* process_6_thread_3(void* process_6_and_7_sync_as_void) {
    struct process_6_process_7_synchronization* process_6_and_7_sync =
        (struct process_6_process_7_synchronization*)process_6_and_7_sync_as_void;
    
    // Wait for T7.1 to end
    pthread_mutex_lock(&process_6_and_7_sync->guard);
        while (!process_6_and_7_sync->process_7_thread_1_exited) {
            pthread_cond_wait(&process_6_and_7_sync->process_7_thread_1_exited_cond, &process_6_and_7_sync->guard);
        }
    pthread_mutex_unlock(&process_6_and_7_sync->guard);
    
    info(BEGIN, 6, 3);
    
    // do_work();
    
    info(END, 6, 3);
    return NULL;
}

void* process_6_thread_5(void* process_6_and_7_sync_as_void) {
    struct process_6_process_7_synchronization* process_6_and_7_sync =
        (struct process_6_process_7_synchronization*)process_6_and_7_sync_as_void;
    
    info(BEGIN, 6, 5);
    
    // do_work();
    
    // Terminate and notify T7.1
    info(END, 6, 5);
    pthread_mutex_lock(&process_6_and_7_sync->guard);
        process_6_and_7_sync->process_6_thread_5_exited = true;
        pthread_cond_signal(&process_6_and_7_sync->process_6_thread_5_exited_cond);
    pthread_mutex_unlock(&process_6_and_7_sync->guard);
    
    return NULL;
}

void process_6() {
    info(BEGIN, 6, 0);
    
    pid_t process_8_pid = fork();
    if (0 == process_8_pid) {
        process_8();
        return;
    }
    
    struct process_6_process_7_synchronization* process_6_and_7_sync = get_process_6_and_7_sync();
    
    pthread_t thread_ids[6];
    struct os_assig_2_thread_data thread_arguments[6];
    for (int thread_number = 1; thread_number <= 6; ++thread_number) {
        int thread_index = thread_number - 1;
        switch (thread_number) {
            case 3:
                pthread_create(&thread_ids[thread_index], NULL, process_6_thread_3, process_6_and_7_sync);
                break;
            case 5:
                pthread_create(&thread_ids[thread_index], NULL, process_6_thread_5, process_6_and_7_sync);
                break;
            default:
                thread_arguments[thread_index].process_id = 6;
                thread_arguments[thread_index].thread_id = thread_number;
                pthread_create(&thread_ids[thread_index], NULL, generic_thread, &thread_arguments[thread_index]);
        }
    }
    
    for (int i = 0; i < 6; ++i) {
        pthread_join(thread_ids[i], NULL);
    }
    
    cleanup_process_6_and_7_sync(process_6_and_7_sync);
    
    waitpid(process_8_pid, NULL, 0);
    info(END, 6, 0);
}

void process_5() {
    info(BEGIN, 5, 0);
    
    info(END, 5, 0);
}

struct process_4_synchronization {
    pthread_mutex_t outside_info_guard;
    pthread_mutex_t within_info_guard;
    pthread_cond_t thread_count_increased_cond;
    pthread_cond_t more_threads_may_enter_cond;
    pthread_cond_t thread_11_exited_cond;
    int running_threads_without_info_count;
    int running_threads_with_info_count;
    int MAX_THREAD_COUNT;
    int number_of_threads_that_may_still_terminate; // If we're left with fewer than 5 others, T4.11 can't terminate
};

struct process_4_thread_args {
    int thread_id;
    struct process_4_synchronization* sync;
};

void initialize_process_4_sync(struct process_4_synchronization* sync) {
    pthread_mutex_init(&sync->outside_info_guard, NULL);
    pthread_mutex_init(&sync->within_info_guard, NULL);
    pthread_cond_init(&sync->thread_count_increased_cond, NULL);
    pthread_cond_init(&sync->more_threads_may_enter_cond, NULL);
    pthread_cond_init(&sync->thread_11_exited_cond, NULL);
    sync->running_threads_without_info_count = 0;
    sync->running_threads_with_info_count = 0;
    sync->MAX_THREAD_COUNT = 6;
    sync->number_of_threads_that_may_still_terminate = 35 - 6; // Total threads minus minimum threads needed for T4.11
}

void* process_4_generic_thread(void* process_4_thread_args_as_void) {
    int thread_id = ((struct process_4_thread_args*) process_4_thread_args_as_void)->thread_id;
    struct process_4_synchronization* sync = ((struct process_4_thread_args*) process_4_thread_args_as_void)->sync;
    
    // Wait until there are less than MAX_THREAD_COUNT threads running (regardless of info() calls) to start
    pthread_mutex_lock(&sync->outside_info_guard);
        while (sync->running_threads_without_info_count >= sync->MAX_THREAD_COUNT) {
            pthread_cond_wait(&sync->more_threads_may_enter_cond, &sync->outside_info_guard);
        }
        sync->running_threads_without_info_count++;
    pthread_mutex_unlock(&sync->outside_info_guard);
    
    info(BEGIN, 4, thread_id); // Expensive, must keep outside mutex lock blocks
    
    // Only signal T4.11 AFTER info() has been completed
    pthread_mutex_lock(&sync->within_info_guard);
        sync->running_threads_with_info_count++;
        pthread_cond_signal(&sync->thread_count_increased_cond);
    pthread_mutex_unlock(&sync->within_info_guard);
    
    
    // do_work();
    
    
    // Make sure there are enough threads left so T4.11 can terminate
    pthread_mutex_lock(&sync->within_info_guard);
        while (sync->number_of_threads_that_may_still_terminate == 0) {
            pthread_cond_wait(&sync->thread_11_exited_cond, &sync->within_info_guard);
        }
        sync->number_of_threads_that_may_still_terminate--;
        sync->running_threads_with_info_count--;
    pthread_mutex_unlock(&sync->within_info_guard);
    
    info(END, 4, thread_id); // Expensive, must keep outside mutex lock blocks
    
    pthread_mutex_lock(&sync->outside_info_guard);
        sync->running_threads_without_info_count--;
        pthread_cond_signal(&sync->more_threads_may_enter_cond);
    pthread_mutex_unlock(&sync->outside_info_guard);
    
    return NULL;
}

void* process_4_thread_11(void* process_4_sync_as_void) {
    struct process_4_synchronization* sync = (struct process_4_synchronization*) process_4_sync_as_void;
    
    // Wait until there are less than MAX_THREAD_COUNT threads running properly (post info() call)
    pthread_mutex_lock(&sync->outside_info_guard);
        while (sync->running_threads_without_info_count >= sync->MAX_THREAD_COUNT) {
            pthread_cond_wait(&sync->more_threads_may_enter_cond, &sync->outside_info_guard);
        }
        sync->running_threads_without_info_count++;
    pthread_mutex_unlock(&sync->outside_info_guard);
    
    // This fits best in the locked region, but can safely exist outside of it
    // to accomodate info() being an expensive function
    info(BEGIN, 4, 11);
    
    pthread_mutex_lock(&sync->within_info_guard);
        sync->running_threads_with_info_count++;
    pthread_mutex_unlock(&sync->within_info_guard);
    
    // do_work();
    
    // Wait until there are exactly 6 threads running properly (after info(BEGIN), before info(END))
    pthread_mutex_lock(&sync->within_info_guard);
        while (sync->running_threads_with_info_count < 6) {
            pthread_cond_wait(&sync->thread_count_increased_cond, &sync->within_info_guard);
        }
        info(END, 4, 11);
        sync->running_threads_with_info_count--;
        pthread_mutex_lock(&sync->outside_info_guard);
            sync->running_threads_without_info_count--;
        pthread_mutex_unlock(&sync->outside_info_guard);
        sync->number_of_threads_that_may_still_terminate += 5; // Allow the last threads to exit since T4.11 can't get stuck anymore
        pthread_cond_broadcast(&sync->thread_11_exited_cond);
        pthread_cond_broadcast(&sync->more_threads_may_enter_cond); // 5 signal()s would be more precise, technically
    pthread_mutex_unlock(&sync->within_info_guard);
    
    return NULL;
}

void process_4() {
    info(BEGIN, 4, 0);
    
    pid_t process_7_pid = fork();
    if (0 == process_7_pid) {
        process_7();
        return;
    }
    
    struct process_4_synchronization sync;
    initialize_process_4_sync(&sync);
    
    pthread_t threads[35];
    struct process_4_thread_args thread_arguments[35];
    for (int thread_number = 1; thread_number <= 35; ++thread_number) {
        int thread_index = thread_number - 1;
        
        if (11 == thread_number) {
            pthread_create(&threads[thread_index], NULL, process_4_thread_11, &sync);
        }
        else {
            thread_arguments[thread_index].thread_id = thread_number;
            thread_arguments[thread_index].sync = &sync;
            pthread_create(&threads[thread_index], NULL, process_4_generic_thread, &thread_arguments[thread_index]);
        }
    }
    
    for (int i = 0; i < 35; ++i) {
        pthread_join(threads[i], NULL);
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
    init();

    process_1();
    
    return 0;
}