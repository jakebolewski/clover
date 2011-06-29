#include "compiler.h"

#include <cstring>
#include <string>
#include <sstream>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Frontend/LangStandard.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Module.h>
#include <llvm/LLVMContext.h>

using namespace Coal;

Compiler::Compiler(const char *options)
: p_valid(false)
{
    size_t len = std::strlen(options);
    
    // Parse the user options
    std::istringstream options_stream(options);
    std::string token;
    llvm::SmallVector<const char *, 16> argv;
    
    while (options_stream >> token)
    {
        char *arg = new char[token.size() + 1];
        
        std::strcpy(arg, token.c_str());
        argv.push_back(arg);
    }
    
    argv.push_back("program.cl");
    
    // Create the diagnostics engine
    p_compiler.createDiagnostics(0 ,NULL);
    
    if (!p_compiler.hasDiagnostics())
        return;
    
    // Create a compiler invocation
    clang::CompilerInvocation &invocation = p_compiler.getInvocation();
    invocation.setLangDefaults(clang::IK_OpenCL);
    
    clang::CompilerInvocation::CreateFromArgs(invocation, 
                                              argv.data(), 
                                              argv.data() + argv.size(), 
                                              p_compiler.getDiagnostics());

    // Set diagnostic options
    clang::DiagnosticOptions &diag_opts = invocation.getDiagnosticOpts();
    diag_opts.Pedantic = true;
    diag_opts.ShowColumn = true;
    diag_opts.ShowLocation = true;
    diag_opts.ShowCarets = true;
    diag_opts.ShowFixits = true;
    diag_opts.ShowColors = false;
    diag_opts.ErrorLimit = 19;
    diag_opts.MessageLength = 80;
    
    // Set frontend options
    clang::FrontendOptions &frontend_opts = invocation.getFrontendOpts();
    frontend_opts.ProgramAction = clang::frontend::EmitLLVMOnly;
    frontend_opts.DisableFree = true;
    
    // Set header search options
    clang::HeaderSearchOptions &header_opts = invocation.getHeaderSearchOpts();
    header_opts.Verbose = true;
    header_opts.UseBuiltinIncludes = false;
    header_opts.UseStandardIncludes = false;
    header_opts.UseStandardCXXIncludes = false;
    
    // Set lang options
    clang::LangOptions &lang_opts = invocation.getLangOpts();
    lang_opts.NoBuiltin = true;
    lang_opts.OpenCL = true;
    
    // Set preprocessor options
    clang::PreprocessorOptions &prep_opts = invocation.getPreprocessorOpts();
    //prep_opts.Includes.push_back("stdlib.h");
    //prep_opts.addRemappedFile("stdlib.h", ...);
    
    p_valid = true;
}

Compiler::~Compiler()
{
    
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
      return 0;

    module = act->takeModule();
    
    // Cleanup
    prep_opts.eraseRemappedFile(prep_opts.remapped_file_buffer_end());
   
   return module;
}