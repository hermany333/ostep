#include "types.h"
#include "pstat.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  struct pstat ps;
  struct pstat *p = &ps;

  if(getpinfo(p) < 0) {
    printf(1, "ps: ex failed\n");
    exit();
  }

  for(int i = 0; i < NPROC; i++){
    if(!p->inuse[i])
      continue;

    printf(1,"PID:%d  Tickets:%d  Ticks:%d\n", p->pid[i], p->tickets[i], p->ticks[i]);
  }

  exit();
}
