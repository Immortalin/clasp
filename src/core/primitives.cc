/*
    File: primitives.cc
*/

/*
Copyright (c) 2014, Christian E. Schafmeister

CLASP is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

See directory 'clasp/licenses' for full details.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
/* -^- */
#define DEBUG_LEVEL_FULL

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <clasp/core/foundation.h>
#include <clasp/core/object.h>
#include <clasp/core/cons.h>
#include <clasp/core/corePackage.h>
#include <clasp/core/environment.h>
#include <clasp/core/fileSystem.h>
#include <clasp/core/bformat.h>
#include <clasp/core/bignum.h>
#include <clasp/core/sysprop.h>
#include <clasp/core/character.h>
#include <clasp/core/package.h>
#include <clasp/core/readtable.h>
#include <clasp/core/vectorObjectsWithFillPtr.h>
#include <clasp/core/instance.h>
#include <clasp/core/backquote.h>
#include <clasp/core/sequence.h>
#include <clasp/core/structureObject.h>
#include <clasp/core/wrappedPointer.h>
#include <clasp/core/bitVector.h>
#include <clasp/core/pathname.h>
#include <clasp/core/unixfsys.h>
#include <clasp/core/predicates.h>
#include <clasp/core/genericFunction.h>
#include <clasp/core/pointer.h>
#include <clasp/core/lispMath.h>
#include <clasp/core/symbolTable.h>
#include <clasp/core/clcenv.h>
#include <clasp/core/null.h>
//#include "debugger.h"
#include <clasp/core/ql.h>
#include <clasp/core/str.h>
#include <clasp/core/numbers.h>
#include <clasp/core/evaluator.h>
#include <clasp/core/compiler.h>
#include <clasp/core/print.h>
#include <clasp/core/standardClass.h>
#include <clasp/core/forwardReferencedClass.h>
#include <clasp/core/singleDispatchMethod.h>
#include <clasp/core/singleDispatchGenericFunction.h>
#include <clasp/core/lambdaListHandler.h>
#include <clasp/core/designators.h>
#include <clasp/core/primitives.h>
#include <clasp/core/hashTable.h>
#include <clasp/core/hashTableEql.h>
#include <clasp/core/multipleValues.h>
#include <clasp/core/lispStream.h>
#include <clasp/core/structureObject.h>
#include <clasp/core/lispReader.h>
#include <clasp/core/designators.h>
#include <clasp/core/profile.h>
#include <clasp/core/wrappers.h>
namespace core {

CL_LAMBDA(seconds);
CL_DECLARE();
CL_DOCSTRING("sleep");
CL_DEFUN void cl__sleep(T_sp oseconds) {
  SYMBOL_EXPORT_SC_(ClPkg, sleep);
  if (oseconds.nilp()) {
    ERROR_WRONG_TYPE_ONLY_ARG(cl::_sym_sleep, oseconds, cl::_sym_Number_O);
  }
  double dsec = clasp_to_double(gc::As<Real_sp>(oseconds));
  if (dsec < 0.0) {
    SIMPLE_ERROR(BF("You cannot sleep for < 0 seconds"));
  }
  double seconds = floor(dsec);
  double frac_seconds = dsec - seconds;
  double nanoseconds = (frac_seconds * 1000000000.0);
  timespec ts;
  ts.tv_sec = seconds;
  ts.tv_nsec = nanoseconds;
  nanosleep(&ts, NULL);
}

CL_LAMBDA();
CL_DECLARE();
CL_DOCSTRING("lispImplementationType");
CL_DEFUN T_sp cl__lisp_implementation_type() {
  return Str_O::create(program_name());
};

CL_LAMBDA();
CL_DECLARE();
CL_DOCSTRING("lispImplementationVersion");
CL_DEFUN T_sp cl__lisp_implementation_version() {
  stringstream ss;
  List_sp cleavir = gc::As<Cons_sp>(cl::_sym_STARfeaturesSTAR->symbolValue())->memberEq(kw::_sym_cclasp);
  if (cleavir.notnilp()) {
    ss << "c";
  }
  ss << program_name();
  ss << "-";
#ifdef USE_MPS
  ss << "mps-";
#endif
#ifdef USE_BOEHM
  ss << "boehm-";
#endif
  ss << CLASP_VERSION;
  return Str_O::create(ss.str());
};

CL_LAMBDA(pow);
CL_DECLARE();
CL_DOCSTRING("cache_resize - Resize the cache to 2^pow");
CL_DEFUN void core__method_cache_resize(Fixnum pow) {
  if (pow < 2 || pow > 64) {
    SIMPLE_ERROR(BF("Cache power must be in the range of 2...64"));
  }
  size_t size = 1 << pow;
  return _lisp->_Roots._MethodCachePtr->setup(Lisp_O::MaxFunctionArguments, size);
}

CL_LAMBDA(pow);
CL_DECLARE();
CL_DOCSTRING("cache_resize - Resize the cache to 2^pow");
CL_DEFUN void core__slot_cache_resize(Fixnum pow) {
  if (pow < 2 || pow > 64) {
    SIMPLE_ERROR(BF("Cache power must be in the range of 2...64"));
  }
  size_t size = 1 << pow;
  return _lisp->_Roots._SlotCachePtr->setup(Lisp_O::MaxClosSlots, size);
}

CL_LAMBDA(pow);
CL_DECLARE();
CL_DOCSTRING("cache_resize - Resize the cache to 2^pow");
CL_DEFUN void core__single_dispatch_method_cache_resize(Fixnum pow) {
  if (pow < 2 || pow > 64) {
    SIMPLE_ERROR(BF("Cache power must be in the range of 2...64"));
  }
  size_t size = 1 << pow;
  return _lisp->_Roots._SingleDispatchMethodCachePtr->setup(2, size);
}

CL_LAMBDA();
CL_DECLARE();
CL_DOCSTRING("cache_status - (values searches misses total-depth)");
CL_DEFUN T_mv core__method_cache_status() {
  return Values(clasp_make_fixnum(_lisp->_Roots._MethodCachePtr->_searches),
                clasp_make_fixnum(_lisp->_Roots._MethodCachePtr->_misses),
                clasp_make_fixnum(_lisp->_Roots._MethodCachePtr->_total_depth));
}
CL_LAMBDA();
CL_DECLARE();
CL_DOCSTRING("cache_status - (values searches misses total-depth)");
CL_DEFUN T_mv core__slot_cache_status() {
  return Values(clasp_make_fixnum(_lisp->_Roots._SlotCachePtr->_searches),
                clasp_make_fixnum(_lisp->_Roots._SlotCachePtr->_misses),
                clasp_make_fixnum(_lisp->_Roots._SlotCachePtr->_total_depth));
}

CL_LAMBDA();
CL_DECLARE();
CL_DOCSTRING("cache_status - (values searches misses total-depth)");
CL_DEFUN T_mv core__single_dispatch_method_cache_status() {
  return Values(clasp_make_fixnum(_lisp->_Roots._SingleDispatchMethodCachePtr->_searches),
                clasp_make_fixnum(_lisp->_Roots._SingleDispatchMethodCachePtr->_misses),
                clasp_make_fixnum(_lisp->_Roots._SingleDispatchMethodCachePtr->_total_depth));
}

CL_LAMBDA();
CL_DECLARE();
CL_DOCSTRING("lispImplementationId - the git commit sha1 code");
CL_DEFUN T_sp core__lisp_implementation_id() {
  string all = CLASP_GIT_COMMIT;
#define RIGHT_CHARS 8
  string rightChars;
  if (all.size() > RIGHT_CHARS) {
    rightChars = all.substr(all.size() - RIGHT_CHARS);
  } else {
    rightChars = all;
  }
  return Str_O::create(rightChars);
};

CL_LAMBDA(obj);
CL_DECLARE();
CL_DOCSTRING("Convert an object, either a fixnum, character or single float into an tagged version and return as an integer (either Fixnum or Bignum) or return NIL");
CL_DEFUN T_sp core__create_tagged_immediate_value_or_nil(T_sp object) {
  if (object.fixnump() || object.characterp() || object.single_floatp()) {
    return Integer_O::create((gc::Fixnum)object.raw_());
  }
  return _Nil<T_O>();
};

CL_LAMBDA();
CL_DECLARE();
CL_DOCSTRING("softwareType");
CL_DEFUN T_sp cl__software_type() {
  return _Nil<T_O>();
};

CL_LAMBDA();
CL_DECLARE();
CL_DOCSTRING("softwareVersion");
CL_DEFUN T_sp cl__software_version() {
  string all = CLASP_VERSION;
  return Str_O::create(all);
};

CL_LAMBDA();
CL_DECLARE();
CL_DOCSTRING("machineType");
CL_DEFUN T_sp cl__machine_type() {
  return _Nil<T_O>();
};

CL_LAMBDA();
CL_DECLARE();
CL_DOCSTRING("machineVersion");
CL_DEFUN T_sp cl__machine_version() {
  return _Nil<T_O>();
};

CL_LAMBDA();
CL_DECLARE();
CL_DOCSTRING("machineInstance");
CL_DEFUN T_sp cl__machine_instance() {
  return _Nil<T_O>();
};

CL_LAMBDA();
CL_DECLARE();
CL_DOCSTRING("argc");
CL_DEFUN int core__argc() {
  return _lisp->_Argc;
};

CL_LAMBDA(idx);
CL_DECLARE();
CL_DOCSTRING("argv");
CL_DEFUN Str_sp core__argv(int idx) {
  if ( idx < _lisp->_Argc ) return Str_O::create(_lisp->_Argv[idx]);
  return Str_O::create("");
};

CL_LAMBDA(sym value);
CL_DECLARE();
CL_DOCSTRING("set");
CL_DEFUN T_sp cl__set(Symbol_sp sym, T_sp val) {
  if (sym.nilp()) {
    SIMPLE_ERROR(BF("You cannot assign to the constant NIL"));
  }
  sym->setf_symbolValue(val);
  return val;
};

CL_LAMBDA(arg);
CL_DECLARE();
CL_DOCSTRING("dumpAddressOf");
CL_DEFUN void core__dump_address_of(T_sp arg) {
  ASSERT(arg.objectp());
  void *ptr = &(*arg);
  printf("%s:%d  AddressOf = %p\n", __FILE__, __LINE__, ptr);
};

CL_LAMBDA(arg);
CL_DECLARE();
CL_DOCSTRING("incompleteNextHigherPowerOf_2 - see the incompleteNextHigherPowerOf_2 builtin - only works for Fixnums and not the full range; just for testing");
CL_DEFUN int core__incomplete_next_higher_power_of_2(Fixnum_sp fn) {
  unsigned int f = unbox_fixnum(fn);
  return 1 << ((sizeof(f) * 8) - __builtin_clz(f));
};

CL_LAMBDA();
CL_DECLARE();
CL_DOCSTRING("allRegisteredClassNames");
CL_DEFUN Vector_sp core__all_registered_class_names() {
  VectorObjects_sp vo = VectorObjects_O::make(_Nil<T_O>(), _Nil<T_O>(), _lisp->classSymbolsHolder().size(), false, cl::_sym_T_O);
  for (int i(0), iEnd(_lisp->classSymbolsHolder().size()); i < iEnd; ++i) {
    vo->setf_elt(i, _lisp->classSymbolsHolder()[i]);
  }
  return vo;
};

CL_LAMBDA(arg);
CL_DECLARE();
CL_DOCSTRING("toTaggedFixnum");
CL_DEFUN T_sp core__to_tagged_fixnum(int val) {
  return gctools::smart_ptr<T_O>(val);
};

CL_LAMBDA(val);
CL_DECLARE();
CL_DOCSTRING("fromTaggedFixnum");
CL_DEFUN gctools::Fixnum core__from_tagged_fixnum(T_sp val) {
  if (val.fixnump()) {
    return val.unsafe_fixnum();
  }
  SIMPLE_ERROR(BF("Not a fixnum"));
};

CL_LAMBDA(arg);
CL_DECLARE();
CL_DOCSTRING("dumpTaggedFixnum");
CL_DEFUN void core__dump_tagged_fixnum(T_sp val) {
  if (val.fixnump()) {
    printf("%s:%d Raw TaggedFixnum %p   Untagged %ld\n",
           __FILE__, __LINE__, val.raw_(), val.unsafe_fixnum());
  } else
    printf("%s:%d Not a tagged fixnum\n", __FILE__, __LINE__);
}

CL_LAMBDA(name value);
CL_DECLARE();
CL_DOCSTRING("Set environment variable NAME to VALUE");
CL_DEFUN void ext__setenv(Str_sp name, Str_sp value) {
  setenv(name->get().c_str(), value->get().c_str(), 1);
}

CL_LAMBDA(cmd);
CL_DECLARE();
CL_DOCSTRING("system");
CL_DEFUN T_mv ext__system(Str_sp cmd) {
  string command = cmd->get();
  int ret = system(command.c_str());
  if (ret == 0) {
    return Values(core::make_fixnum(0));
  } else {
    return Values(core::make_fixnum(ret), Str_O::create(std::strerror(errno)));
  }
}

CL_LAMBDA(call-and-arguments &optional return-stream);
CL_DECLARE();
CL_DOCSTRING("vfork_execvp - set optional return-stream if you want the output stream of the child");
CL_DEFUN T_mv ext__vfork_execvp(List_sp call_and_arguments, T_sp return_stream) {
  bool bReturnStream = return_stream.isTrue();
  if (call_and_arguments.nilp())
    return Values0<T_O>();
  std::vector<char const *> execvp_args(cl__length(call_and_arguments) + 1);
  size_t idx = 0;
  for (auto cur : call_and_arguments) {
    Str_sp sarg = gc::As<Str_sp>(oCar(cur));
    size_t sarg_size = sarg->size();
//    printf("%s:%d sarg = %s sarg->size() = %ld  strlen(sarg->c_str()) = %ld\n", __FILE__, __LINE__, sarg->c_str(), sarg->size(), strlen(sarg->c_str()));
    char *arg = (char *)malloc(sarg_size + 1);
    std::strncpy(arg, sarg->c_str(),sarg_size);
    arg[sarg_size] = '\0';
    execvp_args[idx++] = arg;
  }
  execvp_args[idx] = NULL;
  int filedes[2];
  if (bReturnStream) {
    if (pipe(filedes) == -1 ) {
      perror("pipe");
      abort();
    }
  }
  pid_t child_PID = vfork();
  if (child_PID >= 0) {
    if (child_PID == 0) {
      // Child
      if ( bReturnStream ) {
        while ((dup2(filedes[1],STDOUT_FILENO) == -1) && (errno == EINTR)) {}
        close(filedes[1]);
        close(filedes[0]);
        int flags = fcntl(STDOUT_FILENO,F_GETFL,0);
        fcntl(STDOUT_FILENO,F_SETFL,flags|FD_CLOEXEC);
      }
      execvp(execvp_args[0], (char *const *)execvp_args.data());
      printf("%s:%d execvp returned with errno=%d   strerror(errno) = %s\n", __FILE__, __LINE__, errno, strerror(errno));
      for (int i = 0; execvp_args[i] != NULL; ++i) {
        printf("    arg#%d  %s\n", i, execvp_args[i]);
      }
      printf("  cannot continue... exiting... sorry...\n");
      _exit(0); // Should never reach
    } else {
      // Parent
      int status;
      pid_t wait_ret = wait(&status);
      // Clean up args
      for (int i(0); i < execvp_args.size() - 1; ++i)
        free((void *)execvp_args[i]);
      if (wait_ret >= 0) {
        if (wait_ret != child_PID) {
          printf("%s:%d wait return PID(%d) that did not match child(%d)\n", __FILE__, __LINE__, wait_ret, child_PID);
        }
        if ( bReturnStream ) {
          int flags = fcntl(filedes[0],F_GETFL,0);
          fcntl(filedes[0],F_SETFL,flags|O_NONBLOCK);
          T_sp stream = clasp_make_file_stream_from_fd(Str_O::create("execvp"), filedes[0], clasp_smm_input_file, 8, CLASP_STREAM_DEFAULT_FORMAT, _Nil<T_O>());
          return Values(_Nil<T_O>(), clasp_make_fixnum(child_PID),stream);
        }
        return Values(_Nil<T_O>(),clasp_make_fixnum(child_PID),_Nil<T_O>());
      }
      // error
      return Values(clasp_make_fixnum(errno), Str_O::create(std::strerror(errno)),_Nil<T_O>());
    }
  } else {
    // Clean up args
    for (int i(0); i < execvp_args.size() - 1; ++i)
      free((void *)execvp_args[i]);
    return Values(clasp_make_fixnum(-1), Str_O::create(std::strerror(errno)),_Nil<T_O>());
  }
}



CL_LAMBDA(call-and-arguments &optional return-stream);
CL_DECLARE();
CL_DOCSTRING("fork_execvp - set optional return-stream if you want the output stream of the child");
CL_DEFUN T_mv ext__fork_execvp(List_sp call_and_arguments, T_sp return_stream) {
  bool bReturnStream = return_stream.isTrue();
  if (call_and_arguments.nilp())
    return Values0<T_O>();
  std::vector<char const *> execvp_args(cl__length(call_and_arguments) + 1);
  size_t idx = 0;
  for (auto cur : call_and_arguments) {
    Str_sp sarg = gc::As<Str_sp>(oCar(cur));
    size_t sarg_size = sarg->size();
//    printf("%s:%d sarg = %s sarg->size() = %ld\n", __FILE__, __LINE__, sarg->c_str(), sarg->size());
    char *arg = (char *)malloc(sarg_size + 1);
    std::strncpy(arg, sarg->c_str(),sarg_size);
    arg[sarg_size] = '\0';
      execvp_args[idx++] = arg;
  }
  execvp_args[idx] = NULL;
  int filedes[2];
  if (bReturnStream) {
    if (pipe(filedes) == -1 ) {
      perror("pipe");
      abort();
    }
  }
  pid_t child_PID = fork();
  if (child_PID >= 0) {
    if (child_PID == 0) {
      // Child
      if ( bReturnStream ) {
        while ((dup2(filedes[1],STDOUT_FILENO) == -1) && (errno == EINTR)) {}
        close(filedes[1]);
        close(filedes[0]);
        int flags = fcntl(STDOUT_FILENO,F_GETFL,0);
        fcntl(STDOUT_FILENO,F_SETFL,flags|FD_CLOEXEC);
      }
      execvp(execvp_args[0], (char *const *)execvp_args.data());
      printf("%s:%d execvp returned with errno=%d   strerror(errno) = %s\n", __FILE__, __LINE__, errno, strerror(errno));
      for (int i = 0; execvp_args[i] != NULL; ++i) {
        printf("    arg#%d  %s\n", i, execvp_args[i]);
      }
      printf("  cannot continue... exiting... sorry...\n");
      _exit(0); // Should never reach
    } else {
      // Parent
      int status;
      pid_t wait_ret = wait(&status);
      // Clean up args
      for (int i(0); i < execvp_args.size() - 1; ++i)
        free((void *)execvp_args[i]);
      if (wait_ret >= 0) {
        if (wait_ret != child_PID) {
          printf("%s:%d wait return PID(%d) that did not match child(%d)\n", __FILE__, __LINE__, wait_ret, child_PID);
        }
        if ( bReturnStream ) {
          int flags = fcntl(filedes[0],F_GETFL,0);
          fcntl(filedes[0],F_SETFL,flags|O_NONBLOCK);
          T_sp stream = clasp_make_file_stream_from_fd(Str_O::create("execvp"), filedes[0], clasp_smm_input_file, 8, CLASP_STREAM_DEFAULT_FORMAT, _Nil<T_O>());
          return Values(_Nil<T_O>(), clasp_make_fixnum(child_PID),stream);
        }
        return Values(_Nil<T_O>(),clasp_make_fixnum(child_PID),_Nil<T_O>());
      }
      // error
      return Values(clasp_make_fixnum(errno), Str_O::create(std::strerror(errno)),_Nil<T_O>());
    }
  } else {
    // Clean up args
    for (int i(0); i < execvp_args.size() - 1; ++i)
      free((void *)execvp_args[i]);
    return Values(clasp_make_fixnum(-1), Str_O::create(std::strerror(errno)),_Nil<T_O>());
  }
}

CL_LAMBDA(arg);
CL_DECLARE();
CL_DOCSTRING("Get environment variable NAME");
CL_DEFUN T_sp ext__getenv(Str_sp arg) {
  char *sres = getenv(arg->c_str());
  if (sres == NULL) {
    return _Nil<T_O>();
  }
  return Str_O::create(sres);
};


CL_DOCSTRING(R"doc(Return a string representing the llvm version (eg: 3.6.0))doc");
CL_DEFUN T_sp ext__llvm_version() {
  return core::Str_O::create(LLVM_VERSION);
}


CL_LAMBDA(name &optional stream);
CL_DECLARE();
CL_DOCSTRING(R"doc(Describe a
C++ object
like CL:DESCRIBE)doc");
CL_DEFUN void core__describe_cxx_object(T_sp obj, T_sp stream)
{
  if (obj.generalp()) {
    obj.unsafe_general()->describe(stream);
  } else if (obj.consp()) {
    obj.unsafe_cons()->describe(stream);
  }
  SIMPLE_ERROR(BF("Use the CL facilities to describe this object"));
};

CL_LAMBDA(name arg overwrite);
CL_DECLARE();
CL_DOCSTRING(R"doc(Set an environment variable)doc");
CL_DEFUN void core__setenv(Str_sp name, Str_sp arg, bool overwrite) {
  setenv(name->c_str(), arg->c_str(), overwrite);
};

CL_LAMBDA(arg);
CL_DECLARE();
CL_DOCSTRING("Return the value of the pointer - used by conditions.lsp");
CL_DEFUN int core__pointer(T_sp obj) {
  return obj.intptr();
};

CL_LAMBDA(arg);
CL_DECLARE();
CL_DOCSTRING("isTrue");
CL_DEFUN bool core__is_true(T_sp arg) {
  return arg.isTrue();
};

CL_LAMBDA();
CL_DECLARE();
CL_DOCSTRING("Return the UNBOUND value");
CL_DEFUN T_sp core__unbound() {
  return _Unbound<T_O>();
};

CL_LAMBDA();
CL_DECLARE();
CL_DOCSTRING("smartPointerDetails - returns (values ptr-type px-offset px-size). The ptr-type is the type of pointer used to pass objects - either MPS-GARBAGE-COLLECTION or INTRUSIVE-REFERENCE-COUNTED-POINTER. The px-offset is the number of bytes offset of the smart_ptr data pointer from the start of the smart_ptr and px-size is the size of the data pointer");
CL_DEFUN T_mv core__smart_pointer_details() {
  SYMBOL_SC_(CorePkg, intrusiveReferenceCountedPointer);
  SYMBOL_SC_(CorePkg, sharedReferenceCountedPointer);
  SYMBOL_SC_(CorePkg, mpsGarbageCollection);
#if defined(USE_MPS)
  Symbol_sp ptrType = _sym_mpsGarbageCollection;
#else
  Symbol_sp ptrType = _sym_intrusiveReferenceCountedPointer;
#endif
  T_sp dummy;
  Fixnum_sp pxOffset = make_fixnum(0);
  Fixnum_sp pxSize = make_fixnum((gctools::Fixnum)gctools::pointer_size);
  return Values(ptrType, pxOffset, pxSize);
}

CL_LAMBDA(&va-rest args);
CL_DECLARE();
CL_DOCSTRING("values");
CL_DEFUN T_mv cl__values(VaList_sp vargs) {
  // returns multiple values
  ASSERT(vargs.valistp());
  size_t nargs = vargs->remaining_nargs();
  if (nargs >= core::MultipleValues::MultipleValuesLimit) {
    SIMPLE_ERROR(BF("Too many arguments to values - only %d are supported and you tried to return %d values") % core::MultipleValues::MultipleValuesLimit % nargs );
  }
  SUPPRESS_GC();
  core::MultipleValues &me = (core::lisp_multipleValues());
  me.setSize(0);
  core::T_sp first = vargs->next_arg();
  for (size_t i(1); i< nargs; ++i ) {
    T_sp csp = vargs->next_arg();//LCC_NEXT_ARG(vargs,i);
    me.valueSet(i, csp);
  }
  me.setSize(nargs);
  ENABLE_GC();
  core::T_mv mv = gctools::multiple_values<core::T_O>(first,nargs);
  return mv;
}


CL_LAMBDA(&rest args);
CL_DECLARE();
CL_DOCSTRING("values");
CL_DEFUN T_mv core__values_testing(List_sp args) {
  // returns multiple values
  T_mv result = ValuesFromCons(args);
  printf("%s:%d core__values_testing: %s\n", __FILE__, __LINE__, _rep_(args).c_str());
  return result;
}

CL_LAMBDA(list);
CL_DECLARE();
CL_DOCSTRING("values_list");
CL_DEFUN T_mv cl__values_list(List_sp list) {
  return ValuesFromCons(list);
}

Symbol_sp functionBlockName(T_sp functionName) {
  if (cl__symbolp(functionName))
    return gc::As<Symbol_sp>(functionName);
  if ((functionName).consp()) {
    List_sp cfn = functionName;
    if (oCar(cfn) == cl::_sym_setf && cl__symbolp(oCadr(cfn)) & oCadr(cfn).notnilp()) {
      return gc::As<Symbol_sp>(oCadr(cfn));
    }
  }
  return _Nil<Symbol_O>();
}

CL_LAMBDA(functionName);
CL_DECLARE();
CL_DOCSTRING("See CLHS glossary 'function block name'. If the functionName is a symbol return it.  If the functionName is a cons of the form (setf xxxx) return xxxx");
CL_DEFUN Symbol_sp core__function_block_name(T_sp functionName) {
  Symbol_sp output = functionBlockName(functionName);
  if (output.nilp()) {
    SIMPLE_ERROR(BF("Invalid function name: %s") % _rep_(functionName));
  }
  return output;
}

CL_LAMBDA(arg);
CL_DECLARE();
CL_DOCSTRING("validFunctionNameP");
CL_DEFUN T_mv core__valid_function_name_p(T_sp arg) {
  T_sp name = functionBlockName(arg);
  if (name.nilp())
    return (Values(_Nil<T_O>()));
  return (Values(_lisp->_true()));
};

CL_LAMBDA();
CL_DECLARE();
CL_DOCSTRING("testMemoryError");
CL_DEFUN void core__test_memory_error() {
  int *h = (int *)malloc(sizeof(int));
  *h = 1;
  free(h);
  *h = 2;
};

CL_LAMBDA(listOfPairs);
CL_DECLARE();
CL_DOCSTRING("Split a list of pairs into a pair of lists returned as MultipleValues. The first list is each first element and the second list is each second element or nil if there was no second element");
CL_DEFUN T_mv core__separate_pair_list(List_sp listOfPairs) {
  ql::list firsts(_lisp);
  ql::list seconds(_lisp);
  for (auto cur : listOfPairs) {
    T_sp element = oCar(cur);
    if (cl__atom(element)) {
      firsts << element;
      seconds << _Nil<T_O>();
    } else if ((element).consp()) {
      List_sp pair = element;
      size_t pairlen = cl__length(pair);
      if (pairlen == 2 || pairlen == 1) {
        firsts << oCar(pair);
        seconds << oCadr(pair);
      } else {
        SIMPLE_ERROR(BF("Expected one or two element list got: %s") % _rep_(pair));
      }
    } else {
      SIMPLE_ERROR(BF("Expected single object or 2-element list - got: %s") % _rep_(element));
    }
  }
  T_sp tfirsts = firsts.cons();
  return (Values(tfirsts, seconds.cons()));
}

#if DEPRECIATED_C_FUNCTION
CL_LAMBDA(sym);
CL_DECLARE();
CL_DOCSTRING("c_function");
CL_DEFUN Pointer_mv core__c_function(Symbol_sp sym) {
  return (Values(_lisp->lookup_c_function_ptr(sym)));
};
#endif

// ignore env
CL_DEFUN T_sp core__compiler_macro_function(core::T_sp name, core::T_sp env)
{
  return core__get_sysprop(name,cl::_sym_compiler_macro);
}

// ignore env
CL_LAMBDA(name &optional env);
CL_DEFUN T_mv core__get_compiler_macro_function(core::T_sp name, core::T_sp env)
{
  return core__get_sysprop(name,cl::_sym_compiler_macro);
}

CL_LAMBDA(name function &optional env);
CL_DEFUN void core__setf_compiler_macro_function(core::T_sp name, core::T_sp function, core::T_sp env)
{
  core__put_sysprop(name,cl::_sym_compiler_macro,function);
}

// ignore env
CL_LAMBDA(name &optional env);
CL_DEFUN T_sp core__get_global_inline_status(core::T_sp name, core::T_sp env)
{
  return core__get_sysprop(name,cl::_sym_inline);
}

CL_LAMBDA(name status &optional env);
CL_DEFUN void core__setf_global_inline_status(core::T_sp name, bool status, core::T_sp env)
{
  core__put_sysprop(name,cl::_sym_inline,_lisp->_boolean(status));
}




CL_LAMBDA(symbol &optional env);
CL_DECLARE();
CL_DOCSTRING("See CLHS: macroFunction");
CL_DEFUN T_sp cl__macro_function(Symbol_sp symbol, T_sp env) {
  T_sp func = _Nil<T_O>();
  if (env.nilp()) {
    func = af_interpreter_lookup_macro(symbol, env);
  } else if (Environment_sp eenv = env.asOrNull<Environment_O>()) {
    func = af_interpreter_lookup_macro(symbol, eenv);
  } else if (clcenv::Entry_sp cenv = env.asOrNull<clcenv::Entry_O>()) {
    clcenv::Info_sp info = clcenv::function_info(cenv,symbol);
    if ( clcenv::LocalMacroInfo_sp lm = info.asOrNull<clcenv::LocalMacroInfo_O>() ) {
      func = lm->_Expander;
    } else if (clcenv::GlobalMacroInfo_sp gm = info.asOrNull<clcenv::GlobalMacroInfo_O>() ) {
      func = gm->_Expander;
    }
  } else {
    if (cleavirEnv::_sym_macroFunction->fboundp()) {
      func = eval::funcall(cleavirEnv::_sym_macroFunction, symbol, env);
    } else {
      printf("%s:%d Unexpected environment for MACRO-FUNCTION before Cleavir is available - using toplevel environment\n", __FILE__, __LINE__);
      func = af_interpreter_lookup_macro(symbol, _Nil<T_O>());
    }
  }
  return func;
}

CL_LAMBDA(symbol);
CL_DECLARE();
CL_DOCSTRING("See CLHS: special-operator-p");
CL_DEFUN T_mv cl__special_operator_p(T_sp sym) {
  SYMBOL_EXPORT_SC_(ClPkg, let);
  SYMBOL_EXPORT_SC_(ClPkg, letSTAR);
  SYMBOL_EXPORT_SC_(ClPkg, return_from);
  SYMBOL_EXPORT_SC_(ClPkg, catch);
  SYMBOL_EXPORT_SC_(ClPkg, load_time_value);
  SYMBOL_EXPORT_SC_(ClPkg, setq);
  SYMBOL_EXPORT_SC_(ClPkg, eval_when);
  SYMBOL_EXPORT_SC_(ClPkg, locally);
  SYMBOL_EXPORT_SC_(ClPkg, symbol_macrolet);
  SYMBOL_EXPORT_SC_(ClPkg, flet);
  SYMBOL_EXPORT_SC_(ClPkg, macrolet);
  SYMBOL_EXPORT_SC_(ClPkg, tagbody);
  SYMBOL_EXPORT_SC_(ClPkg, multiple_value_call);
  SYMBOL_EXPORT_SC_(ClPkg, the);
  SYMBOL_EXPORT_SC_(ClPkg, go);
  SYMBOL_EXPORT_SC_(ClPkg, multiple_value_prog1);
  SYMBOL_EXPORT_SC_(ClPkg, if);
  SYMBOL_EXPORT_SC_(ClPkg, unwind_protect);
  SYMBOL_EXPORT_SC_(ClPkg, labels);
  SYMBOL_EXPORT_SC_(ClPkg, progv);
  if ((sym == cl::_sym_block) ||
      (sym == cl::_sym_let) ||
      (sym == cl::_sym_letSTAR) ||
      (sym == cl::_sym_return_from) ||
      (sym == cl::_sym_load_time_value) ||
      (sym == cl::_sym_setq) ||
      (sym == cl::_sym_eval_when) ||
      (sym == cl::_sym_locally) ||
      (sym == cl::_sym_symbol_macrolet) ||
      (sym == cl::_sym_flet) ||
      (sym == cl::_sym_macrolet) ||
      (sym == cl::_sym_tagbody) ||
      (sym == cl::_sym_function) ||
      (sym == cl::_sym_multiple_value_call) ||
      (sym == cl::_sym_the) ||
      (sym == cl::_sym_go) ||
      (sym == cl::_sym_multiple_value_prog1) ||
      (sym == cl::_sym_if) ||
      (sym == cl::_sym_progn) ||
      (sym == cl::_sym_labels) ||
      (sym == cl::_sym_unwind_protect) ||
      (sym == cl::_sym_catch) ||
      (sym == cl::_sym_throw) ||
      (sym == cl::_sym_progv) ||
      (sym == cl::_sym_quote)) {
    return (Values(_lisp->_true()));
  }
  return (Values(_Nil<T_O>()));
};


CL_DECLARE();
CL_DOCSTRING("CLHS: ash");
CL_DEFUN Integer_sp cl__ash(Integer_sp integer, Integer_sp count) {
  int cnt = clasp_to_int(count);
  return clasp_shift(integer, cnt);
}

CL_LAMBDA(&optional fmt-control &rest args);
CL_DECLARE();
CL_DOCSTRING("Built in implementation of break - that calls the internal debugger - replace this with a CL implemented version");
CL_DEFUN void core__break(T_sp fmt, List_sp args) {
  int frame = core__ihs_top();
  T_sp tframe = clasp_make_fixnum(frame);
  DynamicScopeManager scope(_sym_STARstack_top_hintSTAR,tframe);
  if (fmt.notnilp()) {
    cl__format(_lisp->_true(), gc::As<Str_sp>(fmt), args);
  }
  dbg_hook("built in break");
  core__invoke_internal_debugger(_Nil<core::T_O>());
};

CL_LAMBDA(&optional msg);
CL_DECLARE();
CL_DOCSTRING("hook to invoke gdb");
CL_DEFUN void core__gdb(T_sp msg) {
  T_sp obj = msg;
  string smsg = "No msg";
  if (obj.notnilp()) {
    smsg = _rep_(obj);
  }
  dbg_hook(smsg.c_str());
//  core__invoke_internal_debugger(_Nil<core::T_O>());
};


CL_LAMBDA(&optional msg);
CL_DECLARE();
CL_DOCSTRING("hook to invoke gdb");
CL_DEFUN void core__trap_execution(T_sp msg) {
  T_sp obj = msg;
  string smsg = "No msg";
  if (obj.notnilp()) {
    smsg = _rep_(obj);
  }
  printf("%s:%d In core__trap_execution: %s \n", __FILE__, __LINE__, smsg.c_str());
  fflush(stdout);
};

CL_LAMBDA(msg o);
CL_DECLARE();
CL_DOCSTRING("hook to invoke gdb");
CL_DEFUN void core__gdb_inspect(Str_sp msg, T_sp o) {
  printf("gdbInspect object: %s\n", _rep_(o).c_str());
  dbg_hook(msg->get().c_str());
  core__invoke_internal_debugger(_Nil<core::T_O>());
};

CL_LAMBDA(obj &optional env);
CL_DECLARE();
CL_DOCSTRING("constantp");
CL_DEFUN bool cl__constantp(T_sp obj, T_sp env) {
  // ignore env
  if (cl__numberp(obj))
    return true;
  if (cl__characterp(obj))
    return true;
  if (core__arrayp(obj))
    return true;
  // TODO add various kinds of array
  if ((obj).consp() && oCar(obj) == cl::_sym_quote)
    return true;
  if (obj.nilp())
    return true;
  if (cl__symbolp(obj)) {
    if (cl__keywordp(obj))
      return true;
    return gc::As<Symbol_sp>(obj)->isConstant();
  }
  return false;
};

CL_LAMBDA(arg);
CL_DECLARE();
CL_DOCSTRING("identity");
CL_DEFUN T_mv cl__identity(T_sp arg) {
  return (Values(arg));
};

CL_LAMBDA(macro-function form macro-env);
CL_DECLARE();
CL_DOCSTRING("macroexpand_default Default value of *macroexpand-hook*");
CL_DEFUN T_mv core__macroexpand_default(Function_sp macro_function, T_sp form, T_sp macro_env) {
  Function_sp debugMacroFunction = macro_function;
  T_sp debugForm = form;
  T_sp debugEnvironment = macro_env;
  T_sp tllh = macro_function->functionLambdaListHandler();
  if (LambdaListHandler_sp llh = tllh.asOrNull<LambdaListHandler_O>()) {
    if (llh->numberOfRequiredArguments() != 2) {
      stringstream err;
      err << __FILE__ << ":" << __LINE__ << " Caught a problem in af_macroexpand_default - the macro_function requires " << llh->numberOfRequiredArguments() << " arguments but I'm only going to pass 2!!!" << std::endl;
      err << "lambda_list: " << _rep_(llh) << std::endl;
      err << "Passing argument 1: " << _rep_(form) << std::endl;
      err << "Passing argument 2: " << _rep_(macro_env) << std::endl;
      Closure_sp closure = macro_function;
      err << "macro_function[" << _rep_(macro_function->name()) << std::endl;
      if (auto ic = closure.as<InterpretedClosure_O>()) {
        err << "code: " << _rep_(ic->code());
      } else {
        err << "macro_function is not an interpreted function";
      }
      SIMPLE_ERROR(BF("Wrong number of arguments %d within macroexpand_default when trying to invoke macro %s\nMore detail: %s") % llh->numberOfRequiredArguments() % _rep_(macro_function->functionName()) % err.str());
    }
  }
  T_sp result = eval::funcall(macro_function, form, macro_env);
  return (Values(result));
};

CL_LAMBDA(obj);
CL_DECLARE();
CL_DOCSTRING("null test - return true if the object is the empty list otherwise return nil");
CL_DEFUN T_mv cl__null(T_sp obj) {
  if (obj.nilp())
    return (Values(_lisp->_true()));
  return (Values(_Nil<T_O>()));
};

CL_LAMBDA(obj);
CL_DECLARE();
CL_DOCSTRING("return class of object - see CLHS");
CL_DEFUN Class_sp cl__class_of(T_sp obj) {
  Class_sp result = lisp_instance_class(obj);
  return (result);
}

SYMBOL_EXPORT_SC_(CorePkg,STARdebug_fsetSTAR);
CL_LAMBDA(function-name fn &optional is-macro pretty-print (lambda-list nil lambda-list-p));
CL_DECLARE();
CL_DOCSTRING(R"doc(* Arguments
- function-name :: The name of the function to bind.
- fn :: The function object.
- is-macro :: A boolean.
- pretty-print : A boolean.
- lambda-list : A lambda-list or nil.
- lambda-list-p : T if lambda-list is passed
* Description
Bind a function to the function slot of a symbol
- handles symbol function-name and (SETF XXXX) names. 
IS-MACRO defines if the function is a macro or not. 
PRETTY-PRINT was inherited from ecl - I don't know what its for.  
LAMBDA-LIST passes the lambda-list.)doc");
CL_DEFUN T_sp core__fset(T_sp functionName, Function_sp functor, T_sp is_macro, T_sp pretty_print, T_sp lambda_list, T_sp lambda_list_p) {
#ifdef DEBUG_DRAG
  if (_sym_STARdebug_fsetSTAR->symbolValue().isTrue()) {
    printf("%s:%d FSET called with\n   functionName: %s\n   function: %s\n   is_macro: %s\n   pretty_print: %s\n   lambda_list: %s\n   lambda_list_p: %s\n",
           __FILE__, __LINE__, _rep_(functionName).c_str(), _rep_(functor).c_str(), _rep_(is_macro).c_str(), _rep_(pretty_print).c_str(), _rep_(lambda_list).c_str(), _rep_(lambda_list_p).c_str());
  }
#endif // DEBUG_DRAG
  if ( NamedFunction_sp functionObject = functor.asOrNull<NamedFunction_O>() ) {
    if (is_macro.isTrue()) {
      functionObject->set_kind(kw::_sym_macro);
    } else {
      functionObject->set_kind(kw::_sym_function);
    }
    if ( lambda_list_p.notnilp() ) {
      functionObject->setf_lambda_list(lambda_list);
    }
    if (comp::_sym_STARall_functions_for_one_compileSTAR->boundP()
        && functionObject->compiledP()) {
      functionObject->setAssociatedFunctions(comp::_sym_STARall_functions_for_one_compileSTAR->symbolValue());
    }
  }
  if (cl__symbolp(functionName)) {
    Symbol_sp symbol = gc::As<Symbol_sp>(functionName);
    symbol->setf_symbolFunction(functor);
    return functor;
  } else if ((functionName).consp()) {
    SYMBOL_EXPORT_SC_(ClPkg, setf);
    List_sp cur = functionName;
    if (oCar(cur) == cl::_sym_setf) {
      Symbol_sp symbol = gc::As<Symbol_sp>(oCadr(cur));
      symbol->setSetfFdefinition(functor);
      return functor;
    }
  }
  SIMPLE_ERROR(BF("Illegal name for function[%s]") % _rep_(functionName));
};

CL_LAMBDA(function-name);
CL_DECLARE();
CL_DOCSTRING("fdefinition");
CL_DEFUN T_sp cl__fdefinition(T_sp functionName) {
  if (cl__symbolp(functionName)) {
    Symbol_sp sym = gc::As<Symbol_sp>(functionName);
    return sym->symbolFunction();
  } else if ((functionName).consp()) {
    List_sp cname = functionName;
    if (oCar(cname) == cl::_sym_setf) {
      Symbol_sp name = gc::As<Symbol_sp>(oCadr(cname));
      if (name.notnilp()) {
        return name->getSetfFdefinition();
      }
    }
  }
  SIMPLE_ERROR(BF("Illegal function-name[%s]") % _rep_(functionName));
}

CL_LAMBDA(function-name);
CL_DECLARE();
CL_DOCSTRING("fboundp");
CL_DEFUN bool cl__fboundp(T_sp functionName) {
  if (functionName.nilp()) {
    return false;
  }
  if (cl__symbolp(functionName)) {
    Symbol_sp sym = gc::As<Symbol_sp>(functionName);
    return sym->fboundp();
  } else if ((functionName).consp()) {
    List_sp cname = functionName;
    if (oCar(cname) == cl::_sym_setf) {
      Symbol_sp name = gc::As<Symbol_sp>(oCadr(cname));
      if (name.notnilp())
        return name->setf_fboundp();
    }
  }
  SIMPLE_ERROR(BF("Illegal function-name[%s]") % _rep_(functionName));
}

CL_LAMBDA(function-name);
CL_DECLARE();
CL_DOCSTRING("fmakunbound");
CL_DEFUN T_mv cl__fmakunbound(T_sp functionName) {
  if (cl__symbolp(functionName)) {
    Symbol_sp sym = gc::As<Symbol_sp>(functionName);
    sym->setf_symbolFunction(_Unbound<Function_O>());
    return (Values(sym));
  } else if ((functionName).consp()) {
    List_sp cname = functionName;
    if (oCar(cname) == cl::_sym_setf) {
      Symbol_sp name = gc::As<Symbol_sp>(oCadr(cname));
      if (name.notnilp()) {
        name->resetSetfFdefinition(); //_lisp->remove_setfDefinition(name);
        return (Values(functionName));
      }
    }
  }
  SIMPLE_ERROR(BF("Illegal function-name[%s]") % _rep_(functionName));
}

CL_LAMBDA(char &optional input-stream-designator recursive-p);
CL_DECLARE();
CL_DOCSTRING("read a list up to a specific character - see CLHS");
CL_DEFUN T_mv cl__read_delimited_list(Character_sp chr, T_sp input_stream_designator, T_sp recursive_p) {
  T_sp sin = coerce::inputStreamDesignator(input_stream_designator);
#if 0
	// I think it is safe to ignore recursive_p
  if ( recursive_p.isTrue() )
  {
    SIMPLE_ERROR(BF("Currently I don't handle recursive-p[true] for read_delimited_list"));
  }
#endif
  T_sp result = read_list(sin, clasp_as_char(chr), true);
  if (cl::_sym_STARread_suppressSTAR->symbolValue().isTrue()) {
    return (Values(_Nil<T_O>()));
  }
  return (Values(result));
}

CL_LAMBDA(&optional input-stream-designator (eof-error-p t) eof-value recursive-p);
CL_DECLARE();
CL_DOCSTRING("read an object from a stream - see CLHS");
CL_DEFUN T_sp cl__read(T_sp input_stream_designator, T_sp eof_error_p, T_sp eof_value, T_sp recursive_p) {
  bool preserve_whitespace = true;
  if ( recursive_p.isTrue() ) {
    preserve_whitespace = _sym_STARpreserve_whitespace_pSTAR->symbolValue().isTrue();
  } else {
    preserve_whitespace = false;
  }
  DynamicScopeManager scope(_sym_STARpreserve_whitespace_pSTAR, _lisp->_boolean(preserve_whitespace));
  T_sp sin = coerce::inputStreamDesignator(input_stream_designator);
  return read_lisp_object(sin, eof_error_p.isTrue(), eof_value, recursive_p.notnilp());
}

CL_LAMBDA(&optional input-stream-designator (eof-error-p t) eof-value recursive-p);
CL_DECLARE();
CL_DOCSTRING("read an object from a stream while preserving whitespace - see CLHS");
CL_DEFUN T_sp cl__read_preserving_whitespace(T_sp input_stream_designator, T_sp eof_error_p, T_sp eof_value, T_sp recursive_p) {
  bool preserve_whitespace = true;
  if ( recursive_p.isTrue() ) {
    preserve_whitespace = _sym_STARpreserve_whitespace_pSTAR->symbolValue().isTrue();
  } else {
    preserve_whitespace = true;
  }
  DynamicScopeManager scope(_sym_STARpreserve_whitespace_pSTAR, _lisp->_boolean(preserve_whitespace));
  T_sp sin = coerce::inputStreamDesignator(input_stream_designator);
  return read_lisp_object(sin, eof_error_p.isTrue(), eof_value, recursive_p.isTrue());
}

/* -------------------------------------------------------- */
/*     Sequence primitives                                  */


ListOfSequenceSteppers::ListOfSequenceSteppers(List_sp sequences) {
  this->_AtEnd = false;
  for (auto cur : sequences) {
    T_sp obj = oCar(cur);
    if (Vector_sp vobj = obj.asOrNull<Vector_O>()) {
      if (cl__length(vobj) == 0)
        goto EMPTY;
      VectorStepper_sp  vP(gc::GC<VectorStepper_O>::allocate(vobj));
      this->_Steppers.push_back(vP);
    } else if (Cons_sp cobj = obj.asOrNull<Cons_O>()) {
      ConsStepper_sp cP(gc::GC<ConsStepper_O>::allocate(cobj));
      this->_Steppers.push_back(cP);
    } else if (obj.nilp()) {
      goto EMPTY;
    } else if (obj.generalp()) {
      General_sp gobj((gctools::Tagged)obj.raw_());
      SIMPLE_ERROR(BF("Illegal object for stepper[%s] class[%s]") % _rep_(gobj) % gobj->_instanceClass()->classNameAsString());
    }
  }
  this->_AtEnd = false;
  return;
 EMPTY:
  this->_AtEnd = true;
}

void ListOfSequenceSteppers::fillValueFrameUsingCurrentSteppers(ActivationFrame_sp frame) const {
  if (this->_AtEnd)
    SIMPLE_ERROR(BF("Tried to make list of ended stepper"));
  int idx = 0;
  ValueFrame_sp vframe = frame.as<ValueFrame_O>();
  for (auto rit = this->_Steppers.begin(); rit != this->_Steppers.end(); rit++) {
    vframe->set_entry(idx, (*rit)->element());
    ++idx;
  }
}

bool ListOfSequenceSteppers::advanceSteppers() {
  _OF();
  if (this->_AtEnd)
    SIMPLE_ERROR(BF("Tried to advance ended stepper"));
  for (auto it = this->_Steppers.begin(); it != this->_Steppers.end(); it++) {
    this->_AtEnd |= (*it)->advance();
  }
  return !this->_AtEnd;
}

class ListOfListSteppers : public ListOfSequenceSteppers {
public:
  ListOfListSteppers(List_sp lists);
  virtual ~ListOfListSteppers(){};
};

ListOfListSteppers::ListOfListSteppers(List_sp sequences) {
  for (auto cur : sequences) {
    T_sp obj = oCar(cur);
    if (Cons_sp cobj = obj.asOrNull<Cons_O>()) {
      ConsStepper_sp  cP(gc::GC<ConsStepper_O>::allocate(cobj));
      this->_Steppers.push_back(cP);
    } else {
      goto EMPTY;
    }
  }
  this->_AtEnd = false;
  return;
 EMPTY:
  this->_AtEnd = true;
  return;
}

bool test_every_some_notevery_notany(Function_sp predicate, List_sp sequences, bool elementTest, bool elementReturn, bool fallThroughReturn, T_sp &retVal) {
  ListOfSequenceSteppers steppers(sequences);
  ValueFrame_sp frame(ValueFrame_O::create(steppers.size(), _Nil<T_O>()));
//  printf("%s:%d  %s  frame->length() = %d\n", __FILE__, __LINE__, __FUNCTION__,frame->length());
//  printf("        frame->_Objects.capacity() = %d\n", frame->_Objects.capacity());
  if (steppers.atEnd())
    goto FALLTHROUGH; // return elementReturn;
  while (!steppers.atEnd()) {
    steppers.fillValueFrameUsingCurrentSteppers(frame);
    LOG(BF("Applying predicate to elements[%s]") % frame->asString());
    retVal = eval::applyToActivationFrame(predicate, frame);
    bool test = retVal.isTrue();
    if (test == elementTest) {
      LOG(BF("element test was %d - returning %d") % elementTest % elementReturn);
      return elementReturn;
    }
    steppers.advanceSteppers();
  }
  LOG(BF("passed-through - returning %d") % fallThroughReturn);
 FALLTHROUGH:
  return fallThroughReturn;
}

CL_LAMBDA(predicate &rest sequences);
CL_DECLARE();
CL_DOCSTRING("See CLHS for every");
CL_DEFUN T_sp cl__every(T_sp predicate, List_sp sequences) {
  Function_sp op = coerce::functionDesignator(predicate);
  T_sp dummy;
  bool result = test_every_some_notevery_notany(op, sequences, false, false, true, dummy);
  return _lisp->_boolean(result);
}

CL_LAMBDA(predicate &rest sequences);
CL_DECLARE();
CL_DOCSTRING("See CLHS for some");
CL_DEFUN T_sp cl__some(T_sp predicate, List_sp sequences) {
  Function_sp op = coerce::functionDesignator(predicate);
  T_sp retVal;
  bool result = test_every_some_notevery_notany(op, sequences, true, true, false, retVal);
  if (result)
    return retVal;
  return _Nil<T_O>();
}

CL_LAMBDA(predicate &rest sequences);
CL_DECLARE();
CL_DOCSTRING("See CLHS for notany");
CL_DEFUN T_sp cl__notany(T_sp predicate, List_sp sequences) {
  Function_sp op = coerce::functionDesignator(predicate);
  T_sp dummy;
  bool result = test_every_some_notevery_notany(op, sequences, true, false, true, dummy);
  return _lisp->_boolean(result);
}

CL_LAMBDA(predicate &rest sequences);
CL_DECLARE();
CL_DOCSTRING("See CLHS for notevery");
CL_DEFUN T_sp cl__notevery(T_sp predicate, List_sp sequences) {
  Function_sp op = coerce::functionDesignator(predicate);
  T_sp dummy;
  bool result = test_every_some_notevery_notany(op, sequences, false, true, false, dummy);
  return _lisp->_boolean(result);
}

/*
  __BEGIN_DOC(candoScript.general.mapcar)
  __END_DOC
*/
SYMBOL_EXPORT_SC_(ClPkg, mapcar);

CL_LAMBDA(func-desig &rest lists);
CL_DECLARE();
CL_DOCSTRING("See CLHS for mapcar");
CL_DEFUN T_sp cl__mapcar(T_sp func_desig, List_sp lists) {
  Function_sp func = coerce::functionDesignator(func_desig);
  ListOfListSteppers steppers(lists);
  ValueFrame_sp frame(ValueFrame_O::create(steppers.size(), _Nil<ActivationFrame_O>()));
  ql::list result(_lisp);
  while (!steppers.atEnd()) {
    steppers.fillValueFrameUsingCurrentSteppers(frame);
    T_sp res = eval::applyToActivationFrame(func, frame);
    result << res;
    steppers.advanceSteppers();
  }
  return result.cons();
}

/*
  __BEGIN_DOC(candoScript.general.mapcar)
  __END_DOC
*/
CL_LAMBDA(op &rest lists);
CL_DECLARE();
CL_DOCSTRING("See CLHS mapc");
CL_DEFUN T_sp cl__mapc(T_sp top, List_sp lists) {
  Function_sp op = coerce::functionDesignator(top);
  VectorObjectsWithFillPtr_sp argumentLists(VectorObjectsWithFillPtr_O::make(_Nil<T_O>(), _Nil<T_O>(), 8, 0, true, cl::_sym_T_O));
  // Copy the arguments into argumentLists
  for (auto carg : lists) {
    argumentLists->vectorPushExtend(oCar(carg), 8);
  }
  List_sp result, curResult;
  ValueFrame_sp frame(ValueFrame_O::create(cl__length(argumentLists), _Nil<ActivationFrame_O>()));
  while (1) {
    int idx = 0;
    for (size_t it(0), itEnd(cl__length(argumentLists)); it < itEnd; ++it) {
      if (argumentLists->operator[](it).nilp()) {
        // We hit a nil - jump to the end
        goto RETURN;
      }
      frame->set_entry(idx, oCar(argumentLists->operator[](it)));
      argumentLists->operator[](it) = oCdr(argumentLists->operator[](it));
      ++idx;
    }
    LOG(BF("About to evaluate map op[%s] on arguments[%s]") % _rep_(op) % _rep_(frame));
    T_sp res = eval::applyToActivationFrame(op, frame);
  }
 RETURN:
  return oCar(lists);
}

CL_LAMBDA(func-desig &rest lists);
CL_DECLARE();
CL_DOCSTRING("See CLHS maplist");
CL_DEFUN T_sp cl__maplist(T_sp func_desig, List_sp lists) {
  //        printf("%s:%d maplist func_desig=%s   lists=%s\n", __FILE__, __LINE__, _rep_(func_desig).c_str(), _rep_(lists).c_str() );
  Function_sp op = coerce::functionDesignator(func_desig);
  VectorObjectsWithFillPtr_sp argumentLists(VectorObjectsWithFillPtr_O::make(_Nil<T_O>(), _Nil<T_O>(), 16, 0, true, cl::_sym_T_O));
  //	vector<List_sp> argumentLists;
  // Copy the arguments into argumentLists
  for (auto carg : lists) {
    argumentLists->vectorPushExtend(oCar(carg), 8);
    //	    argumentLists.push_back(oCar(carg));
  }
  //        printf("%s:%d  argumentLists = %s\n", __FILE__, __LINE__, _rep_(argumentLists).c_str() );
  List_sp result, curResult;
  result = Cons_O::create(_Nil<T_O>(), _Nil<T_O>());
  ValueFrame_sp frame(ValueFrame_O::create(cl__length(argumentLists), _Nil<ActivationFrame_O>()));
  curResult = result;
  while (1) {
    int idx = 0;
    //            printf("%s:%d  length(argumentLists) = %d   argumentLists->fillPointer()=%d\n", __FILE__, __LINE__, cl__length(argumentLists), argumentLists->fillPointer() );
    for (int it(0), itEnd(cl__length(argumentLists)); it < itEnd; ++it) {
      T_sp val = (*argumentLists)[it];
      if (val.nilp())
        goto RETURN; // hit nil in arguments - exit
      frame->set_entry(idx, val);
      idx++;
    }
    //            printf("%s:%d op %s on frame: %s\n", __FILE__, __LINE__, _rep_(op).c_str(), _rep_(frame).c_str() );
    LOG(BF("About to evaluate map op[%s] on arguments[%s]") % _rep_(op) % _rep_(frame));
    T_sp res = eval::applyToActivationFrame(op, frame);
    Cons_sp one = Cons_O::create(res);
    curResult.asCons()->setCdr(one);
    curResult = one;
    // Advance to the next element
    for (int it(0), itEnd(cl__length(argumentLists)); it < itEnd; ++it) {
      argumentLists->operator[](it) = oCdr(argumentLists->operator[](it));
      //		*it = cCdr((*it));
    }
  }
 RETURN:
  return oCdr(result);
}

CL_LAMBDA(op &rest lists);
CL_DECLARE();
CL_DOCSTRING("See CLHS maplist");
CL_DEFUN T_sp cl__mapl(T_sp top, List_sp lists) {
  Function_sp op = coerce::functionDesignator(top);
  cl__maplist(op, lists);
  return oCar(lists);
}

CL_LAMBDA(fun &rest cargs);
CL_DECLARE();
CL_DOCSTRING("mapappend is like mapcar except that the results are appended together - see AMOP 280");
CL_DEFUN T_mv core__mapappend(Function_sp fun, List_sp cargs) {
  IMPLEMENT_MEF(BF("Fix me - I think I'm broken"));
  T_sp testNull = eval::funcall(cl::_sym_some, cl::_sym_null->symbolFunction(), cargs);
  if (testNull.nilp())
    return (Values(_Nil<T_O>()));
  T_sp arg0 = eval::funcall(cl::_sym_mapcar, cl::_sym_car->symbolFunction(), cargs);
  T_sp appendHead = eval::funcall(fun, arg0);
  T_sp arg2 = eval::funcall(cl::_sym_mapcar, cl::_sym_cdr->symbolFunction(), cargs);
  T_sp appendTail = eval::funcall(_sym_mapappend, fun, arg2);
  return eval::funcall(cl::_sym_append, appendHead, appendTail);
};

CL_LAMBDA(op &rest lists);
CL_DECLARE();
CL_DOCSTRING("mapcon");
CL_DEFUN T_mv cl__mapcon(T_sp op, List_sp lists) {
  List_sp parts = cl__maplist(op, lists);
  T_sp result = cl__nconc(parts);
  return (Values(result));
};

CL_LAMBDA(op &rest lists);
CL_DECLARE();
CL_DOCSTRING("mapcan");
CL_DEFUN T_mv cl__mapcan(T_sp op, List_sp lists) {
  List_sp parts = cl__mapcar(op, lists);
  T_sp result = cl__nconc(parts);
#if 0
  ValueFrame_sp frame(ValueFrame_O::create(parts,_Nil<ActivationFrame_O>()));
  T_sp result = eval::applyToActivationFrame(cl::_sym_nconc,frame);
#endif
  return (Values(result));
};



/*!
  Equivalent to Common Lisps append function
  (append a b c)
  It recreates the list structures of the first arguments a and b and strings
  them together into one list and then points the cdr of the last element of this new list
  to c.
*/
CL_LAMBDA(&rest lists);
CL_DECLARE();
CL_DOCSTRING("append as in clhs");
CL_DEFUN List_sp cl__append(List_sp lists) {
  ql::list list;
  LOG(BF("Carrying out append with arguments: %s") % _rep_(lists));
  auto it = lists.begin();
  auto end = lists.end();
  T_sp curit = *it;
  while (it != end) {
    curit = *it;
    it++;
    if (it == end)
      break;
    for (auto inner : (List_sp)oCar(curit)) {
      list << oCar(inner);
    }
  }
  /* Now append the last argument by setting the new lists last element cdr
       to the last argument of append */
  T_sp last = oCar(curit);
  list.dot(last);
  T_sp res = list.cons();
  return res;
}

CL_LAMBDA(func sequence start end);
CL_DECLARE();
CL_DOCSTRING("Copied from ecl::sequence.d::sequence_start_end - throws errors if start/end are out of range for the sequence. I'm not sure what the func argument is for. If end is nil then it is set to the end of the sequence.  Return MultipleValues(start,end,length).");
CL_DEFUN T_mv core__sequence_start_end(T_sp func, T_sp sequence, Fixnum_sp start, T_sp end) {
  uint len = cl__length(sequence);
  if (end.nilp())
    end = make_fixnum(len);
  Fixnum_sp fnend = gc::As<Fixnum_sp>(end);
  if (unbox_fixnum(start) < 0) {
    SIMPLE_ERROR(BF("start[%d] must be greater than zero") % _rep_(start));
  }
  if (unbox_fixnum(fnend) > len) {
    SIMPLE_ERROR(BF("end[%d] must be <= length of sequence[%d]") % _rep_(end) % len);
  }
  Fixnum_sp length = make_fixnum(len);
  if (unbox_fixnum(fnend) < unbox_fixnum(start)) {
    SIMPLE_ERROR(BF("end[%d] is less than start[%d]") % _rep_(end) % _rep_(start));
  }
  return (Values(start, fnend, length));
};


CL_LAMBDA(&optional x);
CL_DECLARE();
CL_DOCSTRING("See CLHS gensym");
CL_DEFUN Symbol_mv cl__gensym(T_sp x) {
  stringstream ss;
  if (x.nilp()) {
    int counter = unbox_fixnum(gc::As<Fixnum_sp>(cl::_sym_STARgensym_counterSTAR->symbolValue()));
    cl::_sym_STARgensym_counterSTAR->setf_symbolValue(make_fixnum(counter + 1));
    ss << "G";
    ss << counter;
  } else if (cl__stringp(x)) {
    int counter = unbox_fixnum(gc::As<Fixnum_sp>(cl::_sym_STARgensym_counterSTAR->symbolValue()));
    cl::_sym_STARgensym_counterSTAR->setf_symbolValue(make_fixnum(counter + 1));
    ss << gc::As<Str_sp>(x)->get();
    ss << counter;
  } else if (cl__integerp(x)) {
    int counter = clasp_to_int(gc::As<Integer_sp>(x));
    ASSERTF(counter >= 0, BF("gensym argument %d must be >= 0") % counter);
    ss << "G";
    ss << counter;
  } else {
    SIMPLE_ERROR(BF("Illegal argument for gensym[%s]") % _rep_(x));
  }
  Symbol_sp sym = Symbol_O::create(ss.str());
  sym->setPackage(_Nil<Package_O>());
  return (Values(sym));
}

CL_LAMBDA(x);
CL_DECLARE();
CL_DOCSTRING("type_to_symbol");
CL_DEFUN Symbol_mv core__type_to_symbol(T_sp x) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
  if (x.fixnump())
    return (Values(cl::_sym_fixnum));
  else if ( x.characterp() )
    return (Values(cl::_sym_character));
  else if ( x.single_floatp() ) 
    return (Values(cl::_sym_single_float));
  else if (x.consp())
    return (Values(cl::_sym_list));
  else if (x.generalp()) {
    General_sp gx(x.unsafe_general());
    if (DoubleFloat_sp dfx = gx.asOrNull<DoubleFloat_O>())
      return (Values(cl::_sym_DoubleFloat_O));
    else if (Symbol_sp sx = gx.asOrNull<Symbol_O>())
      return (Values(cl::_sym_Symbol_O));
    else if (gx.nilp())
      return (Values(cl::_sym_Symbol_O)); // Return _sym_null??
    else if (Bignum_sp bnx = gx.asOrNull<Bignum_O>())
      return (Values(cl::_sym_Bignum_O));
    else if (Ratio_sp rx = gx.asOrNull<Ratio_O>())
      return (Values(cl::_sym_Ratio_O));
#ifdef CLASP_LONG_FLOAT
    else if (LongFloat_sp lfx = gx.asOrNull<LongFloat_O>())
      return (Values(cl::_sym_LongFloat_O));
#endif
    else if (Complex_sp cx = gx.asOrNull<Complex_O>())
      return (Values(cl::_sym_Complex_O));
    else if (Package_sp px = gx.asOrNull<Package_O>())
      return (Values(cl::_sym_Package_O));
    else if (HashTable_sp htx = gx.asOrNull<HashTable_O>())
      return (Values(cl::_sym_HashTable_O));
    else if (Vector_sp vx = gx.asOrNull<Vector_O>())
      return (Values(cl::_sym_Vector_O));
    else if (BitVector_sp bvx = gx.asOrNull<BitVector_O>())
      return (Values(cl::_sym_BitVector_O));
    else if (Array_sp ax = gx.asOrNull<Array_O>())
      return (Values(cl::_sym_Array_O));
    else if (Str_sp strx = gx.asOrNull<Str_O>())
      return (Values(cl::_sym_String_O));
  //    else if ( x.isA<BaseString_O>() ) return(Values(_sym_BaseString_O));
    else if (Stream_sp streamx = gx.asOrNull<Stream_O>())
      return (Values(cl::_sym_Stream_O));
    else if (ReadTable_sp rtx = gx.asOrNull<ReadTable_O>())
      return (Values(cl::_sym_ReadTable_O));
    return Values(gx->__class()->className());
  }
  SIMPLE_ERROR(BF("Add core__type_to_symbol support for type: %s") % cl__class_of(x)->classNameAsString());
#pragma clang diagnostic pop
}

