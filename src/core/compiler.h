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
        Compiler();
        ~Compiler();

        bool setOptions(const std::string &options);
        llvm::Module *compile(llvm::MemoryBuffer *source);

        const std::string &log() const;
        const std::string &options() const;
        bool valid() const;

    private:
        clang::CompilerInstance p_compiler;

        std::string p_log, p_options;
        llvm::raw_string_ostream p_log_stream;
        clang::TextDiagnosticPrinter *p_log_printer;
};

}

#endif
