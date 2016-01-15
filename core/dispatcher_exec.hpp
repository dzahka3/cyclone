#ifndef _DISPATCHER_EXEC_
#define _DISPATCHER_EXEC_
#include "cyclone.hpp"
#include "clock.hpp"

typedef struct rpc_info_st {
  rpc_t *rpc;
  int len;
  int sz;
  void *ret_value;
  volatile unsigned long follower_data_lock;
  void *follower_data;
  int follower_data_size;
  bool have_follower_data;
  char * volatile req_follower_data;
  volatile int req_follower_data_size;
  volatile int req_follower_term;
  volatile bool req_follower_data_active;
  volatile bool rep_success;
  volatile bool rep_failed;
  volatile bool rep_follower_success;
  volatile bool complete;
  struct rpc_info_st *next;
} rpc_info_t;


extern void dispatcher_exec_startup();
extern void exec_rpc(rpc_info_t *rpc);
extern void exec_rpc_internal(rpc_info_t *rpc);
extern void exec_rpc_internal_synchronous(rpc_info_t *rpc);
extern void exec_rpc_internal_ro(rpc_info_t *rpc);
#endif
