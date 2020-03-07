# Use the C or C++ compiler
Compiler = wcc386

# compiler options
#comp = -oneatx -zp4 -5 -d0
comp = -d0

Linker_options = option stack=60000

# Compile using dos4g or pmodew
#System = dos4g
System = pmodew

Exe_file = v2chr.exe

Objs = video.obj vidmouse.obj v2chrcfg.obj pcx.obj &
       font.obj vidgui.obj v2cemod.obj v2chred.obj

####################

v2chr.exe: $(Objs)

           *wlink system $(System) $(Linker_Options) name v2chr.exe &
           file {$<}
#           *pmwlite -c4 v2chr.exe

.c.obj:
        $(Compiler) $(comp) $<

.asm.obj:
        wasm -3pr $<
