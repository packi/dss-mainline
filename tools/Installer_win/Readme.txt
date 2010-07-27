Installscript for dSS4win

GENERATING A SETUP.EXE FILE
===========================
1. Install NSIS (Nullsoft Scriptable Install System) on your computer
   you can get it from: http://nsis.sourceforge.net/Download

2. Get the neccesary files from: http://developer.digitalstrom.org/download/dss/contrib/dss_win32_installer_files.zip
   and extract them to an empty directory

3. Copy the installer.nsi file from the dss-mainline repository to the directory where you copied the files from step 2

4. Compile the installer.nsi using NSIS

5. The compiler generates a file named "Setup dSS4win.exe", this is the finished installer

NOTES
=====
1. The installer was tested under Windows XP and Windows 7

2. For a proper installation administrator privileges are required