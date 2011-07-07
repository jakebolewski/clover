//===-- llvm/Support/StandardPasses.h - Standard pass lists -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines utility functions for creating a "standard" set of
// optimization passes, so that compilers and tools which use optimization
// passes use the same set of standard passes.
//
// These are implemented as inline functions so that we do not have to worry
// about link issues.
//
//===----------------------------------------------------------------------===//

#ifndef STANDARDPASSES_H
#define STANDARDPASSES_H

#include <llvm/PassManager.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/IPO.h>

static inline void createStandardAliasAnalysisPasses(llvm::PassManagerBase *PM)
{
    // Add TypeBasedAliasAnalysis before BasicAliasAnalysis so that
    // BasicAliasAnalysis wins if they disagree. This is intended to help
    // support "obvious" type-punning idioms.
    PM->add(llvm::createTypeBasedAliasAnalysisPass());
    PM->add(llvm::createBasicAliasAnalysisPass());
}

static inline void addOnePass(llvm::PassManagerBase *PM, llvm::Pass *P)
{
    PM->add(P);
}

static inline void createStandardLTOPasses(llvm::PassManagerBase *PM,
                                           bool Internalize,
                                           bool RunInliner,
                                           const std::vector<const char *> &api)
{
    // Provide AliasAnalysis services for optimizations.
    createStandardAliasAnalysisPasses(PM);

    // Now that composite has been compiled, scan through the module, looking
    // for a main function.  If main is defined, mark all other functions
    // internal.
    if (Internalize)
      addOnePass(PM, llvm::createInternalizePass(api));

    // Propagate constants at call sites into the functions they call.  This
    // opens opportunities for globalopt (and inlining) by substituting function
    // pointers passed as arguments to direct uses of functions.
    addOnePass(PM, llvm::createIPSCCPPass());

    // Now that we internalized some globals, see if we can hack on them!
    addOnePass(PM, llvm::createGlobalOptimizerPass());

    // Linking modules together can lead to duplicated global constants, only
    // keep one copy of each constant...
    addOnePass(PM, llvm::createConstantMergePass());

    // Remove unused arguments from functions...
    addOnePass(PM, llvm::createDeadArgEliminationPass());

    // Reduce the code after globalopt and ipsccp.  Both can open up significant
    // simplification opportunities, and both can propagate functions through
    // function pointers.  When this happens, we often have to resolve varargs
    // calls, etc, so let instcombine do this.
    addOnePass(PM, llvm::createInstructionCombiningPass());

    // Inline small functions
    if (RunInliner)
      addOnePass(PM, llvm::createFunctionInliningPass());

    addOnePass(PM, llvm::createPruneEHPass());   // Remove dead EH info.
    // Optimize globals again if we ran the inliner.
    if (RunInliner)
      addOnePass(PM, llvm::createGlobalOptimizerPass());
    addOnePass(PM, llvm::createGlobalDCEPass()); // Remove dead functions.

    // If we didn't decide to inline a function, check to see if we can
    // transform it to pass arguments by value instead of by reference.
    addOnePass(PM, llvm::createArgumentPromotionPass());

    // The IPO passes may leave cruft around.  Clean up after them.
    addOnePass(PM, llvm::createInstructionCombiningPass());
    addOnePass(PM, llvm::createJumpThreadingPass());
    // Break up allocas
    addOnePass(PM, llvm::createScalarReplAggregatesPass());

    // Run a few AA driven optimizations here and now, to cleanup the code.
    addOnePass(PM, llvm::createFunctionAttrsPass()); // Add nocapture.
    addOnePass(PM, llvm::createGlobalsModRefPass()); // IP alias analysis.

    addOnePass(PM, llvm::createLICMPass());      // Hoist loop invariants.
    addOnePass(PM, llvm::createGVNPass());       // Remove redundancies.
    addOnePass(PM, llvm::createMemCpyOptPass()); // Remove dead memcpys.
    // Nuke dead stores.
    addOnePass(PM, llvm::createDeadStoreEliminationPass());

    // Cleanup and simplify the code after the scalar optimizations.
    addOnePass(PM, llvm::createInstructionCombiningPass());

    addOnePass(PM, llvm::createJumpThreadingPass());

    // Delete basic blocks, which optimization passes may have killed.
    addOnePass(PM, llvm::createCFGSimplificationPass());

    // Now that we have optimized the program, discard unreachable functions.
    addOnePass(PM, llvm::createGlobalDCEPass());
}

#endif
