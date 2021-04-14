#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>

#include "parser.hpp"
#include "codegen.hpp"

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Usage:\n");
		printf("    diamond source_file_path\n");
		exit(EXIT_FAILURE);
	}

	// Read file
	std::ifstream in;
	in.open(argv[1]);
	std::stringstream stream;
	stream << in.rdbuf();
	std::string source_file = stream.str();

	// Parse
	auto ast = parse::program(Source(argv[1], source_file.begin(), source_file.end()));
	ast.print();

	// Codegen
	Codegen ir = codegen(ast);

	// Generate object file
	auto TargetTriple = llvm::sys::getDefaultTargetTriple();
	llvm::InitializeAllTargetInfos();
	llvm::InitializeAllTargets();
	llvm::InitializeAllTargetMCs();
	llvm::InitializeAllAsmParsers();
	llvm::InitializeAllAsmPrinters();

	std::string Error;
	auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);

	// Print an error and exit if we couldn't find the requested target.
	// This generally occurs if we've forgotten to initialise the
	// TargetRegistry or we have a bogus target triple.
	if (!Target) {
		llvm::errs() << Error;
		return 1;
	}

	auto CPU = "generic";
	auto Features = "";

	llvm::TargetOptions opt;
	auto RM = llvm::Optional<llvm::Reloc::Model>();
	auto TargetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

	ir.module->setDataLayout(TargetMachine->createDataLayout());
	ir.module->setTargetTriple(TargetTriple);

	std::string Filename = "";
	size_t i = strlen(argv[1]) - 1;
	while (argv[1][i] != '/' && i > 0) {
		i--;
	}
	if (i > 0) i++;
	while (i < strlen(argv[1]) && argv[1][i] != '.') {
		Filename += argv[1][i];
		i++;
	}
	std::string filename = Filename;
	Filename += ".o";
	std::cout << Filename << '\n';

	std::error_code EC;
	llvm::raw_fd_ostream dest(Filename, EC, llvm::sys::fs::OF_None);

	if (EC) {
		llvm::errs() << "Could not open file: " << EC.message();
		return 1;
	}

	llvm::legacy::PassManager pass;
	auto FileType = llvm::CGFT_ObjectFile;

	if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
		llvm::errs() << "TargetMachine can't emit a file of this type";
		return 1;
	}

	pass.run(*(ir.module));
	dest.flush();

	std::string command = "clang-12 -o ";
	command += filename;
	command += " ";
	command += Filename;
	system(command.c_str());

	return 0;
}
