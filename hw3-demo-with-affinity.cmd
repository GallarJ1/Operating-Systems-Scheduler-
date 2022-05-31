@rem  https://ss64.com/nt/start.html
@rem  1st parameter is the affinity mask as a hexadecimal number
@rem  2nd parameter is the scheduling algorithm
start /B /affinity %1  timethis  hw3-demo.exe  %2  %3 %4 %5 %6 %7 %8 %9
