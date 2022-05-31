# Operating-Systems-Scheduler-
The scheduler for this assignment makes the following assumptions.

We know the running time of every process.
All the processes arrive at the same time.
The scheduler cannot interrupt any process.
We are scheduling multiple resources (multiple CPUs).
Each process uses only one resource, a CPU.
We will be interested in two metrics,

average turnaround time,
total completion time.
The scheduler will implement three scheduling algorithms.

FCFS
SJF
LJF
In the zip file there is a program called computeProgram_64.exe that takes one command-line argument, a positive integer, and then computes for that many seconds of CPU time (not wall-clock-time). The program keeps track of exactly how much CPU runtime it has accumulated and it stops when it has accumulated exactly the given number of CPU seconds. The program does not count time when it is in the "ready state" and is not running on a CPU (try running this program a few times). For this assignment, instances of the computeProgram_64.exe program will represent tasks that you are to schedule on a group of CPU's.

In the zip file there is a file hw3.c that outlines the code that you need to write to implement the scheduler. I'll describe the scheduler in two steps. First, a slightly simplified version of the scheduler (that only implements FCFS), and then the full scheduler.

The slightly simplified version of the scheduler has the following command-line syntax.

    hw3  SECONDS...
The notation SECONDS... represents a list of positive integer values. The integer values are execution times for instances of the program computeProgram_64.exe. For example, consider running the scheduler with this command-line.

    Z:\hw3>hw3  6 12 4 5 10
When the scheduler program starts up, it should use the GetProcessAffinityMask() function to ask the operating system which processors are being made available to it. Suppose the operating system makes three CPU's available to the scheduler program and they are CPU's 0, 3 and 5. The scheduler should then use the CreateProcess() function to run three instances of computeProgram_64.exe, a 6 second instance on CPU 0, a 12 second instance on CPU 3 and a 4 second instance on CPU 5 (the scheduler should use the SetProcessAffinityMask() function to tell the operating system to assign each process to a specific CPU; the details of this step are given in a paragraph below). The scheduler program should then use the WaitForMultipleObjects() function to wait for one of the running processes to finish (the details of this step are also given in a paragraph below). When the 4 second instance on CPU 5 finishes, the scheduler should run a 5 second instance on CPU 5 and then wait again, using WaitForMultipleObjects(), for one of its child processes to finish. When the 6 second instance finishes on CPU 0, the scheduler should run a 10 second instance on CPU 0. Since there are no more new instances to run, the scheduler should wait, again using WaitForMultipleObjects(), for each of the currently running processes to finish.

Here are further details about the above steps.

Your program only needs to worry about a total of 16 CPU's (the GetProcessAffinityMask() function allows up to 64 CPU's; you only need to worry about CPU's numbered 0 to 15).

Before you can call GetProcessAffinityMask(), you need to call the GetCurrentProcess() function which gives your scheduler process a handle to itself. You need to pass this handle to GetProcessAffinityMask() so that the operating system knows which process you want the affinity mask of.

The GetProcessAffinityMask() function is a good example of a typical C function. It has one "input parameter" and two "output parameters". (The function needs "output parameters" because the function's real output, its return value, is just a boolean "success or failure" result.) Notice that the "output parameters" are pointers. So an output parameter is really an input value that points to a location where the function should place its "output value". Here is an outline of how you use these "output parameters".

     DWORD_PTR processAffinityMask;   // a memory location that can hold a value
     DWORD_PTR  systemAffinityMask;   // a memory location that can hold a value
     GetProcessAffinityMask(myProcess, &processAffinityMask, &systemAffinityMask);
The "address of" operator, &, creates a pointer to the variable processAffinityMask and the GetProcessAffinityMask() function uses that pointer to find the memory location where it should place the function's result. (You don't need the systemAffinityMask result, but you still need to provide a memory location to hold it.)

The GetProcessAffinityMask() and SetProcessAffinityMask() functions use "bitmask" values and you manipulate these bitmasks using "bitwise operations". A bitmask is an integer value where what is important is the individual bits in the number that are set to 1 (rather than the total value of the integer). So, for example, GetProcessAffinityMask() might return the bitmask 00000000000000000000000010011001, which represents four CPU's, CPU number 0, CPU number 3, CPU number 4, and CPU number 7 (bits are read from right to left, that is, from the least-significant-bit to the most-significant-bit). When you get back the result from GetProcessAffinityMask() you need to calculate which bits are set in the mask. And when you call SetProcessAffinityMask() you need to make sure that you have exactly one bit set in the bitmask, the bit that represents the CPU that you are assigning a process to. (When you need to find the bits set in the process affinity mask, you may find it useful to use the C bitwise operators & and <<.)