T_sp type_of(T_sp x) {
  if (x.fixnump()) {
    ql::list res(_lisp);
    res << cl::_sym_integer << x << x;
    return res.cons();
  } else if (x.consp()) {
    return cl::_sym_cons;
  } else if (x.single_floatp()) {
    return cl::_sym_single_float;
  } else if (x.valistp() ) {
    return core::_sym_valist;
  } else if (x.characterp()) {
    if (cl__standard_char_p(gc::As<Character_sp>(x)))
      return cl::_sym_standard_char;
    return cl::_sym_character;
  } else if (Integer_sp ix = x.asOrNull<Integer_O>()) {
    ql::list res(_lisp);
    res << cl::_sym_integer << ix << ix;
    return res.cons();
  }
#ifdef CLOS
  if (Instance_sp instance = x.asOrNull<Instance_O>()) {
    T_sp cl = lisp_instance_class(instance);
    T_sp t;
    if (Class_sp mcl = cl.asOrNull<Class_O>()) {
      t = mcl->className();
    } else if (Instance_sp icl = cl.asOrNull<Instance_O>()) {
      (void)icl;
      DEPRECIATEDP("Classes of instances should always be of Class_O type, not Instance_O");
      //	    t = icl->_CLASS_NAME();
    } else {
      SIMPLE_ERROR(BF("Illegal class %s for instance class of %s") % _rep_(cl) % _rep_(instance));
    }
    Symbol_sp st = gc::As<Symbol_sp>(t);
    if (t.nilp() || cl != T_sp(eval::funcall(cl::_sym_findClass, st, _Nil<T_O>()))) {
      t = cl;
    }
    return t;
  } else if (Class_sp mc = x.asOrNull<Class_O>()) {
    Class_sp mcc = lisp_static_class(mc);
    return mcc->className();
  }
#endif
  if (Symbol_sp symx = x.asOrNull<Symbol_O>()) {
    if (x.nilp()) return cl::_sym_null;
    if (x == _lisp->_true())
      return cl::_sym_boolean;
    if (cl__keywordp(symx))
      return cl::_sym_keyword;
    return cl::_sym_symbol;
  } else if (String_sp sx = x.asOrNull<String_O>()) {
    Symbol_sp t;
    if (sx->adjustable_array_p() || sx->array_has_fill_pointer_p() || sx->_displaced_array_p()) {
      t = cl::_sym_array;
    } else t = cl::_sym_simple_array;
    return (ql::list(_lisp) << t << cl::_sym_base_char << Cons_O::createList(make_fixnum(1), make_fixnum(cl__length(sx)))).cons();
  } else if (Vector_sp vx = x.asOrNull<Vector_O>()) {
    if (vx->adjustable_array_p() || vx->_displaced_array_p()) {
      return (ql::list(_lisp) << cl::_sym_vector << vx->element_type_as_symbol() << vx->arrayDimensions()).cons();
    } else if (vx->array_has_fill_pointer_p() /* || (cl__elttype)x->vector.elttype != aet_object) */) {
      return (ql::list(_lisp) << cl::_sym_simple_array << vx->element_type_as_symbol() << vx->arrayDimensions()).cons();
    } else {
      return (ql::list(_lisp) << cl::_sym_simple_vector << make_fixnum(cl__length(vx))).cons();
    }
  } else if (Array_sp ax = x.asOrNull<Array_O>()) {
    Symbol_sp t;
    if (ax->adjustable_array_p() || ax->_displaced_array_p()) {
      t = cl::_sym_array;
    } else
      t = cl::_sym_simple_array;
    return (ql::list(_lisp) << t << ax->element_type_as_symbol() << ax->arrayDimensions()).cons();
  } else if (BitVector_sp bx = x.asOrNull<BitVector_O>()) {
    Symbol_sp t;
    if (bx->adjustable_array_p() || bx->array_has_fill_pointer_p() || bx->_displaced_array_p()) {
      t = cl::_sym_array;
    } else
      t = cl::_sym_simple_array;
    return (ql::list(_lisp) << t << cl::_sym_bit << Cons_O::createList(make_fixnum(1), make_fixnum(cl__length(bx)))).cons();
  } else if (WrappedPointer_sp pp = x.asOrNull<WrappedPointer_O>()) {
    return pp->_instanceClass()->className();
  } else if (core__structurep(x)) {
    return gc::As<StructureObject_sp>(x)->structureType();
  } else if (Stream_sp stx = x.asOrNull<Stream_O>()) {
    if (gc::IsA<SynonymStream_sp>(stx))
      return cl::_sym_SynonymStream_O;
    else if (gc::IsA<BroadcastStream_sp>(stx))
      return cl::_sym_BroadcastStream_O;
    else if (gc::IsA<ConcatenatedStream_sp>(stx))
      return cl::_sym_ConcatenatedStream_O;
    else if (gc::IsA<TwoWayStream_sp>(stx))
      return cl::_sym_TwoWayStream_O;
    else if (gc::IsA<StringInputStream_sp>(stx))
      return _sym_StringInputStream_O;
    else if (gc::IsA<StringOutputStream_sp>(stx))
      return _sym_StringOutputStream_O;
    else if (gc::IsA<EchoStream_sp>(stx))
      return cl::_sym_EchoStream_O;
    else
      return cl::_sym_FileStream_O;
  } else if (Pathname_sp px = x.asOrNull<Pathname_O>()) {
    if (core__logical_pathname_p(px)) {
      return cl::_sym_logical_pathname;
    } else {
      return cl::_sym_pathname;
    }
  }
  return core__type_to_symbol(x);
}

