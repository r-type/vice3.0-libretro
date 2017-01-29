#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "machine.h"

int skel_main(int argc, char *argv[])
{

  main_program(argc, argv);
  main_exit();

 
  return(0);
}

void main_exit()
{
    machine_shutdown();
}

