#ifndef __CPU_PROGRAM_H__
#define __CPU_PROGRAM_H__

#include "../deviceinterface.h"

namespace llvm
{
    class ExecutionEngine;
    class Module;
}

namespace Coal
{

class CPUDevice;
class Program;

class CPUProgram : public DeviceProgram
{
    public:
        CPUProgram(CPUDevice *device, Program *program);
        ~CPUProgram();

        bool linkStdLib() const;
        void createOptimizationPasses(llvm::PassManager *manager, bool optimize);
        bool build(llvm::Module *module);

        bool initJIT();
        llvm::ExecutionEngine *jit() const;

    private:
        CPUDevice *p_device;
        Program *p_program;

        llvm::ExecutionEngine *p_jit;
        llvm::Module *p_module;
};

}

#endif
