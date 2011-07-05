#ifndef __COMPILER_H__
#define __COMPILER_H__

#include <string>

#include <clang/Frontend/CompilerInstance.h>
#include <llvm/Support/raw_ostream.h>

namespace llvm
{
    class MemoryBuffer;
    class Module;
}

namespace clang
{
    class TextDiagnosticPrinter;
}

namespace Coal
{

class Compiler
{
    public:
        Compiler(const std::string &options);
        ~Compiler();

        llvm::Module *compile(llvm::MemoryBuffer *source);
        std::string log() const;

        bool valid() const;

    private:
        bool p_valid;

        clang::CompilerInstance p_compiler;

        std::string p_log;
        llvm::raw_string_ostream p_log_stream;
        clang::TextDiagnosticPrinter *p_log_printer;
};

}

#endif
