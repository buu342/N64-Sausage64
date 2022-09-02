@echo off
for %%f in (*.png) do (
    if "%%~xf"==".png" bin2c.exe %%f %%~nf.h %%~nf_png
)