# mkpsxiso + dumpsxiso: build/rebuild PSX CD images (for `./Build iso`).
# Packaged in-repo because it isn't in nixpkgs. FLAC is disabled — we don't
# encode CD-DA, the audio track is a raw .bin.
{ stdenv, fetchFromGitHub, cmake }:

stdenv.mkDerivation {
  pname = "mkpsxiso";
  version = "2.11-unstable-633adc6";

  src = fetchFromGitHub {
    owner = "Lameguy64";
    repo = "mkpsxiso";
    rev = "633adc6778b7a7c677eecaebc7da41bd19068048";
    fetchSubmodules = true;
    hash = "sha256-HeodzX/G/0OPIjLsIGSYDUUvnxcCGg1J61CcQ/nCcwo=";
  };

  nativeBuildInputs = [ cmake ];

  cmakeFlags = [
    "-DMKPSXISO_NO_LIBFLAC=ON"
    "-DCMAKE_BUILD_TYPE=Release"
  ];
}
