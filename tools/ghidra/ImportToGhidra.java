// Push the repo's verified knowledge (types + function signatures + global var
// types) INTO the Ghidra program — the repo->Ghidra half of the sync loop.
// (The Ghidra->repo half is ExportSymbolsTypes.java + tools/import_symbols.py.)
//
// It consumes a directory prepared by tools/sync_to_ghidra.py:
//   import.h     flat, preprocessed C: base int typedefs + game structs/enums +
//                `extern` prototypes for every function/global we want to apply.
//                CParser turns the structs/enums into data types AND each
//                prototype into a FunctionDefinitionDataType named after the fn.
//   apply.tsv    one directive per line, tab-separated. Targets are resolved by
//                ADDRESS (from config/symbols) — addresses are the invariant;
//                Ghidra's symbol names drift, so we never match on name:
//                  FUNC   <name> <hexaddr>                apply signature @ addr
//                  GLOBAL <name> <baseType> <ptrs> <addr> type the global @ addr
//
// Nothing is renamed and no C bodies are touched — only types, signatures, and
// global var types (the "knowledge layer"). Run read-only for a dry run:
//   analyzeHeadless <proj_dir> <proj> -process MAIN.EXE -readOnly \
//     -scriptPath tools/ghidra -postScript ImportToGhidra.java <prepDir>
// Drop -readOnly to actually commit. tools/sync_to_ghidra.py wraps both.
//
//@author tenchu-decomp
//@category Data Types

import ghidra.app.cmd.function.ApplyFunctionSignatureCmd;
import ghidra.app.cmd.function.FunctionRenameOption;
import ghidra.app.script.GhidraScript;
import ghidra.app.util.cparser.C.CParser;
import ghidra.app.util.parser.FunctionSignatureParser;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.CategoryPath;
import ghidra.program.model.data.DataType;
import ghidra.program.model.data.DataTypeManager;
import ghidra.program.model.data.FunctionDefinitionDataType;
import ghidra.program.model.data.PointerDataType;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.symbol.SourceType;
import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.List;

public class ImportToGhidra extends GhidraScript {

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        File prep = (args.length > 0) ? new File(args[0])
                                      : askDirectory("Sync prep directory", "Select");
        boolean readOnly = !currentProgram.canSave();
        println("ImportToGhidra: prep=" + prep + (readOnly ? "  (DRY RUN, read-only)" : "  (COMMIT)"));

        DataTypeManager dtm = currentProgram.getDataTypeManager();

        // --- 1. types + function-definition data types ----------------------
        String flat = new String(Files.readAllBytes(new File(prep, "import.h").toPath()),
                                 StandardCharsets.UTF_8);
        int tx = currentProgram.startTransaction("sync: import types");
        int before = dtm.getDataTypeCount(true);
        try {
            CParser parser = new CParser(dtm, true, new DataTypeManager[] {});
            parser.parse(flat);
            if (!parser.didParseSucceed()) {
                println("  C parse messages:\n" + parser.getParseMessages());
            }
        } catch (Exception e) {
            println("  TYPE PARSE FAILED: " + e.getMessage());
        } finally {
            currentProgram.endTransaction(tx, true);
        }
        println(String.format("  types: %d -> %d in the program's data type manager",
                before, dtm.getDataTypeCount(true)));

        // --- 2 & 3. apply signatures and global var types (by address) ------
        List<String> directives = Files.readAllLines(new File(prep, "apply.tsv").toPath());
        Listing listing = currentProgram.getListing();
        int nfunc = 0, nglob = 0, nskip = 0;
        FunctionSignatureParser sigParser = new FunctionSignatureParser(dtm, null);
        StringBuilder drift = new StringBuilder();

