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

#include <math.h>
#include <sstmac/software/libraries/compute/compute_event.h>
#include <sstmac/software/libraries/compute/lib_compute_matrix.h>
#include <sstmac/software/process/operating_system.h>
#include <sstmac/software/process/key.h>

namespace sstmac {
namespace sw {

void
lib_compute_matrix::double_mxm(long nrow, long nlink, long ncol,
                               long nthread)
{
  double_compute(nrow * nlink * ncol,
                 nrow * ncol + nrow * nlink + ncol * nlink, nrow * ncol, nrow * ncol,
                 0, 0, nrow * nlink * ncol, nthread);
}

void
lib_compute_matrix::double_add(long nrow, long ncol, long nthread)
{
  double_compute(2 * nrow * ncol, 2 * nrow * ncol, nrow * ncol, nrow * ncol,
                 nrow * ncol, 0, 0, nthread);
}

void
lib_compute_matrix::double_copy(long nrow, long ncol, long nthread)
{
  double_compute(nrow * ncol, nrow * ncol, nrow * ncol, nrow * ncol, 0, 0,
                 0, nthread);
}

void
lib_compute_matrix::double_compute(long ndata_read,
                                   long working_set_size_read, long ndata_write,
                                   long working_set_size_write, long nadd, long nmult, long nmultadd,
                                   long nthread)
{
  auto inst = new basic_compute_event;
  basic_instructions_st& inst_st = inst->data();
  inst_st.flops = ceil(double(nadd + nmult + 2 * nmultadd) / nthread);
  doing_memory_ = true;
  lib_compute_memmove::read(working_set_size_read / nthread);
  lib_compute_memmove::write(working_set_size_write / nthread);
  doing_memory_ = false;

  lib_compute_inst::compute_inst(inst);
  delete inst;
}

}
} //end of namespace sstmac