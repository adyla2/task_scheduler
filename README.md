README:
This program executes a task every N seconds. Using a priority queue it checks
the appropriate runtime for each task. If the time now is greater than or equal to
the runtime, it executes the task.

Currently: I am segmentation faulting, and have spent many hours trying to fix it. I'm
thinking there is a race condition I am not considering? Feedback would be very
much appreciated.

System: Ubuntu
Notes: Initially I developed this in a MAC OSX environment, and while it does
run for a few iterations, eventually this too seg faults.
I ran the project in a Docker container to test in the Ubuntu environment,
in which the segmentation fault occurs immediately.

Still missing: task2 and cancelling task function.

How to run:
  1. make run

  Any additional attempts:
  1. make clean
  2. make run
# task_scheduler
