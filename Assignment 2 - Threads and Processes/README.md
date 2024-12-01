# Assignment 2 - Threads and Processes
This folder contains the task requirements, tester, and solution code for the second assignment.

## Task Requirements
The complete task requirements are specified in a2_en.pdf.\
In summary, the task is to write a program that creates a given process hierarchy, with some of these processes then creating sub-threads, then coordinate the execution of these processes and threads such that they meet certain timing constraints.

## Running in Docker
In order to see the tester run in a Docker container, first build an image with
```
docker build --tag os-assig-2 --file a2_dockerfile .
```
then run the image with
```
docker run --rm os-assig-2
```

### Extra Considerations
In the process of writing the code for this assignment, a few considerations have arisen that I'd like to at least mention somewhere:
- The way the testing is implemented, the tester communicates with my code inside the `info()` function, triggering a `usleep()` instruction of longer or shorter duration. This is somewhat unnatural: it's meant to simulate the thread needing to do some work in order to enforce proper implementation of parallelism, but it'd have made more sense for the `info()` function to be lightweight and there to exist a second function, e.g. `do_work()` for the actual heavy workload. Or, alternatively, for the `info()` function to be renamed so as to make it clear that it is processing-expensive and hence the implementation should be rearranged so that `info()` is never called inside a mutex-locked region, for instance.
- Normally it'd be nice to handle potential errors when creating processes or threads, e.g. by freeing up some memory then retrying, or by solving the task some other way, or by making a note that the system offers limited functionality and restricting the activity of our app. However, this assignment has a pretty strict process / thread hierarchy and there isn't much we can try to change if an error does occur; as such, I didn't really know what I could do if errors were to happen. A similar (though less solid) case can be made for the creation and operation of POSIX synchronization tools like mutexes and conditionals.
- When calling `pthread_cond_wait()`, I run the operation inside a `while` loop (instead of simply using an `if` statement). This has two purposes: on one hand, it guards against spurious wakes (i.e. the thread being woken even though the condition is not yet met) and on the other hand it avoids the wait entirely if the condition is already fulfilled so as to not accidentally lock the thread forever.
- When passing mutexes / conditional variables to a thread, one must be very careful about how they are passed. Passing around a pointer to one is safe, but passing them by value is very much not safe, and the same applies to making copies of them via assignment to a local variable. (In summary: do not copy a mutex, only pass references.)
- When launching a thread via pthread_create(), I often need to pass it an argument, such as its thread id. It may seem tempting to use a single variable for this (e.g. pass the thread a pointer to the for-loop variable), but this is not safe: it's possible for the parent thread to modify this variable before the newly-created thread reads it. My chosen solution for this was to simply create a separate variable (or have an array of them) for each argument, such that it never needs to be modified and the thread can take its time reading it. If memory were more of a concern, we could opt to dynamically allocate these variables, such that the thread could free() them after having read the information.