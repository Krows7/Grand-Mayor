@echo off

set name=city
set src=src

::move /Y %src%\data\lib\*.h tmp\ > nul
::copy /Y true\*.h %src%\data\lib\ > nul

cc65 -g -Oirs %src%\%name%.c --add-source
ca65 crt0.s
ca65 %src%\%name%.s -g

ld65 -C nrom_32k_vert.cfg -o %name%.nes crt0.o %src%\%name%.o nes.lib -Ln labels.txt --dbgfile %name%.dbg

del *.o

::copy /Y tmp\*.h %src%\data\lib\ > nul

move /Y %name%.dbg build\ > nul
move /Y labels.txt build\ > nul
move /Y %src%\%name%.s build\ > nul
move /Y %name%.nes build\ > nul