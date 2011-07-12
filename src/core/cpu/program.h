#ifndef __CPU_PROGRAM_H__
#define __CPU_PROGRAM_H__

#include "../deviceinterface.h"

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
        bool build(const llvm::Module *module);

    private:
        CPUDevice *p_device;
        Program *p_program;
};

}

#endif
