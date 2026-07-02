// Export every function's decompiled C + metadata, for import into the decomp
// repo (see tools/ghidra_import.py). Produces, in the chosen output directory:
//   functions.tsv   one line per function:  <hexaddr>\t<sizeBytes>\t<name>
//   c/<hexaddr>.c   that function's decompiled C (for reference / commenting in)
//
// Run it on the program that holds Tenchu's main.exe (loaded at 0x80011000):
//   * GUI: Script Manager -> this script -> pick an output dir; or
//   * headless: analyzeHeadless <proj_dir> <proj> -process <prog> \
//               -scriptPath tools/ghidra -postScript ExportDecomp.java <outDir>
//
//@author tenchu-decomp
//@category Decompiler

import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;

public class ExportDecomp extends GhidraScript {

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        File outDir = (args.length > 0) ? new File(args[0])
                                        : askDirectory("Export output directory", "Select");
        File cDir = new File(outDir, "c");
        cDir.mkdirs();

        DecompInterface ifc = new DecompInterface();
        ifc.toggleCCode(true);
        if (!ifc.openProgram(currentProgram)) {
            printerr("Could not initialise the decompiler: " + ifc.getLastMessage());
            return;
        }

        StringBuilder manifest = new StringBuilder();
        FunctionIterator funcs = currentProgram.getFunctionManager().getFunctions(true);
        int ok = 0, fail = 0;
        while (funcs.hasNext() && !monitor.isCancelled()) {
            Function f = funcs.next();
            if (f.isThunk() || f.isExternal()) {
                continue;
            }
            long addr = f.getEntryPoint().getOffset();
            long size = f.getBody().getNumAddresses();
            String name = f.getName();

            String c;
            DecompileResults r = ifc.decompileFunction(f, 60, monitor);
            if (r != null && r.decompileCompleted() && r.getDecompiledFunction() != null) {
                c = r.getDecompiledFunction().getC();
                ok++;
            } else {
                c = "// ghidra: decompile failed"
                    + (r != null ? " (" + r.getErrorMessage() + ")" : "") + "\n";
                fail++;
            }
            Files.write(new File(cDir, String.format("%08x.c", addr)).toPath(),
                        c.getBytes(StandardCharsets.UTF_8));
            manifest.append(String.format("%08x\t%d\t%s%n", addr, size, name));
        }
        Files.write(new File(outDir, "functions.tsv").toPath(),
                    manifest.toString().getBytes(StandardCharsets.UTF_8));
        println(String.format("ExportDecomp: %d functions (%d decompiled, %d failed) -> %s",
                              ok + fail, ok, fail, outDir));
    }
}