CL_LAMBDA(obj);
CL_DECLARE();
CL_DOCSTRING("type_of");
CL_DEFUN T_sp cl__type_of(T_sp x) {
  return type_of(x);
}

CL_LAMBDA(obj);
CL_DECLARE();
CL_DOCSTRING("sxhash");
CL_DEFUN Integer_sp cl__sxhash(T_sp obj) {
  if (obj.nilp())
    return make_fixnum(1);
  HashGenerator hg;
  clasp_sxhash(obj, hg);
  return Integer_O::create(hg.hash());
}

// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------

}; /* core */

namespace core {



bool satisfiesTest(InvocationHistoryFrameIterator_sp iterator, T_sp test) {
  if (!iterator->isValid()) {
    SIMPLE_ERROR(BF("Invalid InvocationHistoryFrameIterator"));
  }
  if (test.nilp())
    return true;
  T_sp res = eval::funcall(test, iterator);
  return res.isTrue();
}

void nextInvocationHistoryFrameIteratorThatSatisfiesTest(Fixnum num, InvocationHistoryFrameIterator_sp iterator, T_sp test) {
  do {
    if (!iterator->isValid())
      return;
    if (satisfiesTest(iterator, test)) {
      if (num == 0)
        return;
      --num;
    }
    iterator->move_to_previous_frame();
  } while (num >= 0);
}

int backtrace_length(InvocationHistoryFrame* frame) {
  int length = 0;
  while ( (frame = frame->previous()) ) {++length;};
  return length;
}

CL_LISPIFY_NAME(make-invocation-history-frame-iterator);
CL_DEFUN InvocationHistoryFrameIterator_sp InvocationHistoryFrameIterator_O::make(Fixnum first, T_sp test) {
  InvocationHistoryFrame *top = my_thread->_InvocationHistoryStack;
  int length = backtrace_length(top);
  InvocationHistoryFrameIterator_sp iterator = InvocationHistoryFrameIterator_O::create(top,length);
  nextInvocationHistoryFrameIteratorThatSatisfiesTest(first, iterator, test);
  return iterator;
}

CL_LISPIFY_NAME("frameIteratorPreviousFrame");
CL_DEFMETHOD InvocationHistoryFrameIterator_sp InvocationHistoryFrameIterator_O::prev(T_sp test) {
  nextInvocationHistoryFrameIteratorThatSatisfiesTest(1, this->asSmartPtr(), test);
  return this->asSmartPtr();
}

CL_LISPIFY_NAME("frameIteratorFunctionName");
CL_DEFMETHOD T_sp InvocationHistoryFrameIterator_O::functionName() {
  if (!this->isValid()) {
    SIMPLE_ERROR(BF("Invalid InvocationHistoryFrameIterator"));
  }
  Function_sp closure = this->_Frame->function();
  if (!closure) {
    SIMPLE_ERROR(BF("Could not access closure of InvocationHistoryFrame"));
  }
  return closure->name();
}

CL_LISPIFY_NAME("frameIteratorEnvironment");
CL_DEFMETHOD T_sp InvocationHistoryFrameIterator_O::environment() {
  if (!this->isValid()) {
    SIMPLE_ERROR(BF("Invalid InvocationHistoryFrameIterator"));
  }
  T_sp closure = this->_Frame->function();
  return closure;
}

int InvocationHistoryFrameIterator_O::index() {
  if (!this->isValid()) {
    SIMPLE_ERROR(BF("Invalid InvocationHistoryFrameIterator"));
  }
  return this->_Index;
}

Function_sp InvocationHistoryFrameIterator_O::function() {
  if (!this->isValid()) {
    SIMPLE_ERROR(BF("Invalid InvocationHistoryFrameIterator"));
  }
  Function_sp closure = this->_Frame->function();
  if (!closure) {
    SIMPLE_ERROR(BF("Could not access closure of InvocationHistoryFrame"));
  }
  return closure;
}

CL_LISPIFY_NAME("frameIteratorArguments");
CL_DEFMETHOD Vector_sp InvocationHistoryFrameIterator_O::arguments() {
  if (!this->isValid()) {
    SIMPLE_ERROR(BF("Invalid InvocationHistoryFrameIterator"));
  }
  InvocationHistoryFrame *frame = this->_Frame;
  return frame->arguments();
}

SYMBOL_SC_(CorePkg, makeInvocationHistoryFrameIterator);

;


SYMBOL_EXPORT_SC_(CorePkg, STARbacktraceFrameSelectorHookSTAR);
SYMBOL_EXPORT_SC_(KeywordPkg, next);
SYMBOL_EXPORT_SC_(KeywordPkg, prev);

CL_LAMBDA(idx direction);
CL_DECLARE();
CL_DOCSTRING("getInvocationHistoryFrameSearch - Return an InvocationHistoryFrame as an iterator. If idx == NIL return the top frame. If idx>=0 return the frame that satisfies *backtrace-frame-selector-hook* that has the index idx if direction==NIL, or if direction==:PREV return the frame previous to it (away from the top) or if direction==:NEXT the next frame (towards the top). *backtrace-frame-selector-hook* is a function that takes an invocation-history-frame-iterator and returns true if it should be in the backtrace. Test the result to make sure it is valid.");
CL_DEFUN InvocationHistoryFrameIterator_sp core__get_invocation_history_frame_search(T_sp idx, Symbol_sp direction) {
  T_sp backtraceFrameSelectorHook = core::_sym_STARbacktraceFrameSelectorHookSTAR->symbolValue();
  InvocationHistoryFrameIterator_sp top = InvocationHistoryFrameIterator_O::make(0, backtraceFrameSelectorHook);
  if (idx.nilp())
    return top;
  if (!idx.fixnump()) {
    SIMPLE_ERROR(BF("The first argument must be nil (for top) or a fixnum >= 0 - was given: %s") % _rep_(idx));
  }
  Fixnum fidx = idx.unsafe_fixnum();
  InvocationHistoryFrameIterator_sp lastSelectedFrame = top;
  InvocationHistoryFrameIterator_sp cur = top;
  while (cur->isValid() && fidx != cur->index()) {
    lastSelectedFrame = cur;
    cur = cur->prev(backtraceFrameSelectorHook);
  }
  if (direction.nilp())
    return cur;
  if (direction == kw::_sym_next)
    return lastSelectedFrame;
  if (direction == kw::_sym_prev)
    return cur->prev(backtraceFrameSelectorHook);
  SIMPLE_ERROR(BF("Direction argument must be one of NIL, :NEXT, :PREV - received %s") % _rep_(direction));
}

CL_LAMBDA();
CL_DECLARE();
CL_DOCSTRING("getInvocationHistoryFrameSearch - Return an top InvocationHistoryFrame as an iterator.");
CL_DEFUN InvocationHistoryFrameIterator_sp core__get_invocation_history_frame_top() {
  return core__get_invocation_history_frame_search(_Nil<T_O>(), _Nil<T_O>());
}

CL_LAMBDA();
CL_DECLARE();
CL_DOCSTRING("getInvocationHistoryFrame - Return an indexed InvocationHistoryFrame as an iterator.");
CL_DEFUN InvocationHistoryFrameIterator_sp core__get_invocation_history_frame(int idx) {
  Fixnum_sp fnidx = clasp_make_fixnum(idx);
  return core__get_invocation_history_frame_search(fnidx, _Nil<T_O>());
}

CL_LAMBDA();
CL_DECLARE();
CL_DOCSTRING("getInvocationHistoryFramePrev - Return the prev InvocationHistoryFrame before index as an iterator.");
CL_DEFUN InvocationHistoryFrameIterator_sp core__get_invocation_history_frame_prev(int idx) {
  Fixnum_sp fnidx = clasp_make_fixnum(idx);
  return core__get_invocation_history_frame_search(fnidx, kw::_sym_prev);
}

CL_LAMBDA();
CL_DECLARE();
CL_DOCSTRING("getInvocationHistoryFrameNext - Return the next InvocationHistoryFrame after index as an iterator.");
CL_DEFUN InvocationHistoryFrameIterator_sp core__get_invocation_history_frame_next(int idx) {
  Fixnum_sp fnidx = clasp_make_fixnum(idx);
  return core__get_invocation_history_frame_search(fnidx, kw::_sym_next);
}
};

