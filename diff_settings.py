def apply(config, args):
    
    config["baseimg"] = "disks/tenchu/main.exe"
    config["myimg"] = ".shake/build/tenchu/main.exe"
    config["mapfile"] = ".shake/build/tenchu/main.exe.map"
    # config["source_directories"] = ["."]
    # config["show_line_numbers_default"] = True
    config["arch"] = "mips"
    # config["map_format"] = "gnu" # gnu, mw, ms
    # config["build_dir"] = "build/" # only needed for mw and ms map format
    # config["expected_dir"] = "expected/" # needed for -o
    # config["makeflags"] = []
    config["objdump_executable"] = "mipsel-unknown-linux-gnu-objdump"
    pass