        int tx2 = currentProgram.startTransaction("sync: apply signatures + globals");
        try {
            for (String line : directives) {
                line = line.trim();
                if (line.isEmpty() || line.startsWith("#")) continue;
                String[] f = line.split("\t");
                String kind = f[0];

                if (kind.equals("FUNC") && f.length >= 4) {
                    String name = f[1];
                    Address addr = currentProgram.getAddressFactory().getAddress(f[2]);
                    String proto = f[3];
                    Function fn = getFunctionAt(addr);
                    if (fn == null) { println("  [skip func] no function at " + f[2] + " (" + name + ")"); nskip++; continue; }
                    FunctionDefinitionDataType def;
                    try {
                        def = sigParser.parse(fn.getSignature(), proto);
                    } catch (Exception e) {
                        println("  [skip func] can't parse '" + proto + "': " + e.getMessage()); nskip++; continue;
                    }
                    if (def == null) { println("  [skip func] parse returned null for " + proto); nskip++; continue; }
                    // NO_CHANGE: apply the repo's return/param TYPES, but never
                    // rename the function or touch its calling convention.
                    ApplyFunctionSignatureCmd cmd = new ApplyFunctionSignatureCmd(
                            addr, def, SourceType.IMPORTED, true, FunctionRenameOption.NO_CHANGE);
                    if (cmd.applyTo(currentProgram, monitor)) {
                        println("  [func] " + fn.getName() + " @ " + addr + "  ->  " + fn.getSignature().getPrototypeString());
                        noteDrift(drift, addr, name, fn.getName());
                        nfunc++;
                    } else {
                        println("  [skip func] apply failed for " + name + ": " + cmd.getStatusMsg()); nskip++;
                    }

                } else if (kind.equals("GLOBAL") && f.length >= 5) {
                    String name = f[1], baseName = f[2];
                    int ptrs = Integer.parseInt(f[3]);
                    Address addr = currentProgram.getAddressFactory().getAddress(f[4]);
                    DataType base = findType(dtm, baseName);
                    if (base == null) { println("  [skip glob] unknown type '" + baseName + "' for " + name); nskip++; continue; }
                    DataType dt = base;
                    for (int i = 0; i < ptrs; i++) dt = new PointerDataType(dt);
                    try {
                        listing.clearCodeUnits(addr, addr.add(Math.max(0, dt.getLength() - 1)), false);
                        listing.createData(addr, dt);
                        println("  [glob] " + name + " : " + dt.getName() + " @ " + addr);
                        ghidra.program.model.symbol.Symbol s = getSymbolAt(addr);
                        noteDrift(drift, addr, name, s == null ? null : s.getName());
                        nglob++;
                    } catch (Exception e) {
                        println("  [skip glob] " + name + " @ " + addr + ": " + e.getMessage()); nskip++;
                    }
                }
            }
        } finally {
            currentProgram.endTransaction(tx2, true);
        }

        println(String.format("ImportToGhidra: %d signatures, %d globals applied, %d skipped%s",
                nfunc, nglob, nskip, readOnly ? "  (DRY RUN — nothing saved)" : ""));
        if (drift.length() > 0) {
            println("Name drift (same address, different name) — reconcile which side is better:\n"
                    + drift.toString().trim());
        }
    }

    // Log addresses where the repo's name and Ghidra's current name disagree, so
    // the operator can pull Ghidra's (often better) name back with import_symbols.py,
    // or rename in Ghidra to the repo's — this is the loop's manual reconcile step.
    private void noteDrift(StringBuilder drift, Address addr, String repoName, String ghidraName) {
        if (ghidraName != null && !repoName.equals(ghidraName)) {
            drift.append(String.format("  %s  repo=%-28s ghidra=%s%n", addr, repoName, ghidraName));
        }
    }

    // Resolve a (non-function) type by simple name across all category paths.
    private DataType findType(DataTypeManager dtm, String name) {
        DataType dt = dtm.getDataType(new CategoryPath("/"), name);
        if (dt != null) return dt;
        java.util.List<DataType> all = new java.util.ArrayList<>();
        dtm.findDataTypes(name, all);
        return all.isEmpty() ? null : all.get(0);
    }
}
