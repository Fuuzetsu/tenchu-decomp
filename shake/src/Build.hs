{-# LANGUAGE DeriveGeneric #-}
{-# LANGUAGE LambdaCase #-}
{-# LANGUAGE NamedFieldPuns #-}
{-# LANGUAGE TypeFamilies #-}

import Control.Monad (when)
import qualified Data.Aeson as A
import Data.Binary (Binary)
import Data.Functor ((<&>))
import Data.Hashable (Hashable)
import qualified Data.Set as Set
import qualified Data.UUID as UUID
import Data.UUID.V4 (nextRandom)
import Development.Shake
import Development.Shake.Classes (NFData)
import Development.Shake.FilePath
import Development.Shake.Util (neededMakefileDependencies, parseMakefile)
import GHC.Generics (Generic)
import qualified System.Directory as IO

shakeDir :: FilePath
shakeDir = ".shake"

genDir :: FilePath
genDir = shakeDir </> "gen"

mainGenDir :: FilePath
mainGenDir = genDir </> "main.exe"

mainGen :: FilePath
mainGen = genDir </> "main.exe.done"

configDir :: FilePath
configDir = "config"

asmDir :: FilePath
asmDir = "asm"

srcDir :: FilePath
srcDir = "src"

buildDir :: FilePath
buildDir = shakeDir </> "build"

-- C files ran through CPP
processedDir :: FilePath
processedDir = shakeDir </> "processed"

mainExe :: FilePath
mainExe = buildDir </> "tenchu" </> "main.exe"

cross :: String -> FilePath
cross t = "mipsel-unknown-linux-gnu-" <> t

objcopy :: FilePath
objcopy = cross "objcopy"

objcopyFlags :: [String]
objcopyFlags = ["-O", "binary"]

asFlags :: [String]
asFlags = ["-EL", "-Iinclude", "-march=r3000", "-mtune=r3000", "-no-pad-sections", "-O1", "-G0"]

cpp :: FilePath
cpp = cross "cpp"

cc :: FilePath
cc = "tools/cc1-281"

-- | maspsx post-processes cc1's asm so GNU @as@ reproduces PSY-Q ASPSX's bytes
-- (div traps, load-delay nops, $at/li/move expansion, $gp small-data layout).
-- Runs as a filter between cc1 and @as@; provided by the nix devShell.
maspsx :: FilePath
maspsx = "maspsx"

-- | @--aspsx-version=2.77@ is the evidenced version for Tenchu (gp symbol+offset
-- addressing present, 1993-1997 SDK). @-G8@ is the small-data threshold maspsx
-- emulates for $gp layout (it matches cc1's @-G8@; @as@ still gets @-G0@).
maspsxFlags :: [String]
maspsxFlags = ["--aspsx-version=2.77", "-G8"]

as :: FilePath
as = cross "as"

linkerDir :: FilePath
linkerDir = "linker"

metaDir :: FilePath
metaDir = "meta"

assetsDir :: FilePath
assetsDir = "assets"

splatDirs :: [FilePath]
splatDirs =
  [ metaDir,
    linkerDir,
    asmDir,
    srcDir,
    assetsDir
  ]

ld :: FilePath
ld = cross "ld"

ldFlags :: [String]
ldFlags =
  [ "-EL"
  ]

ccFlags :: [String]
ccFlags =
  [ "-mcpu=3000",
    "-quiet",
    -- -G8: small globals go in .sdata/.sbss and are addressed via $gp, as the
    -- original ASPSX build did. maspsx reproduces the resulting layout; `as`
    -- itself still runs with -G0 (see asFlags).
    "-G8",
    "-w",
    "-O2",
    "-funsigned-char",
    "-fpeephole",
    "-ffunction-cse",
    "-fpcc-struct-return",
    "-fcommon",
    "-fverbose-asm",
    "-fgnu-linker",
    "-mgas",
    "-msoft-float"
  ]

cppFlags :: [String]
cppFlags =
  [ "-Iinclude",
    "-undef",
    "-Wall",
    "-lang-c",
    "-fno-builtin",
    "-gstabs",
    "-Dmips",
    "-D__GNUC__=2",
    "-D__OPTIMIZE__",
    "-D__mips__",
    "-D__mips",
    "-Dpsx",
    "-D__psx__",
    "-D__psx",
    "-D_PSYQ",
    "-D__EXTENSIONS__",
    "-D_MIPSEL",
    "-D_LANGUAGE_C",
    "-DLANGUAGE_C",
    "-DHACKS"
  ]

-- | The sha256 of the known-good target @disks/tenchu/main.exe@. Pinned so that
-- @check@ can detect a swapped/corrupt base image rather than merely proving the
-- build is self-consistent with whatever splat happened to extract.
expectedSha256 :: String
expectedSha256 = "0690a5c14ff3e975ebcb3de26e196a4dbb6afc992677d0de2cbcf86af9993558"

-- | Register the @.include@'d files recorded by @as --MD@ as dependencies.
--
-- @as@ resolves @.include@ directives (the INCLUDE_ASM'd nonmatching .s and
-- @include/macro.inc@) at assembly time; cpp's @-MMD@ never sees them, so without
-- this the object would go stale when that asm changes. splat bakes a @config/..@
-- prefix into the INCLUDE_ASM paths, so normalise them to match the generated-file
-- rules. Mirrors 'neededMakefileDependencies' but with the normalisation.
neededAsmDeps :: FilePath -> Action ()
neededAsmDeps depFile =
  needed . map normaliseEx . concatMap snd . parseMakefile =<< liftIO (readFile depFile)

main :: IO ()
main = do
  topD <- getProjectRoot
  IO.setCurrentDirectory topD
  let opts =
        shakeOptions
          { shakeColor = True,
            shakeVerbosity = Verbose,
            shakeTimings = True,
            shakeChange = ChangeDigest,
            shakeFiles = shakeDir
          }
  shakeArgs opts rules

rules :: Rules ()
rules = do
  _ <- addOracle (liftIO . runIdOracle)
  mainRules
  phonyRules
  objRules

objRules :: Rules ()
objRules = do
  [buildDir </> "*" </> "data" <//> "*.s.o", buildDir </> "*" </> "*.s.o"] |%> \out -> do
    let fileComponent = makeRelative buildDir out
        target = takeDirectory1 fileComponent
        src = genDir </> target </> asmDir </> dropDirectory1 (dropExtension fileComponent)
    need [src]
    liftIO $ IO.createDirectoryIfMissing True (takeDirectory out)
    withTempFile $ \depFile -> do
      cmd_ as asFlags ["--MD", depFile, "-o", out, src]
      neededAsmDeps depFile

  [processedDir <//> "*.c", processedDir <//> "*.h"] |%> \out -> do
    _generatedFiles <- getGeneratedFiles mainGen
    let fileComponent = makeRelative processedDir out
        target = takeDirectory1 fileComponent
        file = dropDirectory1 fileComponent
        header = srcDir </> target </> target <.> "h"
        genPath = genDir </> target </> "src" </> file
        srcPath = srcDir </> target </> file

    -- If corresponding file is in `src` then use it, otherwise use it from gen
    -- dir.
    src <-
      doesFileExist srcPath <&> \case
        True -> srcPath
        False -> genPath

    orderOnly [header]
    -- Make sure we have generated sources. Whether and what we need from them
    -- exactly, we'll find out from the compiler soon enough and we'll `needed`
    -- it.
    --
    -- TODO: Note that it doesn't seem to include the files inside INCLUDE_ASM,
    -- only headers :(. Very sad. Not sure what to do about it. Parse the file
    -- and find INCLUDE_ASM by hand? But maybe it'll already recompile if
    -- anything in mainGen changes.
    need [mainGen, src]
    liftIO $ IO.createDirectoryIfMissing True (takeDirectory out)
    withTempFile $ \makeOut -> do
      cmd_ cpp (cppFlags <> ["-MMD", "-MF", makeOut, "-I", takeDirectory header]) src out
      neededMakefileDependencies makeOut

  processedDir <//> "*.s" %> \out -> do
    let processed = replaceExtension out "c"
    need [processed]
    liftIO $ IO.createDirectoryIfMissing True (takeDirectory out)
    putInfo $ "Feeding " <> processed <> " to cc | maspsx"
    -- cc1 -> maspsx (filter) -> .s. maspsx massages cc1's asm so that `as`
    -- reproduces ASPSX's bytes; it leaves INCLUDE_ASM's .include/.section/.set
    -- directives untouched, so stubs pass through unchanged.
    withTempFile $ \ccOut -> do
      cmd_ (FileStdin processed) (FileStdout ccOut) cc ccFlags
      cmd_ (FileStdin ccOut) (FileStdout out) maspsx maspsxFlags

  buildDir <//> "*.c.o" %> \out -> do
    let fileComponent = makeRelative buildDir out
        target = takeDirectory1 fileComponent
        processed = processedDir </> target </> replaceExtension (dropExtension (dropDirectory1 fileComponent)) "s"

    need [processed]
    liftIO $ IO.createDirectoryIfMissing True (takeDirectory out)
    putInfo $ "Feeding " <> processed <> " to as"
    -- `as` resolves the INCLUDE_ASM'd nonmatching .s (and include/macro.inc)
    -- here, not cpp; --MD lets us depend on them so an asm edit rebuilds this.
    withTempFile $ \depFile -> do
      cmd_ (FileStdin processed) as asFlags ["--MD", depFile, "-o", out]
      neededAsmDeps depFile

  buildDir <//> "*.bin.o" %> \out -> do
    let fileComponent = makeRelative buildDir out
        target = takeDirectory1 fileComponent
        asset = genDir </> target </> assetsDir </> dropExtension (dropDirectory1 fileComponent)
    -- The linker script places assets with ELF section syntax `x.bin.o(.data)`,
    -- which needs a real relocatable object, not a raw blob (matches Makefile).
    need [asset]
    liftIO $ IO.createDirectoryIfMissing True (takeDirectory out)
    cmd_ ld ["-r", "-b", "binary", "-o", out, asset]

mainRules :: Rules ()
mainRules = do
  want [mainExe]

  priority 2 $ generator mainGen [mainGenDir <//> "*"] $ do
    -- genereratedFiles <- getGeneratedFiles mainGen
    let splatFile = configDir </> "splat.main.exe.yaml"
    need [splatFile, "disks/tenchu/main.exe", configDir </> "symbols.main.exe.txt"]
    -- Remove the gen directory if it's present: this means that if some file
    -- changes upstream, we can regenerate easily without worrying about having
    -- stale things around.
    -- liftIO $ removeFiles mainGenDir ["//"]
    liftIO $ mapM_ (\d -> IO.createDirectoryIfMissing True (mainGenDir </> d)) splatDirs
    cmd_ "split.py" [splatFile]
    -- Clean up paths in linker to not be buildDir </> genDir as we want to put
    -- object files in separate place for aesthetics or whatever.
    let mainBuildDir = buildDir </> "main.exe"
        beforeAsm = mainBuildDir </> mainGenDir </> asmDir
        beforeSrc = mainBuildDir </> mainGenDir </> srcDir
        beforeAssets = mainBuildDir </> mainGenDir </> assetsDir
    cmd_ "sed" ["-i", "s|" <> beforeAsm <> "|" <> mainBuildDir <> "|g", mainGenDir </> linkerDir </> "main.exe.ld"]
    cmd_ "sed" ["-i", "s|" <> beforeSrc <> "|" <> mainBuildDir <> "|g", mainGenDir </> linkerDir </> "main.exe.ld"]
    cmd_ "sed" ["-i", "s|" <> beforeAssets <> "|" <> mainBuildDir <> "|g", mainGenDir </> linkerDir </> "main.exe.ld"]

  let mainElf = mainExe <.> "elf"
  mainElf %> \out -> do
    _generatedFiles <- getGeneratedFiles mainGen
    mainSFiles <- liftIO $ getDirectoryFilesIO (mainGenDir </> asmDir) ["*.s", "data/*.s"]
    cFiles <- liftIO $ do
      userFiles <- Set.fromList <$> getDirectoryFilesIO (srcDir </> "main.exe") ["//*.c"]
      genFiles <- Set.fromList <$> getDirectoryFilesIO (mainGenDir </> "src") ["//*.c"]
      pure $ Set.toList (userFiles <> genFiles)
    mainAssetFiles <- liftIO $ getDirectoryFilesIO (mainGenDir </> assetsDir) ["//*.bin"]
    let mainOFiles = map (\f -> buildDir </> "main.exe" </> f <.> "o") (cFiles <> mainSFiles <> mainAssetFiles)
        ldFile = mainGenDir </> linkerDir </> "main.exe.ld"
        definedSymbols = configDir </> "symbols.main.exe.txt"
        undefinedSymbols = mainGenDir </> metaDir </> "undefined_symbols_auto.main.exe.txt"
        undefinedFunctions = mainGenDir </> metaDir </> "undefined_functions_auto.main.exe.txt"
        mainSFilesExp = map (\f -> mainGenDir </> asmDir </> f) mainSFiles
        -- mainCFilesExp = map (\f -> srcDir </> "main.exe" </> f) mainCFiles
        mainAssetFilesExp = map (\f -> mainGenDir </> assetsDir </> f) mainAssetFiles
    need (mainSFilesExp <> mainAssetFilesExp <> mainOFiles <> [ldFile, undefinedSymbols, undefinedFunctions])
    -- need [mainCFilesExp]

    liftIO $ IO.createDirectoryIfMissing True (buildDir </> "tenchu")
    cmd_
      ld
      ldFlags
      [ "-o",
        out,
        "-Map",
        buildDir </> "tenchu" </> "main.exe.map",
        "-T",
        ldFile,
        -- -T <( cut -d '/' -f1 $(CONFIG_DIR)/symbols.main.exe.txt ) \
        "-T",
        definedSymbols,
        "-T",
        undefinedSymbols,
        "-T",
        undefinedFunctions,
        "--no-check-sections",
        "-nostdlib",
        "-s"
      ]

  mainExe %> \_out -> do
    need [mainElf]
    cmd_ objcopy objcopyFlags [mainElf, mainExe]

phonyRules :: Rules ()
phonyRules = do
  phony "clean" $ do
    liftIO $
      removeFiles
        "."
        [genDir, buildDir, processedDir]

  phony "extract_main.exe" $ do
    need [mainGen]

  phony "check" $ do
    let reference = "disks" </> "tenchu" </> "main.exe"
    need [mainExe, reference]
    StdoutTrim ref <- cmd "sha256sum" reference
    StdoutTrim ours <- cmd "sha256sum" mainExe
    let refSha = head $ words ref
        ourSha = head $ words ours
    -- Guard against a swapped/corrupt base image: the reference we diff against
    -- must itself be the known-good Tenchu main.exe, not just self-consistent
    -- with whatever splat happened to extract.
    when (refSha /= expectedSha256) $
      fail $ unwords ["Reference", reference, "has sha256", refSha, "but expected known-good", expectedSha256, "- wrong/corrupt base image?"]
    when (ourSha /= expectedSha256) $
      fail $ unwords ["Expected", mainExe, "to have sha256 of", expectedSha256, "but it's", ourSha]

-- | Gets the absolute root directory of the project. Errors on
-- failure. Assumes we're somewhere inside the project.
getProjectRoot :: IO FilePath
getProjectRoot = do
  currentDir <- IO.getCurrentDirectory
  let loop "/" =
        ioError $
          userError $
            "getProjectRoot: couldn't find source tree in " ++ currentDir
      loop path =
        IO.doesFileExist (path </> ".project.root") >>= \case
          True -> return path
          False -> loop (takeDirectory path)
  loop currentDir

data GetGenId = SplatGenId
  { genFile :: String,
    patterns :: [FilePattern]
  }
  deriving (Generic, Show, Eq)

instance Hashable GetGenId

instance NFData GetGenId

instance Binary GetGenId

type instance RuleResult GetGenId = UUID.UUID

data GenData = GenData
  { lastRunId :: UUID.UUID,
    -- TODO: I think we should not only store paths of all the files but also
    -- hashes. If someone tries to change a generated file by hand, we should
    -- retrigger!
    generatedFiles :: [FilePath]
  }
  deriving
    (Generic)

instance A.ToJSON GenData where
  toEncoding = A.genericToEncoding A.defaultOptions

instance A.FromJSON GenData

runIdOracle :: GetGenId -> IO UUID.UUID
runIdOracle genId =
  IO.doesFileExist logFile >>= \case
    True ->
      A.decodeFileStrict' logFile >>= \case
        Just GenData {lastRunId, generatedFiles} -> do
          filesOnDisk <- getDirectoryFilesIO "" (patterns genId)
          if filesOnDisk /= generatedFiles
            then nextRandom
            else pure lastRunId
        Nothing -> nextRandom
    False -> nextRandom
  where
    logFile = genFile genId

recordGeneratedFiles :: UUID.UUID -> FilePath -> [FilePattern] -> IO ()
recordGeneratedFiles runId out patterns = do
  filesCreated <- getDirectoryFilesIO "" patterns
  A.encodeFile out $ GenData {lastRunId = runId, generatedFiles = filesCreated}

generator :: FilePath -> [FilePattern] -> Action () -> Rules ()
generator out generatedPatterns generationCmd = do
  generatedPatterns |%> \_ -> need [out]
  out %> \_ -> do
    runId <- askOracle $ SplatGenId {genFile = out, patterns = generatedPatterns}
    liftIO $ removeFiles "" generatedPatterns
    generationCmd
    liftIO $ recordGeneratedFiles runId out generatedPatterns

getGeneratedFiles :: FilePath -> Action [FilePath]
getGeneratedFiles out = do
  need [out]
  liftIO $
    A.decodeFileStrict' out >>= \case
      Just GenData {generatedFiles} -> pure generatedFiles
      Nothing ->
        fail "GenData decode failure in getGeneratedFiles"