For bookkeeping reasons that we will get to shortly, you need a data structure that keeps track of what is going on with each of the processors your scheduler can use. In the hw3.c file, I give you an example of such a data structure.

   typedef struct processor_data {
      int affinityMask;                /* the affinity mask of this processor (just one bit set) */
      PROCESS_INFORMATION processInfo; /* process currently running on this processor */
      int running;                     /* 1 when this processor is running a task, 0 otherwise */
   } ProcessorData;
This struct keeps track of just one processor. To keep track of all the processors your scheduler can use, you can use an array of these structs. You can use the process affinity mask returned by GetProcessAffinityMask() to help you create and initialize such an array. Here is an outline of the steps.

      int processorCount = 0; /* the number of allocated processors */
      /* Declare an array of ProcessorData structures */
      ProcessorData *processorPool;
      ...
      /* Call GetProcessAffinityMask() */
      ...
      /* Use the affinity mask to compute processorCount */
      ...
      /* Create the array of ProcessorData structures */
      processorPool = malloc(processorCount * sizeof(ProcessorData));
      /* Initialize the array of ProcessorData structures
         Set each affinityMask to the appropriate CPU
         Set each running to 0 */
      ...
      /* start the first group of processes */
      ...
It is now time to start launching processes.

Since initially there are no processors running processes, the scheduler should initially launch as many processes as there are available processors (or, until there are no more processes to run).

Let us look at the details of launching a single process on a specific processor.

When you call CreateProcess(), normally the operating system will immediately start up the new process. But the operating system will start the process on whichever CPU it wants to, not the specific CPU that you want to assign the process to. In order to get a chance to call SetProcessAffinityMask() for the new process, you need to tell CreateProcess() to create the new process in the "suspended state". You create a process in the suspended state by using the CREATE_SUSPENDED Process Creation Flag in CreateProcess' dwCreationFlags parameter. (The dwCreationFlags parameter is another good example of a bitmask. Each process creation flag is just one bit. You combine creation flags by bitwise or'ing the bits together. For example, the bitmask CREATE_NEW_CONSOLE|CREATE_SUSPENDED|DEBUG_PROCESS has three bits set.) After the new process has been created suspended, you can use the new process's handle to call SetProcessAffinityMask() to assign the process to a specific processor (you get the correct affinity mask by looking at the affinityMask field in this processor's entry in the processorPool array). After that, you need to actually start the process running by calling the ResumeThread() function. You give ResumeThread() a handle to the suspended process's primary thread. You get that handle to the primary thread from the suspended process's PROCESS_INFORMATION data structure which should be the processInfo field of this processor's entry in the processorPool array. This PROCESS_INFORMATION data structure (in the processorPool array) should be the pointee of the lpProcessInformation parameter in the call to CreateProcess() so that it will be filled in by the call to CreateProcess(). (Notice that this is the same data structure in which you find the handle to the new process itself.) Once the new process is running, you need to update the running field in this processor's entry in the processorPool array.

Once all the processors are busy with their initial jobs, the scheduler should go into a loop where it waits for a process to end and then launches a new process. The scheduler should stay in this loop until there are no new processes to launch and there are no processes still running on any processor.

Let us look at the details of waiting for one of the processes to end.

After the scheduler has started a new process, the scheduler needs to wait for some process to finish, which frees up a CPU so that the scheduler can start another process. The scheduler does not know which process may be the next one to finish, so the scheduler needs to call an operating system function that waits on the scheduler's set of currently running processes and figures out which of those process finishes first. We have the operating system wait on a set of objects by passing to the WaitForMultipleObjects() function an array of handles to the objects we are waiting on. Remember that each time we call CreateProcess() it returns a handle to the newly created process. This handle is stored by CreateProcess() in a PROCESS_INFORMATION data structure that we keep as a field in our ProcessorData structures in our processorPool array. It is an array of these process handles that we pass to WaitForMultipleObjects(). (Notice that this array of process handles NEED NOT be as long as the processorPool array. Why is that?) When WaitForMultipleObjects() returns, a process has finished, and the return value of this function is the index, in your array of process handles, of the finished process (the return value is not the handle, it is an index into your array of handles). When you get this index from WaitForMultipleObjects(), you need to use it for several purposes. First, use the index to get the appropriate process handle and close it. Then, you need to use this index to determine which processor has become free and mark that processor as inactive (by setting its running field to zero). But this is the kind of information that we are storing in entries of the processorPool array. So what you need to do with the index returned by WaitForMultipleObjects() is translate it into an appropriate index into the processorPool array. How to do that?

Just before you call WaitForMultipleObjects(), you need to create and initialize the array of handles to the running processes. At the same time, simultaneously create a parallel array (a cute example) of indexes into the processorPool that keeps track of where in the processorPool each handle came from. Then the index returned by WaitForMultipleObjects() can be used in either of the two parallel arrays to get all the needed information.

