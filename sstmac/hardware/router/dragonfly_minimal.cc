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

#include <sstmac/hardware/router/routable.h>
#include <sstmac/hardware/router/router.h>
#include <sstmac/hardware/switch/network_switch.h>
#include <sstmac/hardware/topology/dragonfly.h>
#include <sprockit/util.h>
#include <sprockit/sim_parameters.h>
#include <cmath>

#define ftree_rter_debug(...) \
  rter_debug("fat tree: %s", sprockit::printf(__VA_ARGS__).c_str())

namespace sstmac {
namespace hw {

struct dragonfly_minimal_router : public router {
  FactoryRegister("dragonfly_minimal", router, dragonfly_minimal_router)
 public:
  dragonfly_minimal_router(sprockit::sim_parameters* params, topology* top,
                           network_switch* netsw) :
    router(params, top, netsw, routing::minimal)
  {
    dfly_ = dynamic_cast<dragonfly*>(top);
    if (!dfly_){
      spkt_abort_printf("dragonfly router can only be used with dragonfly topology");
    }

    switch_id sid = addr();
    int myA = dfly_->computeA(sid);
    myG_ = dfly_->computeG(sid);
    a_ = dfly_->a();

    group_ports_.resize(dfly_->g());

    std::vector<int> connections;
    for (int g=0; g < dfly_->g(); ++g){
      if (g == myG_) continue;

      dfly_->group_wiring()->connected_to_group(myG_, g, connections);
      int rotater = myA % connections.size();
      group_ports_[g] = connections[rotater];
    }

    //figure out which groups I have a direct connection to
    dfly_->group_wiring()->connected_routers(myA, myG_, connections);
    for (int c=0; c < connections.size(); ++c){
      switch_id dst = connections[c];
      int dstG = dfly_->computeG(dst);
      group_ports_[dstG] = c + dfly_->a();
    }
  }

  std::string to_string() const {
    return "dragonfly minimal router";
  }

  void route_to_switch(switch_id ej_addr, routable::path& path)
  {
    path.vc = path.metadata_bit(routable::crossed_timeline) ? 1 : 0;

    int dstG = dfly_->computeG(ej_addr);
    if (dstG == myG_){
      int dstA = dfly_->computeA(ej_addr);
      path.set_outport(dstA);
    } else {
      int dst_port = group_ports_[dstG];
      if (dst_port >= a_){
        //direct group hop
        path.set_metadata_bit(routable::crossed_timeline);
      }
      path.set_outport(dst_port);
    }
  }

 private:
  dragonfly* dfly_;
  std::vector<int> group_ports_;
  int myG_;
  int a_;
};

}
}
