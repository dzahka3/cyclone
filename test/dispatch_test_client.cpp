// Dispatcher test client
#include "../core/clock.hpp"
#include "../core/logging.hpp"
#include <libcyclone.hpp>
#include <stdio.h>

static void print(const char *prefix,
		  const void *data,
		  const int size)
{
  unsigned long elapsed_usecs = rtc_clock::current_time();
  char *buf = (char *)data;
  if(size != 16) {
    BOOST_LOG_TRIVIAL(fatal) << "CLIENT: Incorrect record size";
    exit(-1);
  }
  else {
    BOOST_LOG_TRIVIAL(info)
      << prefix << " "
      << *(const unsigned int *)buf << " "
      << *(const unsigned int *)(buf + 4) << " " 
      << (elapsed_usecs - *(const unsigned long *)((char *)buf + 8));
  }
}

int main(int argc, char *argv[])
{
  if(argc != 4) {
    printf("Usage: %s client_id replicas clients\n", argv[0]);
    exit(-1);
  }
  int me = atoi(argv[1]);
  int replicas = atoi(argv[2]);
  int clients  = atoi(argv[3]);
  void * handle = cyclone_client_init(me,
				      me,
				      replicas,
				      "cyclone_test.ini",
				      "cyclone_test.ini");
  char *proposal = new char[CLIENT_MAXPAYLOAD];
  int ctr = get_last_txid(handle) + 1;
  while(true) {
    *(unsigned int *)&proposal[0] = me;
    *(unsigned int *)&proposal[4] = ctr;
    *(unsigned long *)&proposal[8] = rtc_clock::current_time();
    void *resp;
    print("PROPOSE", proposal, 16);
    int sz = make_rpc(handle, proposal, 16, &resp, ctr, 0);
    if(sz != 16 || memcmp(proposal, resp, 16) != 0) {
      print("ERROR", proposal, 16);
    }
    else {
      print("ACCEPTED", proposal, 16);
    }
    ctr++;
  }
}
