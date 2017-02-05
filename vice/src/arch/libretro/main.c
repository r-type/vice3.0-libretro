#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "machine.h"

int skel_main(int argc, char *argv[])
{

  main_program(argc, argv);
//  main_exit();
#ifndef NO_LIBCO
  vice_main_exit();
#endif
 
  return(0);
}

void vice_main_exit()
{
printf("vice exit\n");
    machine_shutdown();
}