namespace core {
CL_LAMBDA();
CL_DECLARE();
CL_DOCSTRING("ihsBacktraceNoArgs");
CL_DEFUN void core__ihs_backtrace_no_args() {
  core__ihs_backtrace(_lisp->_true(), _Nil<core::T_O>());
};

CL_LAMBDA();
CL_DECLARE();
CL_DOCSTRING("ihsTop");
CL_DEFUN int core__ihs_top() {
  InvocationHistoryFrameIterator_sp top = core__get_invocation_history_frame_top();
  if (!top->isValid()) return 0;
  return top->index();
};

CL_LAMBDA(cur);
CL_DECLARE();
CL_DOCSTRING("ihsPrev");
CL_DEFUN int core__ihs_prev(int idx) {
  InvocationHistoryFrameIterator_sp prev = core__get_invocation_history_frame_prev(idx);
  if (!prev->isValid())
    return 0;
  return prev->index();
};

CL_LAMBDA(cur);
CL_DECLARE();
CL_DOCSTRING("ihsNext");
CL_DEFUN int core__ihs_next(int idx) {
  InvocationHistoryFrameIterator_sp next = core__get_invocation_history_frame_next(idx);
  if (!next->isValid())
    return 0;
  return next->index();
}

CL_LAMBDA(arg);
CL_DECLARE();
CL_DOCSTRING("ihsFun: return the function in the invocation history stack at i");
CL_DEFUN T_sp core__ihs_fun(int idx) {
  InvocationHistoryFrameIterator_sp cur = core__get_invocation_history_frame(idx);
  if (!cur->isValid())
    return _Nil<T_O>();
  return cur->function();
};

CL_LAMBDA(arg);
CL_DECLARE();
CL_DOCSTRING("ihsArguments: return the arguments to the function in the invocation history stack at i");
CL_DEFUN T_sp core__ihs_arguments(int idx) {
  InvocationHistoryFrameIterator_sp cur = core__get_invocation_history_frame(idx);
  if (!cur->isValid())
    return _Nil<T_O>();
  return cur->arguments();
};

CL_LAMBDA(cur);
CL_DECLARE();
CL_DOCSTRING("ihsEnv");
CL_DEFUN T_sp core__ihs_env(int idx) {
  InvocationHistoryFrameIterator_sp cur = core__get_invocation_history_frame(idx);
  return _Nil<T_O>();
#if 0
  if (!cur->isValid())
    return _Nil<T_O>();
  return cur->environment();
#endif
};

CL_LAMBDA(cur);
CL_DECLARE();
CL_DOCSTRING("ihsBds");
CL_DEFUN int core__ihs_bds(int idx) {
  InvocationHistoryFrameIterator_sp cur = core__get_invocation_history_frame(idx);
  if (!cur->isValid())
    return 0;
  return cur->frame()->bds();
};

CL_LAMBDA();
CL_DECLARE();
CL_DOCSTRING("ihsCurrentFrame");
CL_DEFUN int core__ihs_current_frame() {
  T_sp cf = _sym_STARihsCurrentSTAR->symbolValue();
  if (cf.nilp()) {
    int icf = core__ihs_top();
    return core__set_ihs_current_frame(icf);
  }
  int icf = unbox_fixnum(gc::As<Fixnum_sp>(cf));
  if (icf < 0) {
    _sym_STARihsCurrentSTAR->setf_symbolValue(make_fixnum(icf));
    return 0;
  }
  if (icf >= core__ihs_top()) {
    _sym_STARihsCurrentSTAR->setf_symbolValue(make_fixnum(core__ihs_top()));
    return core__ihs_top();
  }
  return icf;
}

CL_LAMBDA();
CL_DECLARE();
CL_DOCSTRING("setIhsCurrentFrame");
CL_DEFUN int core__set_ihs_current_frame(int icf) {
  if (icf < 0)
    icf = 0;
  else if (icf >= core__ihs_top())
    icf = core__ihs_top();
  _sym_STARihsCurrentSTAR->setf_symbolValue(make_fixnum(icf));
  return icf;
}

CL_LAMBDA();
CL_DECLARE();
CL_DOCSTRING("bdsTop");
CL_DEFUN int core__bds_top() {
  return my_thread->bindings().top();
};

CL_LAMBDA(idx);
CL_DECLARE();
CL_DOCSTRING("bdsVar");
CL_DEFUN Symbol_sp core__bds_var(int idx) {
  return my_thread->bindings().var(idx);
};

CL_LAMBDA(idx);
CL_DECLARE();
CL_DOCSTRING("bdsVal");
CL_DEFUN T_sp core__bds_val(int idx) {
  return my_thread->bindings().val(idx);
};

CL_LAMBDA();
CL_DECLARE();
CL_DOCSTRING("exceptionStack");
CL_DEFUN Vector_sp core__exception_stack() {
  return my_thread->exceptionStack().backtrace();
}

CL_LAMBDA();
CL_DECLARE();
CL_DOCSTRING("exceptionStackDump");
CL_DEFUN void core__exception_stack_dump() {
  ExceptionStack &stack = my_thread->exceptionStack();
  printf("Exception stack size: %zu members\n", stack.size());
  for (int i(0); i < stack.size(); ++i) {
    string kind;
    switch (stack[i]._FrameKind) {
    case CatchFrame:
      kind = "catch";
      break;
    case BlockFrame:
      kind = "block";
      break;
    case TagbodyFrame:
      kind = "tagbody";
      break;
    default:
      kind = "unknown";
      break;
    };
    printf("Exception exceptionstack[%2d] = %8s %s@%p\n", i, kind.c_str(), _rep_(stack[i]._Key).c_str(), stack[i]._Key.raw_());
  }
  printf("----Done----\n");
};

CL_LAMBDA();
CL_DECLARE();
CL_DOCSTRING("dynamicBindingStackDump");
CL_DEFUN void core__dynamic_binding_stack_dump(std::ostream &out) {
  DynamicBindingStack &bd = my_thread->bindings();
  for (int i(0), iEnd(bd.size()); i < iEnd; ++i) {
    out << "  dbstack[" << i << " --> " << _rep_(bd.var(i)) << std::endl;
  };
}

CL_DEFUN List_sp core__list_from_va_list(VaList_sp valist)
{
  ql::list l;
  size_t nargs = valist->remaining_nargs();
  printf("%s:%d in %s  nargs=%zu\n", __FILE__, __LINE__, __FUNCTION__, nargs);
  for ( size_t i=0; i<nargs; ++i ) {
    T_sp one = valist->next_arg();
    l << one;
  }
  T_sp result = l.cons();
  printf("%s:%d Returning: %s\n", __FILE__, __LINE__, _rep_(result).c_str()); 
  return result;
}
  
CL_LAMBDA(&optional (out t) msg);
CL_DECLARE();
CL_DOCSTRING("ihsBacktrace");
CL_DEFUN T_sp core__ihs_backtrace(T_sp outputDesignator, T_sp msg) {
  T_sp ss;
  if (outputDesignator.nilp()) {
    ss = clasp_make_string_output_stream();
  } else {
    ss = coerce::outputStreamDesignator(outputDesignator);
  }
  if (!msg.nilp()) {
    clasp_writeln_string(((BF("\n%s") % _rep_(msg)).str()), ss);
  }
  clasp_writeln_string((BF("%s") % backtrace_as_string()).str(),ss);
  if (outputDesignator.nilp()) {
    return cl__get_output_stream_string(ss);
  }
  return _Nil<T_O>();
};
};

