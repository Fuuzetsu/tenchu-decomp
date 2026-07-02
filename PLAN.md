We initially attempted to set up the repository such that it splits up the
binary into various relevant section and then splits up the functions into their
own ASM blocks.

The idea is then to slowly replace ASM blocks with C implementations that
_should_ produce the same ASM back. Once that's done, we would remove the ASM
block and replace it with the compiled C and get back a working game.

However I think at the moment something doesn't work: the splitting is bad or we
can't put the game back together properly. Figure out what's wrong. Search the
web for examples of other projects that work like this, there are a few. Fix any
bugs in the shake build system or Make system (I think shake was supposed to
replace Make?).

No need to reverse engineer anything now, we just want to make sure we can
disassemble the game and put it back together, I think.