Here are some test cases for your basic scheduler.

Start a cmd.exe shell and run your program with the following command line.

    Z:\hw3>hw3  30
You should see computeProgram_64.exe run for 30 seconds and NOT change its CPU number for the whole time.

Use the Windows Task Manager to set the processor affinity of the cmd.exe program to a single CPU (it doesn't matter which one). Run your program with the following command line.

    Z:\hw3>hw3  30
You should see an instance of computeProgram_64.exe run on the CPU that you assigned cmd.exe to. Change the single CPU that cmd.exe is assigned to and run the above command line again. An instance of computeProgram_64.exe should run the whole time on the new CPU. Now use the following command.

    Z:\hw3>hw3  10 10
You should see one instance of computeProgram_64.exe run for 10 seconds on the assigned CPU, followed by a second instance of computeProgram_64.exe that runs for 10 seconds on the same CPU.

Use Task Manager to set the processor affinity of the cmd.exe program to two CPU's and then run your program with the following command.

    Z:\hw3>hw3  30 30
You should see two instances of computeProgram_64.exe, one running on each of the two CPU's that you assigned cmd.exe to, and the two processes should NOT move between the two CPU's.

Use the Windows Task Manager to make sure that the cmd.exe shell is assigned to only two processors. Run your program three times using the following three command lines.

    Z:\hw3>timethis hw3  5 5 5 25 5
    Z:\hw3>timethis hw3  5 5 5 5 25
    Z:\hw3>timethis hw3  25 5 5 5 5
You should get total completion times of (roughly) 30 seconds, 35 seconds, and 25 seconds. (Make sure you understand why those are the correct runtimes for the above three commands.)

Once you have the basic scheduler written and working, you can add a simple feature to it that makes experimenting with the scheduler easy. The new feature is to have the scheduler implement three different scheduling algorithms, first come first serve (FCFS), shortest job first (SJF), and longest job first (LJF). The new version of the scheduler has the following command-line syntax.

    hw3  SCHEDULE_TYPE  SECONDS...

    Where: SCHEDULE_TYPE = 0 means "first come first serve"
           SCHEDULE_TYPE = 1 means "shortest job first"
           SCHEDULE_TYPE = 2 means "longest job first"
The basic scheduler is actually FCFS. It schedules the jobs in the order that they are given on the command-line. Adding SJF and LJF to the scheduler is very easy. When the scheduler starts up, the first thing it should do is get the value of SCHEDULE_TYPE, then read into an array the (integer) values of the job duration times. Then, depending on the value of SCHEDULE_TYPE, the scheduler should either sort the array of job times in descending order (SCHEDULE_TYPE = 2, or LJF), sort the array of job times in ascending order (SCHEDULE_TYPE = 1, or SJF), or leave the array unsorted (SCHEDULE_TYPE = 0, or FCFS). After the array of job times has been processed, everything else in the scheduler is the same.

To test your program, start a cmd.exe shell and use the Windows Task Manager to give the shell only two processors. Run your program with the following command lines.

    Z:\hw3>timethis hw3  0  5 5 5 25 5
    Z:\hw3>timethis hw3  1  5 5 5 25 5
    Z:\hw3>timethis hw3  2  5 5 5 25 5
You should get total completion times of (roughly) 30 seconds, 35 seconds, and 25 seconds.

Here is another test case for your program. Run your program on two processors with the following command lines.

    Z:\hw3>timethis hw3  0  30 5 20 10 15
    Z:\hw3>timethis hw3  1  30 5 20 10 15
    Z:\hw3>timethis hw3  2  30 5 20 10 15
You should get total completion times of (roughly) 45 seconds, 50 seconds, and 40 seconds. Then use Task Manager to change the number of processors given to cmd to three. Then the above three command lines should have completion times of (roughly) 30 seconds, 40 seconds, and 30 seconds.

You should come up with other tests of your program. Make sure it works in a variety of situations.

In all of the above command-lines, I assumed that you used Task Manager to set the processor affinity of the cmd.exe process before entering each command-line. It is possible to set the processor affinity at the command-line without having to use Task Manager, but the command-line is a bit awkward. Here is an example.

    Z:\hw3>start /B /affinity 23  timethis hw3  0  30 5 20 10 15
In this command-line, the number after /affinity is a hexadecimal processor affinity mask. So this command runs the scheduler (using FCFS) with five jobs on three processors, which are processors 0, 1, and 5 (make sure you understand why those are the correct processor numbers). Here is another example.

    Z:\hw3>start /B /affinity 121  timethis hw3  1  30 5 20 10 15
This command runs the scheduler (using SJF) with five jobs on three processors, which are processors 0, 5, and 8. (Here is some information about the start command.)

In the zip file there is a demonstration version of the assignment that you can experiment with. Your program should produce information output exactly like that of the demo program.
