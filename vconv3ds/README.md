# VConV 3DS
Virtual Controller for VConV

## 需要的软件、硬件
* 3DS: 刷入了Luma3DS菜单，有Homebrew启动器
* Citra模拟器
* devkitPro + 3DS Dev
* VSCode编辑器，安装C/C++，Makefile Tools扩展

## 准备
1. 安装库：`pacman -S 3ds-libvorbisidec`
2. 检查.vscode文件夹内的JSON文件中的路径是否正确，如有需要请修改

## 实机调试方法
1. 在3DS上启动Homebrew Launcher，按Y等待网络传输
2. 按L+下+Select调出Luma3DS菜单，进入Debugger options选项，选择Enable Debugger（若未启用），再选择Force-debug next…选项，退出Luma3DS菜单
3. 在VSCode上打开该工程，点击左侧“运行和调试”按钮，点击“(gdb)Launch”开始调试，VSCode自动进入单步调试状态
4. 结束后如需再次调试，需要重复执行上述步骤

## 模拟器运行方法
在VSCode中打开后按下Ctrl+Shift+P，选择“Task: Run Task”，“run debug”或“run release”

## CIA构建方法
请参考[buildcia](buildcia)文件夹。

## 参考
* [ftpd](https://github.com/mtheall/ftpd)
