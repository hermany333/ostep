#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  printf(1, "==== mprotect() Test START ====\n");
  char *p = sbrk(0x1000); 



  if(argc > 1){
    exit();
  }
  
  // Should work 
  *p = 'a';

  if(mprotect((void *) p, 1) < 0){
    printf(1, "mprotect: failed\n");
    exit();
  } 

  // Should not work give an error
  // *p = 'a';

  if(munprotect((void *) p, 1) < 0){
    printf(1, "munprotect: failed\n");
    exit();
  }

  *p = 'a';

  printf(1, "%c\n", *p);
  exit();
}
