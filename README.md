# B.I.G: A BST-Indexed Graph Model for Version Control

**Course:** Data Structures

**Developer:** Eender  

---

## I. Introduction
B.I.G is a version control system implemented in C, inspired by Git. It utilizes a **Content-Addressable Storage (CAS)** system and a **Directed Acyclic Graph (DAG)** to manage file history efficiently.

### Key Challenges Solved
1.  **Storage Efficiency:** Instead of a naive copy-paste approach, B.I.G uses **Blobs** and **Snapshots** to avoid non-linear disk space growth.
2.  **Binary Safety:** Handles large binary files (e.g., images, videos) correctly by avoiding standard C string handling pitfalls (like premature termination at `\0`).
3.  **Safe Checkout:** Prevents data loss by prohibiting checkout operations when there are uncommitted or unstaged changes.

---

## II. System Architecture
The system follows a **Four-Layered Architecture**:

1.  **Command Layer (`src/commands/`)**
    * The entry point of the system. Handles user input (CLI arguments) and parses flags.
    * Dispatches tasks to the Core Layer.
2.  **Core Layer (`src/core/`)**
    * Contains the main logic of the VCS (Commit, Blob, Index).
    * Manages the DAG and calculates hashes.
3.  **Data Structure Layer (`src/ds/`)**
    * Contains generic data structures independent of the VCS logic.
    * Includes **BST** (Generic Binary Search Tree) and **Stack**.
4.  **Infrastructure Layer (`src/utils/`)**
    * Handles low-level system operations: memory management, file I/O, and error handling.

---

## III. Key Concepts & Data Structures

### Binary Search Tree (BST)
* Used to store the file path and corresponding hash ID in the Index.
* Ensures $O(\log n)$ search performance by converting the sorted list from the staging area into a balanced BST during loading.

### The Implicit Merkle DAG
* There is no explicit Graph structure in the code. The graph is **implicit**.
* Every commit points to its parent, linking snapshots together. Branches create divergent paths in this graph.

### Core Objects
* **Project Directory:** The root directory containing `.big/`.
* **Index (Staging Area):** Maintains a list of tracked files (path and blob hash) before committing.
* **Blob:** The fundamental unit of data storage. The filename is the **SHA-1 hash** of its content (deduplication).
* **Commit:** A snapshot folder containing the file list, metadata (log, date), and parent hash.
* **Branch:** A movable pointer (reference) to a specific commit hash.
* **Leader (HEAD):** A pointer recording "where we are" (usually points to a Branch reference).

---

## IV. Development Environment
* **Platform:** Linux (WSL2) 
* **Compiler:** GCC (C17 Standard)
* **Build Tool:** Make

---

## V. Build & Installation

### 1. Compile
Run `make` in the project root to compile all source files.

```bash
$ make
# Output example:
# Compile succeeded -> build/utils/utils.o
# ...
# Link succeeded -> bin/big
```
### 2. Setup Path
Add the binary to your environment variables or move it to your project folder.

```bash
export PATH="$PATH:/path/to/your/B.I.G/bin"
```

## VI. Usage Commands

| Command                      | Description                                                    |
| :--------------------------- | :------------------------------------------------------------- |
| `big init`                   | Initialize the current directory as the project root.          |
| `big add [-d] <file/dir>`    | Add files to the Index (Staging Area). Use `-d` to delete.     |
| `big commit [-m <msg>]`      | Commit the staged files. Opens Nano if no message is provided. |
| `big log [<amount>]`         | Show commit history logs.                                      |
| `big status`                 | Show the status of files (untracked, modified, staged).        |
| `big checkout <hash/branch>` | Restore files to a specific commit or switch branches.         |
| `big branch [-d] <name>`     | List branches or create a new branch.                          |

### Examples

**Initialize and Add files:**
```bash
$ big init
$ big add src/main.c include/
$ big status
```

Commit changes:
```bash
$ big commit -m "feat: First commit"
```

Create a branch and checkout:
```bash
$ big branch feature_A
$ big checkout feature_A
```


## VII. Future Improvements 
- **Performance vs. Accuracy**: Currently, big status calculates the hash of every file to detect changes, which results in $O(n)$ I/O operations and can be slow for large projects. Future improvements will implement timestamp or file size checks to optimize diffing 2.
- **.bigignore**: Implement a feature similar to .gitignore to exclude specific files from tracking.
