#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include "dispatcher_exec.hpp"
#include "checkpoint.hpp"
#include "cyclone_comm.hpp"
#include "tuning.hpp"
#if defined(DPDK_STACK)
#include <rte_launch.h>
#endif

boost::asio::io_service ioService2;
boost::asio::io_service::work work2(ioService2);

static runq_t<rpc_info_t> issue_q;

ticket_t ticket_window;

extern int dpdk_executor(void *arg);

boost::thread_group threadpool;
boost::thread *executor_thread;

void dispatcher_exec_startup()
{
  ticket_window.go_ticket   = 0;
  issue_q.ticket            = 0;
  for(int i=0;i < executor_threads;i++) {
#if defined(DPDK_STACK)
    int e = rte_eal_remote_launch(dpdk_executor, (void *)i, 3 + i);
    if(e != 0) {
      BOOST_LOG_TRIVIAL(fatal) << "Failed to launch executor on remote lcore";
      exit(-1);
    }
#else
    executor_thread = new boost::thread(boost::ref(executor));
    threadpool.create_thread
      (boost::bind(&boost::asio::io_service::run, &ioService2));
#endif
  }
}

void exec_rpc(rpc_info_t *rpc)
{
  issue_q.add_to_runqueue(rpc);
}

void exec_send_checkpoint(void *socket, void *handle)
{
  ioService2.post(boost::bind(send_checkpoint, socket, handle));
}


