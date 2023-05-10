{-# LANGUAGE DeriveGeneric #-}
{-# LANGUAGE LambdaCase #-}
{-# LANGUAGE NamedFieldPuns #-}
{-# LANGUAGE TypeFamilies #-}

import Control.Monad (when)
import qualified Data.Aeson as A
import Data.Binary (Binary)
import Data.Hashable (Hashable)
import qualified Data.UUID as UUID
import Data.UUID.V4 (nextRandom)
import Development.Shake
import Development.Shake.Classes (NFData)
import Development.Shake.FilePath
import Development.Shake.Util (neededMakefileDependencies)
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
cc = "tools/cc1-26"

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
ccFlags = ["-mcpu=3000", "-quiet", "-G0", "-w", "-O2", "-funsigned-char", "-fpeephole", "-ffunction-cse", "-fpcc-struct-return", "-fcommon", "-fverbose-asm", "-fgnu-linker", "-mgas", "-msoft-float"]

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
    cmd_ as asFlags ["-o"] out src

  processedDir <//> "*.c" %> \out -> do
    let fileComponent = makeRelative processedDir out
        target = takeDirectory1 fileComponent
        src = srcDir </> target </> dropDirectory1 fileComponent
        header = srcDir </> target </> target <.> "h"
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
      cmd_ cpp (cppFlags <> ["-MMD", "-MF", makeOut]) src out
      neededMakefileDependencies makeOut

  buildDir <//> "*.c.o" %> \out -> do
    let fileComponent = makeRelative buildDir out
        target = takeDirectory1 fileComponent
        -- src = srcDir </> target </> dropExtension (dropDirectory1 fileComponent)
        -- header = srcDir </> target </> target <.> "h"
        -- source file after cpp was ran
        processed = processedDir </> target </> dropExtension (dropDirectory1 fileComponent)

    need [processed]
    liftIO $ IO.createDirectoryIfMissing True (takeDirectory out)
    Stdout ccOut <- cmd cc ccFlags processed
    cmd_ (StdinBS ccOut) as asFlags "-o" out

  buildDir <//> "*.bin.o" %> \out -> do
    let fileComponent = makeRelative buildDir out
        target = takeDirectory1 fileComponent
        asset = genDir </> target </> assetsDir </> dropExtension (dropDirectory1 fileComponent)
    copyFile' asset out

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
  -- liftIO $ writeFile done ""

  let mainElf = mainExe <.> "elf"
  mainElf %> \out -> do
    _generatedFiles <- getGeneratedFiles mainGen
    mainSFiles <- liftIO $ getDirectoryFilesIO (mainGenDir </> asmDir) ["*.s", "data/*.s"]
    mainCFiles <- liftIO $ getDirectoryFilesIO (srcDir </> "main.exe") ["//*.c"]
    mainAssetFiles <- liftIO $ getDirectoryFilesIO (mainGenDir </> assetsDir) ["//*.bin"]
    let mainOFiles = map (\f -> buildDir </> "main.exe" </> f <.> "o") (mainCFiles <> mainSFiles <> mainAssetFiles)
        ldFile = mainGenDir </> linkerDir </> "main.exe.ld"
        definedSymbols = configDir </> "symbols.main.exe.txt"
        undefinedSymbols = mainGenDir </> metaDir </> "undefined_symbols_auto.main.exe.txt"
        undefinedFunctions = mainGenDir </> metaDir </> "undefined_functions_auto.main.exe.txt"
        mainSFilesExp = map (\f -> mainGenDir </> asmDir </> f) mainSFiles
        mainCFilesExp = map (\f -> srcDir </> "main.exe" </> f) mainCFiles
        mainAssetFilesExp = map (\f -> mainGenDir </> assetsDir </> f) mainAssetFiles
    putInfo $ show mainOFiles
    need (mainSFilesExp <> mainCFilesExp <> mainAssetFilesExp <> mainOFiles <> [ldFile, undefinedSymbols, undefinedFunctions])
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
    when (refSha /= ourSha) $ do
      fail $ unwords ["Expected", mainExe, "to have sha256 of", refSha, "but it's", ourSha]

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
