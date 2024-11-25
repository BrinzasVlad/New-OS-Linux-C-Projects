#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdbool.h>
#include "a2_helper.h"

struct os_assig_2_thread_data {
    int process_id;
    int thread_id;
};

void* generic_thread(void* thread_info_as_void) {
    struct os_assig_2_thread_data data = *(struct os_assig_2_thread_data*)thread_info_as_void;
    
    info(BEGIN, data.process_id, data.thread_id);
    
    info(END, data.process_id, data.thread_id);
    
    return NULL;
}

void process_8() {
    info(BEGIN, 8, 0);
    
    info(END, 8, 0);
}

struct process_7_synchronization {
    pthread_mutex_t guard;
    pthread_cond_t thread_5_started_cond;
    bool thread_5_started;
    pthread_cond_t thread_2_exited_cond;
    bool thread_2_exited;
};

void* process_7_thread_2(void* process_7_sync_as_void) {
    struct process_7_synchronization *process_7_sync = (struct process_7_synchronization*)process_7_sync_as_void;
    
    // Wait for T7.5 to start
    pthread_mutex_lock(&process_7_sync->guard);
        while (!process_7_sync->thread_5_started) {
            pthread_cond_wait(&process_7_sync->thread_5_started_cond, &process_7_sync->guard);
        }
    pthread_mutex_unlock(&process_7_sync->guard);
    
    info(BEGIN, 7, 2);
    
    // Terminate and notify T7.5
    pthread_mutex_lock(&process_7_sync->guard);
        info(END, 7, 2);
        process_7_sync->thread_2_exited = true;
        pthread_cond_signal(&process_7_sync->thread_2_exited_cond);
    pthread_mutex_unlock(&process_7_sync->guard);
    
    return NULL;
}