namespace core {


CL_LAMBDA(function);
CL_DECLARE();
CL_DOCSTRING("functionLambdaList");
CL_DEFUN T_mv core__function_lambda_list(T_sp obj) {
  if (obj.nilp()) {
    return Values(_Nil<T_O>(), _Nil<T_O>());
  } else if (Symbol_sp sym = obj.asOrNull<Symbol_O>()) {
    if (!sym->fboundp()) {
      return Values(_Nil<T_O>(), _Nil<T_O>());
    }
    Function_sp fn = sym->symbolFunction();
    return Values(fn->lambda_list(), _lisp->_true());
  } else if (NamedFunction_sp func = obj.asOrNull<NamedFunction_O>()) {
    return Values(func->lambda_list(), _lisp->_true());
  }
  return Values(_Nil<T_O>(), _Nil<T_O>());
}

CL_LAMBDA(function);
CL_DECLARE();
CL_DOCSTRING("functionSourcePosInfo");
CL_DEFUN gc::Nilable<SourcePosInfo_sp> core__function_source_pos_info(T_sp functionDesignator) {
  Closure_sp closure = coerce::closureDesignator(functionDesignator);
  gc::Nilable<SourcePosInfo_sp> sourcePosInfo = closure->sourcePosInfo();
  return sourcePosInfo;
}

CL_LAMBDA(fn kind);
CL_DECLARE();
CL_DOCSTRING("set the kind of a function object (:function|:macro)");
CL_DEFUN void core__set_kind(Function_sp fn, Symbol_sp kind) {
  if ( NamedFunction_sp func = fn.asOrNull<NamedFunction_O>() ) {
    fn->set_kind(kind);
    return;
  }
  if ( kind == kw::_sym_function ) return; // by default everything is a function
  SIMPLE_ERROR(BF("You cannot set the kind: %s of a Function_O object") % _rep_(kind));
};

CL_LISPIFY_NAME("core:functionSourcePos");
CL_DEFMETHOD T_mv Function_O::functionSourcePos() const {
  T_sp spi = this->sourcePosInfo();
  T_sp sfi = core__source_file_info(spi);
  if (sfi.nilp() || spi.nilp()) {
    return Values(sfi, make_fixnum(0), make_fixnum(0));
  }
  return Values(sfi, make_fixnum(gc::As<SourcePosInfo_sp>(spi)->filepos()), make_fixnum(gc::As<SourcePosInfo_sp>(spi)->lineno()));
}


};


  SYMBOL_SC_(CorePkg, smartPointerDetails);
  SYMBOL_EXPORT_SC_(ClPkg, null);
  SYMBOL_SC_(CorePkg, unbound);
  SYMBOL_EXPORT_SC_(ClPkg, read);
  SYMBOL_EXPORT_SC_(ClPkg, read_preserving_whitespace);
  SYMBOL_EXPORT_SC_(ClPkg, read_delimited_list);
  SYMBOL_EXPORT_SC_(ClPkg, every);
  SYMBOL_EXPORT_SC_(ClPkg, some);
  SYMBOL_EXPORT_SC_(ClPkg, notevery);
  SYMBOL_EXPORT_SC_(ClPkg, notany);
  SYMBOL_EXPORT_SC_(ClPkg, mapcar);
  SYMBOL_EXPORT_SC_(ClPkg, mapc);
  SYMBOL_EXPORT_SC_(ClPkg, maplist);
  SYMBOL_EXPORT_SC_(ClPkg, mapl);
  SYMBOL_SC_(CorePkg, mapappend);
  SYMBOL_EXPORT_SC_(ClPkg, mapcan);
  SYMBOL_EXPORT_SC_(ClPkg, mapcon);
  SYMBOL_SC_(CorePkg, macroexpand_default);
  SYMBOL_EXPORT_SC_(ClPkg, append);
  SYMBOL_EXPORT_SC_(ClPkg, classOf);
  SYMBOL_EXPORT_SC_(ClPkg, identity);
  SYMBOL_EXPORT_SC_(ClPkg, constantp);
  SYMBOL_SC_(CorePkg, sequence_start_end);
  SYMBOL_EXPORT_SC_(ClPkg, ash);
  SYMBOL_SC_(CorePkg, type_to_symbol);
  SYMBOL_SC_(CorePkg, gdb);
  SYMBOL_SC_(CorePkg, gdbInspect);
  SYMBOL_EXPORT_SC_(ClPkg, gensym);
  SYMBOL_EXPORT_SC_(ClPkg, type_of);
  SYMBOL_EXPORT_SC_(ClPkg, specialOperatorP);
  SYMBOL_EXPORT_SC_(ClPkg, macroFunction);
  SYMBOL_SC_(CorePkg, separatePairList);
  SYMBOL_EXPORT_SC_(ClPkg, set);
  SYMBOL_EXPORT_SC_(ClPkg, gensym);
  SYMBOL_EXPORT_SC_(ClPkg, type_of);
  SYMBOL_SC_(CorePkg, separatePairList);
  SYMBOL_EXPORT_SC_(ClPkg, gensym);
  SYMBOL_EXPORT_SC_(ClPkg, type_of);
  SYMBOL_SC_(CorePkg, separatePairList);
  SYMBOL_SC_(CorePkg, testMemoryError);
  SYMBOL_SC_(CorePkg, functionBlockName);
  SYMBOL_SC_(CorePkg, validFunctionNameP);
  SYMBOL_EXPORT_SC_(ClPkg, fdefinition);
  SYMBOL_EXPORT_SC_(ClPkg, fboundp);
  SYMBOL_EXPORT_SC_(ClPkg, fmakunbound);
  SYMBOL_EXPORT_SC_(ClPkg, values);
  SYMBOL_EXPORT_SC_(ClPkg, values_list);
  SYMBOL_EXPORT_SC_(CorePkg, pointer);
  SYMBOL_EXPORT_SC_(CorePkg, toTaggedFixnum);
  SYMBOL_EXPORT_SC_(CorePkg, fromTaggedFixnum);
  SYMBOL_EXPORT_SC_(CorePkg, dumpTaggedFixnum);
  SYMBOL_SC_(CorePkg, ihsBacktrace);
  SYMBOL_SC_(CorePkg, ihsTop);
  SYMBOL_SC_(CorePkg, ihsPrev);
  SYMBOL_SC_(CorePkg, ihsNext);
  SYMBOL_SC_(CorePkg, ihsFun);
  SYMBOL_SC_(CorePkg, ihsEnv);
  SYMBOL_SC_(CorePkg, bdsTop);
  SYMBOL_SC_(CorePkg, bdsVar);
  SYMBOL_SC_(CorePkg, bdsVal);

namespace core {
void initialize_primitives() {
  //
  // Define functions first because generics and methods depend on some of them
  //

}

};
