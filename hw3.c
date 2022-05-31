/*
*/
#include <windows.h>
#include <stdio.h>


typedef struct processor_data {
   int affinityMask;                /* affinity mask of this processor (just one bit set) */
   PROCESS_INFORMATION processInfo; /* process currently running on this processor */
   int running;                     /* 1 when this processor is running a task, 0 otherwise */
} ProcessorData;

/* function prototypes */
void printError(char* functionName);



int main(int argc, char *argv[])
{
   int processorCount = 0;       /* the number of allocated processors */
   ProcessorData *processorPool; /* an array of ProcessorData structures */
   HANDLE *processHandles;       /* an array of handles to processes */
   DWORD_PTR processAffinityMask;  /* this is a 64 bit data type */
   DWORD_PTR systemAffinityMask;
   
   STARTUPINFO startInfo;
   ZeroMemory(&startInfo, sizeof(startInfo));
   startInfo.cb = sizeof(startInfo);
   
   int processTimes[argc - 1]; //change to int processTimes[argc - 2]; when having schedule_type as parameter

   if (argc < 3)
   {
      fprintf(stderr, "usage, %s  SCHEDULE_TYPE  SECONDS...\n", argv[0]);
      fprintf(stderr, "Where: SCHEDULE_TYPE = 0 means \"first come first serve\"\n");
      fprintf(stderr, "       SCHEDULE_TYPE = 1 means \"shortest job first\"\n");
      fprintf(stderr, "       SCHEDULE_TYPE = 2 means \"longest job first\"\n");
      return 0;
   }
   
   /* read the job duration times off the command-line */
   // Get a command line argument (if it exists).
   if (argc > 1)   //USE THIS WHEN USING SCHEDULE_TYPE AS PARAMETER
   {  
      printf( "The job duration times are:");
      for(int i = 2; i < argc; i++)
      {
         processTimes[i-2] = atoi(argv[i]);
         printf( " %d", processTimes[i-2]);
      }
      
   }
   
   int schedule_Type = atoi(argv[1]);
   
   if(schedule_Type == 1)
   {
      for(int s = 0; s < argc - 3; s++)
      {
         for(int t = s + 1; t < argc - 2; t++)
         {
            if(processTimes[s] > processTimes[t])
            {
               int temp = processTimes[s];
               processTimes[s] = processTimes[t];
               processTimes[t] = temp;
            }
         }
      }
   }
   
   if(schedule_Type == 2)
   {
      for(int s = 0; s < argc - 3; s++)
      {
         for(int t = s + 1; t < argc - 2; t++)
         {
            if(processTimes[s] < processTimes[t])
            {
               int temp = processTimes[s];
               processTimes[s] = processTimes[t];
               processTimes[t] = temp;
            }
         }
      }
   }
   
   int totalNumProcesses = argc - 2;
   int numProcessesRan = 0;
  
   /* get the processor affinity mask for this process */
   GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &systemAffinityMask);
   /* count the number of processors set in the affinity mask */
   printf("\nAffinityMask=%#.2x\n", (int)processAffinityMask);
   int processorbitNum[8];
   
   for(int i = 0; i < 8; i++)
   {
      static int bitMask = 1;
      //printf( "bitMask = %d\n", bitMask);
      if( ( (int) processAffinityMask & bitMask) == bitMask)
      {
         processorCount++;
         static int j = 0;
         processorbitNum[j] = bitMask;
         //printf( "processorbitNum[%d] = %d\n", j, processorbitNum[j]);
         j++;
      }
      bitMask = bitMask << 1;
      //printf( "processorCount = %d\n", processorCount);
   }
   
      
      printf( "processorbitNum[0] = %d\n", processorbitNum[0]);
      printf( "processorbitNum[1] = %d\n", processorbitNum[1]);
      printf( "processorbitNum[2] = %d\n", processorbitNum[2]);
      printf( "processorbitNum[3] = %d\n", processorbitNum[3]);
      printf( "processorbitNum[4] = %d\n", processorbitNum[4]);
      printf( "processorbitNum[5] = %d\n", processorbitNum[5]);
      printf( "processorbitNum[6] = %d\n", processorbitNum[6]);
      printf( "processorbitNum[7] = %d\n", processorbitNum[7]);
   
   /* create, and then initialize, the processor pool data structure */
   processorPool = malloc( processorCount * sizeof(ProcessorData));
   for(int i = 0; i < processorCount; i++)
   {
      processorPool[i].affinityMask = processorbitNum[i];
      processorPool[i].running = 0;
      ZeroMemory( &(processorPool[i].processInfo), sizeof(processorPool[i].processInfo));
   }  
   // "c:\\cs30200\\hw3\\computerprogram_64.exe"
   processHandles = malloc( processorCount * sizeof(HANDLE));
   int handleCount = 0;
   /* start the first group of processes */
   for(int i = 0; i < processorCount; i++)
   {
      //WILL NEED TO CHANGE COMMAND LINE ARGUMENT RELATIVITY WHEN SCHEDULE_TYPE IS PARAMETER
      
      if((numProcessesRan + 2) == argc){
         break;
      }
      printf("Non process ran equal %d %s\n", numProcessesRan,argv[numProcessesRan + 2]);
      if( ! CreateProcess("C:\\Users\\Rocke\\OneDrive\\Desktop\\Hw3\\computeProgram_64.exe", argv[numProcessesRan + 2], NULL, NULL, FALSE, CREATE_NEW_CONSOLE | CREATE_SUSPENDED, NULL, NULL, 
      &startInfo, &(processorPool[i].processInfo)))
      {
        printf("CreateProcess %d failed %d\n", i,GetLastError());
      }
      else
      {
         SetProcessAffinityMask( &(processorPool[i].processInfo), (DWORD) &(processorPool[i].affinityMask));
         ResumeThread(&(processorPool[i].processInfo));
         processorPool[i].running = 1;
         handleCount++;
         numProcessesRan++;
      }   
   }

   /* Repeatedly wait for a process to finish and then,
      if there are more jobs to run, run a new job on
      the processor that just became free. */
   while (1)
   {
      DWORD result;
      int processAffinity[processorCount];

      /* get, from the processor pool, handles to the currently running processes */
      /* put those handles in an array */
      /* use a parallel array to keep track of where in the processor pool each handle came from */
      static int m = 0; 
      for(int i = 0; i < processorCount; i++)
      {
         if(processorPool[i].running == 1)
         {
            processHandles[m] = &(processorPool[i].processInfo);
            processAffinity[m]= processorPool[i].affinityMask;
            printf("ProcessHandles = %p\n", processHandles[m]);
            m++;
         }
      }
      
      /* check that there are still processes running, if not, quit */
      if(handleCount == 0)
      { 
         return 0;
      }

      /* wait for one of the running processes to end */
      printf("Wait handleCount = %d\n", m);
      if (WAIT_FAILED == (result = WaitForMultipleObjects(m, processHandles, FALSE, INFINITE)))
      {
         printf("Last Error= %d result = %d\n", GetLastError(),result);
         printError("WaitForMultipleObjects");
      }

      /* translate result from an index in processHandles[] to an index in processorPool[] */
      processHandles[result] = &processorPool[result];
      
      
      /* close the handles of the finished process and update the processorPool array */
      CloseHandle(processHandles[result]);
      handleCount--;
      processorPool[result].running = 0;
      /* check if there is another process to run on the processor that just became free */
      if(totalNumProcesses == numProcessesRan)
      {
         return 0;
      }
      
       if( !CreateProcess("C:\\Users\\Rocke\\OneDrive\\Desktop\\Hw3\\computeProgram_64.exe ", argv[numProcessesRan + 1], NULL, NULL, FALSE, CREATE_NEW_CONSOLE | CREATE_SUSPENDED, NULL, NULL, 
      &startInfo, &(processorPool[result].processInfo)))
      {
         printf("CreateProcess %d failed", result);
      }
      else
      {
         SetProcessAffinityMask( &(processorPool[result].processInfo), (DWORD) &(processorPool[result].affinityMask));
         ResumeThread(&(processorPool[result].processInfo));
         processorPool[result].running = 1;
         handleCount++;
         numProcessesRan++;
      }
      
      


   } 
   return 0;
}









/****************************************************************
   The following function can be used to print out "meaningful"
   error messages. If you call a Windows function and it returns
   with an error condition, then call this function right away and
   pass it a string containing the name of the Windows function that
   failed. This function will print out a reasonable text message
   explaining the error.
*/
void printError(char* functionName)
{
   LPVOID lpMsgBuf;
   int error_no;
   error_no = GetLastError();
   FormatMessage(
         FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
         NULL,
         error_no,
         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* default language */
         (LPTSTR) &lpMsgBuf,
         0,
         NULL
   );
   /* Display the string. */
   fprintf(stderr, "\n%s failed on error %d: ", functionName, error_no);
   fprintf(stderr, (const char*)lpMsgBuf);
   /* Free the buffer. */
   LocalFree( lpMsgBuf );
}

