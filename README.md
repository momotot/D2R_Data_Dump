# D2R Data Dumper

A tool to extract and dump specific file types from **Diablo II: Resurrected (D2R)** using **CascLib**. 
It also includes a Python script to convert **.sprite** files to **.png**.

The python script is heavily inspired by [d2r-sprites by dzik](https://github.com/dzik87/d2r-sprites)

## Features

- Dump files by extension (e.g., `.sprite`, `.json`).
- Can convert `.sprite` files to `.png` files

## Requirements

Make sure you have the following before using the tool:

- **Windows** (uses Windows-specific APIs).
- **Diablo II: Resurrected** installed on the default path with a `Data` folder.
- **Casclib**  [CascLib by ladislav-zezula](https://github.com/ladislav-zezula/CascLib)
- **Python 3.6+** to use the sprite conversion script (dependencies listed below).

## Python packages

To install required Python packages:

```bash
pip install pillow bitstring numpy
```

## Usage

1. Build the project and run the generated executable in the terminal.
2. To convert `.sprite` files to `.png`, place `convert_sprite_to_png.py` in the same directory as the executable.