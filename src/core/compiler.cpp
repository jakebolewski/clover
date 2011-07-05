#include "compiler.h"

#include <cstring>
#include <string>
#include <sstream>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Frontend/LangStandard.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/Host.h>
#include <llvm/Module.h>
#include <llvm/LLVMContext.h>

using namespace Coal;

Compiler::Compiler(const std::string &options)
: p_valid(false), p_log_stream(p_log), p_log_printer(0)
{
    // Set codegen options
    clang::CodeGenOptions &codegen_opts = p_compiler.getCodeGenOpts();
    codegen_opts.DebugInfo = false;
    codegen_opts.AsmVerbose = true;
    codegen_opts.OptimizationLevel = 2;

    // Set diagnostic options
    clang::DiagnosticOptions &diag_opts = p_compiler.getDiagnosticOpts();
    diag_opts.Pedantic = true;
    diag_opts.ShowColumn = true;
    diag_opts.ShowLocation = true;
    diag_opts.ShowCarets = false;
    diag_opts.ShowFixits = true;
    diag_opts.ShowColors = false;
    diag_opts.ErrorLimit = 19;
    diag_opts.MessageLength = 0;
    diag_opts.DumpBuildInformation = std::string();
    diag_opts.DiagnosticLogFile = std::string();

    // Set frontend options
    clang::FrontendOptions &frontend_opts = p_compiler.getFrontendOpts();
    frontend_opts.ProgramAction = clang::frontend::EmitLLVMOnly;
    frontend_opts.DisableFree = true;
    frontend_opts.Inputs.push_back(std::make_pair(clang::IK_OpenCL, "program.cl"));

    // Set header search options
    clang::HeaderSearchOptions &header_opts = p_compiler.getHeaderSearchOpts();
    header_opts.Verbose = false;
    header_opts.UseBuiltinIncludes = false;
    header_opts.UseStandardIncludes = false;
    header_opts.UseStandardCXXIncludes = false;

    // Set lang options
    clang::LangOptions &lang_opts = p_compiler.getLangOpts();
    lang_opts.NoBuiltin = true;
    lang_opts.OpenCL = true;

    // Set target options
    clang::TargetOptions &target_opts = p_compiler.getTargetOpts();
    target_opts.Triple = llvm::sys::getHostTriple();

    // Set preprocessor options
    clang::PreprocessorOptions &prep_opts = p_compiler.getPreprocessorOpts();
    //prep_opts.Includes.push_back("stdlib.h");
    //prep_opts.addRemappedFile("stdlib.h", ...);

    clang::CompilerInvocation &invocation = p_compiler.getInvocation();
    invocation.setLangDefaults(clang::IK_OpenCL);

    // Parse the user options
    std::istringstream options_stream(options);
    std::string token;
    bool Werror = false, inI = false, inD = false;

    while (options_stream >> token)
    {
        if (inI)
        {
            // token is an include path
            header_opts.AddPath(token, clang::frontend::Angled, true, false, false);
            inI = false;
            continue;
        }
        else if (inD)
        {
            // token is name or name=value
            prep_opts.addMacroDef(token);
        }

        if (token == "-I")
        {
            inI = true;
        }
        else if (token == "-D")
        {
            inD = true;
        }
        else if (token == "-cl-single-precision-constant")
        {
            lang_opts.SinglePrecisionConstants = true;
        }
        else if (token == "-cl-opt-disable")
        {
            codegen_opts.OptimizationLevel = 0;
        }
        else if (token == "-cl-mad-enable")
        {
            codegen_opts.LessPreciseFPMAD = true;
        }
        else if (token == "-cl-unsafe-math-optimizations")
        {
            codegen_opts.UnsafeFPMath = true;
        }
        else if (token == "-cl-finite-math-only")
        {
            codegen_opts.NoInfsFPMath = true;
            codegen_opts.NoNaNsFPMath = true;
        }
        else if (token == "-cl-fast-relaxed-math")
        {
            codegen_opts.UnsafeFPMath = true;
            codegen_opts.NoInfsFPMath = true;
            codegen_opts.NoNaNsFPMath = true;
            lang_opts.FastRelaxedMath = true;
        }
        else if (token == "-w")
        {
            diag_opts.IgnoreWarnings = true;
        }
        else if (token == "-Werror")
        {
            Werror = true;
        }
    }

    // Create the diagnostics engine
    p_log_printer = new clang::TextDiagnosticPrinter(p_log_stream, diag_opts);
    p_compiler.createDiagnostics(0, NULL, p_log_printer);

    if (!p_compiler.hasDiagnostics())
        return;

    p_compiler.getDiagnostics().setWarningsAsErrors(Werror);

    p_valid = true;
}

Compiler::~Compiler()
{

}

bool Compiler::valid() const
{
    return p_valid;
}

llvm::Module *Compiler::compile(llvm::MemoryBuffer *source)
{
    // Feed the compiler with source
    clang::PreprocessorOptions &prep_opts = p_compiler.getPreprocessorOpts();
    prep_opts.addRemappedFile("program.cl", source);
    prep_opts.RetainRemappedFileBuffers = true;

    // Compile
    llvm::Module *module = 0;

    llvm::OwningPtr<clang::CodeGenAction> act(
        new clang::EmitLLVMOnlyAction(new llvm::LLVMContext)
    );

    if (!p_compiler.ExecuteAction(*act))
    {
        return 0;
    }

    p_log_stream.flush();
    module = act->takeModule();

    // Cleanup
    prep_opts.eraseRemappedFile(prep_opts.remapped_file_buffer_end());

    return module;
}

std::string Compiler::log() const
{
    return p_log;
}
