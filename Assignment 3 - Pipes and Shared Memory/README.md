# Assignment 3 - Inter-Process Communication Mechanisms
This folder contains the task requirements, tester, and solution code for the third assignment.

## Task Requirements
The complete task requirements are specified in a3_en.pdf.\
In summary, the task is to write a program that communicates with the tester via named pipe and performs a series of operations such as managing shared memory, memory-mapping files, or reading specific segmens of a file into shared memory.

## Running in Docker
In order to see the tester run in a Docker container, first build an image with
```
docker build --tag os-assig-3 --file a3_dockerfile .
```
then run the image with
```
docker run --rm os-assig-3
```
(Note that it may take a moment for the tester to start, since it needs to generate some test files first.)

If you'd instead prefer to run some more specific tests (e.g. to use the `--test` and `--verbose` options on the tester), you can run
```
docker run --rm -it os-assig-3 sh
```
to run a shell inside the docker container. From there, you can invoke the tester manually or run any other desired commands. You can then `exit` when done.