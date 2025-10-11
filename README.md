# Text Editor

A Text Editor written in C++ using the FLTK GUI library.

# Dependencies

## Linux

### Ubuntu/Debian
```
sudo apt install libfltk1.3-dev clang
```
### Arch
```
sudo pacman -Syu clang fltk
```
### Fedora
```
sudo dnf install clang fltk fltk-devel
```

## Windows (MSYS2)
```
pacman -S mingw-w64-x86_64-toolchain mingw-w64-x86_64-fltk
```

# Compile

## Linux

```
clang++ -lfltk main.cpp -o editor
```

## Windows (MSYS2)

```
g++ main.cpp -static -static-libgcc -static-libstdc++ -lfltk -lcomctl32 -lcomdlg32 -lgdi32 -luser32 -ladvapi32 -lshell32 -lole32 -luuid -lws2_32 -lwinspool -o Editor.exe
```
