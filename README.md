# ConcurrentContainers

Open Source library that contains of thread-safe (both read and write) containers.
There are implementation options for both lock-based and lock-free approaches.
Tested on VS2015, VS2017.

Contents:
lock_based/queue.h - lock-based queue that internally depends on std::queue
lock_based/queue_improved.h - lock-based queue that internally depends on hand-written single-linked list. This version is a bit advanced as long as it has more fine-grained locks


# How To Use
Just include necessary header to you C++ project.