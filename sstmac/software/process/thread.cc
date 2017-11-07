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

#include <sstmac/common/thread_safe_int.h>
#include <sstmac/software/process/thread.h>
#include <sstmac/software/process/operating_system.h>
#include <sstmac/software/process/key.h>
#include <sstmac/software/process/app.h>
#include <sstmac/software/libraries/library.h>
#include <sstmac/software/libraries/compute/compute_event.h>
#include <sstmac/software/api/api.h>
#include <sstmac/common/sst_event.h>
#include <sprockit/errors.h>
#include <sprockit/output.h>
#include <iostream>
#include <exception>
#include <unistd.h>  // getpagesize()
#include <sys/mman.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

MakeDebugSlot(host_compute)

namespace sstmac {
namespace sw {

static thread_safe_long THREAD_ID_CNT(0);
const app_id thread::main_thread_aid(-1);
const task_id thread::main_thread_tid(-1);

//
// Private method that gets called by the scheduler.
//
void
thread::init_thread(sprockit::sim_parameters* params,
  int physical_thread_id, thread_context* des_thread, void *stack,
  int stacksize, void* globals_storage)
{
  thread_info::set_thread_id(stack, physical_thread_id);

  stack_ = stack;

  init_id();

  state_ = INITIALIZED;

  context_ = des_thread->copy();

  context_->start_context(physical_thread_id, stack, stacksize,
                          run_routine, this, globals_storage, des_thread);
}

uint32_t
thread::component_id() const
{
  return os_->component_id();
}

thread*
thread::current()
{
  return operating_system::current_thread();
}

api*
thread::_get_api(const char* name)
{
  return parent_app_->_get_api(name);
}

void
thread::clear_subthread_from_parent_app()
{
  //if this is canceled, the parent app might already be dead
  if (parent_app_){
    parent_app_->remove_subthread(this);
  }
}

void
thread::cleanup()
{
  if (state_ != CANCELED){
    clear_subthread_from_parent_app();
  }
  // We are done, ask the scheduler to remove this task from the
  state_ = DONE;

  os_->complete_active_thread();
}

//
// Run routine that defines the initial context for this task.
//
void
thread::run_routine(void* threadptr)
{
  thread* self = (thread*) threadptr;

  // Go.
  if (self->is_initialized()) {
    self->state_ = ACTIVE;
    bool success = false;
    try {
      self->run();
      success = true;
      //this doesn't so much kill the thread as context switch it out
      //it is up to the above delete thread event to actually to do deletion/cleanup
      //all of this is happening ON THE THREAD - it kills itself
      //this is not the DES thread killing it
      self->cleanup();
    }
    catch (const kill_exception& ex) {
      //great, we are done
    }
    catch (const std::exception &ex) {
      cerrn << "thread terminated with exception: " << ex.what()
                << "\n";
      // should forward the exception to the main thread,
      // but for now will abort
      cerrn << "aborting" << std::endl;
      std::cout.flush();
      std::cerr.flush();
      abort();
    }
    catch (const std::string& str) {
      cerrn << "thread terminated with string exception: " << str << "\n";
      cerrn << "aborting" << std::endl;
      std::cout.flush();
      std::cerr.flush();
      abort();
    }
  }
  else {
    sprockit::abort("thread::run_routine: task has not been initialized");
  }
}

thread::thread(sprockit::sim_parameters* params, software_id sid, operating_system* os) :
  os_(os),
  state_(PENDING),
  isInit(false),
  bt_nfxn_(0),
  last_bt_collect_nfxn_(0),
  thread_id_(thread::main_thread),
  p_txt_(process_context::none),
  context_(nullptr),
  cpumask_(0),
  host_timer_(nullptr),
  parent_app_(nullptr),
  timed_out_(false),
  block_counter_(0),
  sid_(sid)
{
  //make all cores possible active
  cpumask_ = ~(cpumask_);
}

void
thread::start_api_call()
{
  if (host_timer_){
    double duration = host_timer_->stamp();
    debug_printf(sprockit::dbg::host_compute,
                 "host compute for %12.8es", duration);
    parent_app()->compute(timestamp(duration));
  }
}

void
thread::end_api_call()
{
  if (host_timer_){
    host_timer_->start();
  }
}

long
thread::init_id()
{
  //thread id not yet initialized
  if (thread_id_ == thread::main_thread)
    thread_id_ = THREAD_ID_CNT++;
  //I have not yet been assigned a process context (address space)
  //make my own, for now
  if (p_txt_ == process_context::none)
    p_txt_ = thread_id_;
  return thread_id_;
}

void*
thread::get_tls_value(long thekey) const
{
  auto it = tls_values_.find(thekey);
  if (it == tls_values_.end())
    return nullptr;
  return it->second;
}

void
thread::set_tls_value(long thekey, void *ptr)
{
  tls_values_[thekey] = ptr;
}

void
thread::append_backtrace(int id)
{
#if SSTMAC_HAVE_GRAPHVIZ
  backtrace_[bt_nfxn_] = id;
  bt_nfxn_++;
#else
  sprockit::abort("did not compile with call graph support");
#endif
}

void
thread::pop_backtrace()
{
  --bt_nfxn_;
  last_bt_collect_nfxn_ = std::min(last_bt_collect_nfxn_, bt_nfxn_);
}

void
thread::collect_backtrace(int nfxn)
{
  last_bt_collect_nfxn_ = nfxn;
}

void
thread::spawn(thread* thr)
{
  thr->parent_app_ = parent_app();
  if (host_timer_){
    thr->host_timer_ = new HostTimer;
    thr->host_timer_->start();
  }
  os_->start_thread(thr);
}

timestamp
thread::now()
{
  return os_->now();
}

thread::~thread()
{
  if (stack_) stack_alloc::free(stack_);
  if (context_) {
    context_->destroy_context();
    delete context_;
  }
  if (host_timer_) delete host_timer_;
}


void
thread::start_thread(thread* thr)
{
  thr->p_txt_ = p_txt_;
  os_->start_thread(thr);
}


void
thread::join()
{
  if (!this->is_initialized()) {
    // We can't context switch the caller out without first being initialized
    spkt_throw_printf(sprockit::illformed_error,
                     "thread::join: target thread has not been initialized.");
  }
  os_->join_thread(this);
}

}
} // end of namespace sstmac
