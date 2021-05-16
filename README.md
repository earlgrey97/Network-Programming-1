# Assignment 1

- Student ID: 20160233
- Your Name: ParkNaHyeon

## Ethics Oath
I pledge that I have followed all the ethical rules required by this course (e.g., not browsing the code from a disallowed source, not sharing my own code with others, so on) while carrying out this assignment, and that I will not distribute my code to anyone related to this course even after submission of this assignment. I will assume full responsibility if any violation is found later.

Name: ParkNaHyeon 
Date: 2020.09.27

## Brief description
Briefly describe here how you implemented the project.

client.c
It makes socket, connects to the server and give corresponding command to the server for file transfer.

client_multi.c
It makes socket and connects to the server. Here, since it needs to handle multiple requests, it forks according to number of multiple requests.

server.c
It serves the request of client. However for synchronization problem when sending multiple packets(for long data), I used sleep function for each iteration when receiving to handle sync problem. It may have quite bad time efficiency but in this assignment, I implemented it this way.

## Misc
Describe here whatever help (if any) you received from others while doing the assignment.

I did it on my own.
