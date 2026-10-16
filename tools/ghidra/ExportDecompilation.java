// Headless Ghidra post-script: decompile every function in the program and write
//   <outdir>/<program>.c    pseudocode for each function, separated by "// ==== <addr>" headers
//   <outdir>/<program>.tsv  index: address, size, namespaced name, signature
// Usage (via tools/ghidra_export.py): -postScript ExportDecompilation.java <outdir>
// @category JPOG

import java.io.File;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;

public class ExportDecompilation extends GhidraScript {

	private static final int TIMEOUT_SECONDS = 60;

	@Override
	protected void run() throws Exception {
		String[] args = getScriptArgs();
		if (args.length < 1) {
			throw new IllegalArgumentException("usage: ExportDecompilation.java <outdir>");
		}
		File outDir = new File(args[0]);
		outDir.mkdirs();
		String base = currentProgram.getName().replaceAll("\\.[^.]+$", "");

		DecompInterface decomp = new DecompInterface();
		decomp.setOptions(new DecompileOptions());
		decomp.toggleCCode(true);
		decomp.toggleSyntaxTree(false);
		decomp.setSimplificationStyle("decompile");
		if (!decomp.openProgram(currentProgram)) {
			throw new IllegalStateException("decompiler failed to open program: " + decomp.getLastMessage());
		}

		int total = currentProgram.getFunctionManager().getFunctionCount();
		int done = 0;
		int failed = 0;
		monitor.initialize(total);
		try (PrintWriter code = new PrintWriter(new File(outDir, base + ".c"), StandardCharsets.UTF_8);
				PrintWriter index = new PrintWriter(new File(outDir, base + ".tsv"), StandardCharsets.UTF_8)) {
			index.println("address\tsize\tname\tsignature");
			FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
			while (it.hasNext() && !monitor.isCancelled()) {
				Function f = it.next();
				String addr = "0x" + f.getEntryPoint().toString();
				long size = f.getBody().getNumAddresses();
				String name = f.getName(true);
				String sig = f.getPrototypeString(false, false).replace('\t', ' ').replace('\n', ' ');
				index.println(addr + "\t" + size + "\t" + name + "\t" + sig);

				code.println("// ==== " + addr + " " + name + " (" + size + " bytes)");
				if (f.isThunk() || f.isExternal()) {
					code.println("// thunk/external: " + sig);
				} else {
					DecompileResults res = decomp.decompileFunction(f, TIMEOUT_SECONDS, monitor);
					if (res != null && res.decompileCompleted()) {
						code.println(res.getDecompiledFunction().getC().strip());
					} else {
						failed++;
						code.println("// decompilation failed: " + (res == null ? "null" : res.getErrorMessage()));
					}
				}
				code.println();
				done++;
				monitor.incrementProgress(1);
			}
		} finally {
			decomp.dispose();
		}
		println(String.format("ExportDecompilation: %s: %d functions, %d failed", base, done, failed));
	}
}