void* process_7_thread_5(void* process_7_sync_as_void) {
    struct process_7_synchronization* process_7_sync = (struct process_7_synchronization*)process_7_sync_as_void;
    
    // Start and notify T7.2
    pthread_mutex_lock(&process_7_sync->guard);
        info(BEGIN, 7, 5);
        process_7_sync->thread_5_started = true;
        pthread_cond_signal(&process_7_sync->thread_5_started_cond);
    pthread_mutex_unlock(&process_7_sync->guard);
    
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
    pthread_mutex_init(&process_7_sync.guard, NULL);
    pthread_cond_init(&process_7_sync.thread_5_started_cond, NULL);
    process_7_sync.thread_5_started = false;
    pthread_cond_init(&process_7_sync.thread_2_exited_cond, NULL);
    process_7_sync.thread_2_exited = false;
    
    pthread_t threads[5];
    struct os_assig_2_thread_data thread_arguments[5];
    // The above is a bit inefficient, since T7.2 and T7.5 don't use theirs
    // But the space waste is minor enough to ignore (and might be optimized out anyway)
    for (int thread_number = 1; thread_number <= 5; ++thread_number) {
        int thread_index = thread_number - 1;
        switch (thread_number) {
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
    
    info(END, 7, 0);
}

void process_6() {
    info(BEGIN, 6, 0);
    
    pid_t process_8_pid = fork();
    if (0 == process_8_pid) {
        process_8();
        return;
    }
    
    pthread_t thread_ids[6];
    struct os_assig_2_thread_data thread_arguments[6];
    for (int i = 0; i < 6; ++i) {
        thread_arguments[i].process_id = 6;
        thread_arguments[i].thread_id = i + 1;
        pthread_create(&thread_ids[i], NULL, generic_thread, &thread_arguments[i]);
    }
    
    for (int i = 0; i < 6; ++i) {
        pthread_join(thread_ids[i], NULL);
    }
    
    waitpid(process_8_pid, NULL, 0);
    info(END, 6, 0);
}

void process_5() {
    info(BEGIN, 5, 0);
    
    info(END, 5, 0);
}

struct process_4_synchronization {
    pthread_mutex_t guard;
    pthread_cond_t thread_count_increased_cond;
    pthread_cond_t more_threads_may_enter_cond;
    pthread_cond_t thread_11_exited_cond;
    int running_threads_count;
    int MAX_THREAD_COUNT;
    int no_threads_that_may_still_terminate; // If we're left with fewer than 5 others, T4.11 can't terminate
};

struct process_4_thread_args {
    int thread_id;
    struct process_4_synchronization* sync;
};

void* process_4_generic_thread(void* process_4_thread_args_as_void) {
    int thread_id = ((struct process_4_thread_args*) process_4_thread_args_as_void)->thread_id;
    struct process_4_synchronization* sync = ((struct process_4_thread_args*) process_4_thread_args_as_void)->sync;
    
    // Wait until there are less than MAX_THREAD_COUNT threads running to start
    pthread_mutex_lock(&sync->guard);
        while (sync->running_threads_count >= sync->MAX_THREAD_COUNT) {
            pthread_cond_wait(&sync->more_threads_may_enter_cond, &sync->guard);
        }
        sync->running_threads_count++;
    pthread_mutex_unlock(&sync->guard);

    // Normally info() should be a lightweight function that would
    // fit most logically INSIDE the locked region, to prevent desyncs.
    // But because this testing implementation actually has info()
    // perform work (simulated via usleep()), we use this 'hack'
    // of putting it between locked regions.
    // This, however, introduces bugs:
    // - if incrementing the thread count happens BEFORE info(), then
    //   it's possible for T4.11 to exit when it shouldn't.
    // - if incrementing the thread count happens AFTER info(), then
    //   it's possible for more threads to run at once than correct.
    info(BEGIN, 4, thread_id);
    pthread_cond_signal(&sync->thread_count_increased_cond);
    
    // Exit, but make sure there are enough threads left so T4.11 can terminate
    pthread_mutex_lock(&sync->guard);
        while (sync->no_threads_that_may_still_terminate == 0) {
            pthread_cond_wait(&sync->thread_11_exited_cond, &sync->guard);
        }
        info(END, 4, thread_id);
        sync->running_threads_count--;
        sync->no_threads_that_may_still_terminate--;
        pthread_cond_signal(&sync->more_threads_may_enter_cond);
    pthread_mutex_unlock(&sync->guard);
    
    return NULL;
}

void* process_4_thread_11(void* process_4_sync_as_void) {
    struct process_4_synchronization* sync = (struct process_4_synchronization*) process_4_sync_as_void;
    
    // Wait until there are less than MAX_THREAD_COUNT threads running to start
    pthread_mutex_lock(&sync->guard);
        while (sync->running_threads_count >= sync->MAX_THREAD_COUNT) {
            pthread_cond_wait(&sync->more_threads_may_enter_cond, &sync->guard);
        }
        sync->running_threads_count++;
    pthread_mutex_unlock(&sync->guard);
    
    // This fits best in the locked region, but can safely exist outside of it
    // to accomodate info() being an expensive function
    info(BEGIN, 4, 11);
    
    // Wait until there are exactly 6 threads running to exit
    pthread_mutex_lock(&sync->guard);
        while (sync->running_threads_count < 6) {
            pthread_cond_wait(&sync->thread_count_increased_cond, &sync->guard);
        }
        info(END, 4, 11);
        sync->running_threads_count--;
        sync->no_threads_that_may_still_terminate += 5; // Allow the last threads to exit since T4.11 can't get stuck anymore
        pthread_cond_broadcast(&sync->thread_11_exited_cond);
        pthread_cond_broadcast(&sync->more_threads_may_enter_cond); // 5 signal()s would be more precise, technically
    pthread_mutex_unlock(&sync->guard);
    
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
    pthread_mutex_init(&sync.guard, NULL);
    pthread_cond_init(&sync.thread_count_increased_cond, NULL);
    pthread_cond_init(&sync.more_threads_may_enter_cond, NULL);
    pthread_cond_init(&sync.thread_11_exited_cond, NULL);
    sync.running_threads_count = 0;
    sync.MAX_THREAD_COUNT = 6;
    sync.no_threads_that_may_still_terminate = 35 - 6; // Total threads minus minimum threads needed for T4.11
    
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