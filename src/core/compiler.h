#ifndef __COMPILER_H__
#define __COMPILER_H__

#include <clang/Frontend/CompilerInstance.h>

namespace llvm
{
    class MemoryBuffer;
    class Module;
}

namespace Coal
{

class Compiler
{
    public:
        Compiler(const char *options);
        ~Compiler();
        
        llvm::Module *compile(llvm::MemoryBuffer *source);
        
        bool valid() const;
        
    private:
        bool p_valid;
        clang::CompilerInstance p_compiler;
};

}

#endif