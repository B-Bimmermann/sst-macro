/**
Copyright 2009-2017 National Technology and Engineering Solutions of Sandia, 
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S.  Government 
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly 
owned subsidiary of Honeywell International, Inc., for the U.S. Department of 
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2017, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of Sandia Corporation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Questions? Contact sst-macro-help@sandia.gov
*/

#include <sumi/transport.h>
#include <sumi/thread_safe_set.h>

namespace sumi {

class fake_transport : public transport
{
  FactoryRegister("fake", transport, fake_transport,
               "A fake transport that doesn't actually send messages - just logs them")
 public:
  fake_transport(sprockit::sim_parameters* params);

  message* block_until_message();

  message* block_until_message(double timeout){
    return block_until_message();
  }

  collective_done_message* collective_block(collective::type_t ty, int tag);

  void cq_notify();

  void schedule_ping_timeout(pinger *pnger, double to);

  void schedule_next_heartbeat();

  double wall_time() const {
    return 0;
  }

  message* pop_rdma_get(){
    return pop_message(rdma_gets_);
  }

  void start_heartbeat(double interval){} //do nothing

  void stop_heartbeat(){} //do nothing

  message* pop_rdma_put(){
    return pop_message(rdma_puts_);
  }

  message* pop_send(){
    return pop_message(sends_);
  }

  message* pop_nvram_get(){
    return pop_message(nvram_gets_);
  }

  void simulate_vote(int context, const thread_safe_set<int>& failures);

  void allgather(void *dst, void *src, int nelems, int type_size, int tag,
                 bool fault_aware, int context, communicator *dom){} //do nothing

  void allreduce(void *dst, void *src, int nelems, int type_size, int tag,
                 reduce_fxn fxn, bool fault_aware, int context, communicator *dom){} //do nothing

  void dynamic_tree_vote(int vote, int tag, vote_fxn fxn, int context, communicator *dom){}

 private:
  message* pop_message(std::list<message*>& msglist);

  void do_smsg_send(int dst, message* msg);

  void do_rdma_get(int src, message* msg);

  void do_nvram_get(int src, message* msg);

  void do_rdma_put(int dst, message* msg);

  void do_send_terminate(int dst);

  void do_send_ping_request(int dst);

  void go_die();

  void go_revive();

 private:
  std::list<message*> sends_;

  std::list<message*> rdma_gets_;

  std::list<message*> rdma_puts_;

  std::list<message*> nvram_gets_;
};

}
