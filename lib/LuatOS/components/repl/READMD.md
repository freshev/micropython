# REPL for LuatOS

Design a REPL (Read–eval–print loop), that is, a “read-evaluate-output” loop

It is troublesome to be directly compatible with the REPL of lua.exe, and the REPL that comes with lua does not have real multi-line support.

Therefore, there are currently two modes of design

1. Simple single line, ending with `\r`/`\n`
2. Multiple lines with beginning and end, starting with `<<EOF` and ending with `EOF`

