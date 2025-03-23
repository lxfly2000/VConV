@echo off
set makerom=makerom.exe
set bannertool=bannertool\windows-x86_64\bannertool.exe
%bannertool% makebanner -i banner.png -a ../romfs/success.ogg -o banner.bnr
%bannertool% makesmdh -s VConV -l "Virtual Controller for ViGEm" -p lxfly2000 -i ../icon.png -o icon.icn
%makerom% -f cia -o vconv.cia -DAPP_ENCRYPTED=false -rsf vconv.rsf -target t -exefslogo -elf ../vconv3ds.elf -icon icon.icn -banner banner.bnr
