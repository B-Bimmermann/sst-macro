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

#include <sstmac/software/threading/threading_pth.h>
#include <sstmac/software/process/thread_info.h>
#include <sprockit/errors.h>

namespace sstmac {
namespace sw {

void
threading_pth::init_context() {
  if (pth_uctx_create(&context_) != TRUE) {
    spkt_throw_printf(sprockit::os_error,
        "threading_pth::init_context: %s",
        ::strerror(errno));
  }
}

void
threading_pth::destroy_context() {
  if (pth_uctx_destroy(context_) != TRUE) {
      spkt_throw_printf(sprockit::os_error,
        "threading_pth::destroy_context: %s",
        ::strerror(errno));
  }
}

void
threading_pth::start_context(int physical_thread_id, void *stack, size_t stacksize, void
                (*func)(void*), void *args, void* globals_storage, thread_context* from)
{
  if (stacksize < (16384)) {
    sprockit::abort("threading_pth::start_context: PTH does not accept stacks smaller than 16KB");
  }
  thread_info::register_user_space_virtual_thread(physical_thread_id, stack, globals_storage);
  init_context();
  int retval = pth_uctx_make(context_, (char*) stack, stacksize, NULL, func, args, NULL);
  if (retval != TRUE) {
    spkt_throw_printf(sprockit::os_error,
        "threading_pth::start_context: %s",
        ::strerror(errno));
  }
  resume_context(from);
}

void
threading_pth::resume_context(thread_context* from) {
  threading_pth* frompth = static_cast<threading_pth*>(from);
  if (pth_uctx_switch(frompth->context_, context_) != TRUE) {
    spkt_throw_printf(sprockit::os_error,
      "threading_pth::swap_context: %s",
      strerror(errno));
  }
}

void
threading_pth::pause_context(thread_context *to) {
  threading_pth* topth = static_cast<threading_pth*>(to);
  if (pth_uctx_switch(context_, topth->context_) != TRUE) {
    spkt_throw_printf(sprockit::os_error,
      "threading_pth::swap_context: %s",
      strerror(errno));
  }
}

} }
