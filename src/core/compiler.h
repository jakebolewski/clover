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

class DeviceInterface;

class Compiler
{
    public:
        Compiler(DeviceInterface *device);
        ~Compiler();

        bool compile(const std::string &options, llvm::MemoryBuffer *source);

        const std::string &log() const;
        const std::string &options() const;
        bool optimize() const;
        llvm::Module *module() const;

        void appendLog(const std::string &log);

    private:
        DeviceInterface *p_device;
        clang::CompilerInstance p_compiler;
        llvm::Module *p_module;
        bool p_optimize;

        std::string p_log, p_options;
        llvm::raw_string_ostream p_log_stream;
        clang::TextDiagnosticPrinter *p_log_printer;
};

}

#endif
