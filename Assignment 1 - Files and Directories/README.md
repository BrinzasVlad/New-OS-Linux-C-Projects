# Assignment 1 - File System Module
This folder contains the task requirements, tester, and solution code for the first assignment.

## Task Requirements
The complete task requirements are specified in a1_en.pdf.\
In summary, the task is to write a program that, depending on the command line arguments it receives, executes one of multiple options such as filtering and listing directory contents, verifying that a given file fits a specific format, or extracting and printing a specific line from a such file.

## Running in Docker
In order to see the tester run in a Docker container, first build an image with
```
docker build --tag os-assig-1 --file a1_dockerfile .
```
then run the image with
```
docker run --rm os-assig-1
```
(Note that it may take a moment for the tester to start, since it needs to generate some test files first.)