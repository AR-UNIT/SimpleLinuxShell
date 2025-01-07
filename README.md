## What is it?
The Simple Shell is a lightweight command-line interpreter designed to mimic the basic functionality of a Unix shell. Its primary purpose is to parse and execute user commands by interfacing with the operating system via system calls. It supports executing external programs, handling arguments, managing simple I/O redirection, and piping commands

## Limitations
1. **Basic Features Only**: The shell includes only the bare essentials— no support for scripting, environment variable management, etc.
2. **Limited Concurrency**: It can handle simple process management but lacks advanced job control, like running tasks in the background or toggling between foreground and background processes.
3. **No command history**: There is no memory/storage of previously executed commands.

## Design Overview
The Simple Shell consists of three primary components:

1. **Command Parser**:
   - Parses user input into a command and its arguments.
   - Supports tokenization for handling spaces, quotes, and basic delimiters.

2. **Executor**:
   - Uses `fork()` to create a child process and `execvp()` to execute commands.
   - Implements simple error handling to provide feedback for invalid commands.

3. **I/O Manager**:
   - Handles basic redirection (e.g., `>` for output, `<` for input).
   - Supports pipelines (e.g., `|`) for chaining commands.
   - Uses file descriptors to reroute standard input/output streams.

## Key Features
1. **Interactive Mode**: Allows users to type and execute commands interactively.
2. **Non-Interactive Mode**: Executes commands provided via a script or pipeline.
3. **Error Messaging**: Provides feedback for invalid commands or failed execution.
4. **Process Management**: Executes commands in child processes, isolating them from the shell.

## Architecture Diagram
```
+-------------------+
|   User Input      |
+-------------------+
          |
          v
+-------------------+
|   Command Parser  |
+-------------------+
          |
          v
+-------------------+
|     Executor      |
+-------------------+
          |
          v
+-------------------+
|   I/O Management  |
+-------------------+
          |
          v
+-------------------+
| System Calls (OS) |
+-------------------+
```

## Potential Enhancements
1. **Environment Variables**: Add support for managing and utilizing environment variables.
2. **Tab Completion**: Provide user-friendly features like tab completion for commands and file paths.
3. **Improved Parsing**: Enhance the parser to handle more complex input scenarios.
4. **Command History**: Maintain memory of previously executed commands for better user experience.


