# FAT16-Filesystem-Reader
A C-based FAT16 disk image parser and reader that extracts and displays key filesystem metadata, cluster chains, directory contents, and file contents from `.img` files.

## 🔧 Features
- Boot sector parsing (volume label, sector/cluster layout)
- FAT table loading and cluster chain traversal
- Root directory listing with long filename (LFN) support
- File opening, seeking, and reading within FAT16 images
- Support for reading and displaying specific file contents (e.g., `sessions.txt`)

## 📂 Example Use
Simply place a valid FAT16 `.img` file in the project folder and run the program. You'll be prompted for a cluster number and shown directory and file contents interactively.

## 💻 Technologies Used
- C (GCC)
- Linux system calls (`open`, `read`, `lseek`)
- Custom structs for FAT16 (BootSector, DirectoryEntry, LongDirectoryEntry)

## 📦 Usage
```bash
gcc -o fat16_reader Main.c
./fat16_reader

Ensure fat16.img is in the same directory as the executable.
