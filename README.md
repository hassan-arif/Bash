"main.cpp" includes implementation of Custom Shell. (OS: Ubuntu)

End user enters string as if he were using Bash (the Bourne Again Shell).
Provided code tokenizes the arguments and creates child process for successful execution of each command.
After execution, the code returns to parent process and asks for next command. This continues in loop until the end user enters "exit" to exit the shell.
Additionally, support for |, < and > symbols is also added.

Screenshot (screenshot.JPG) and object file (a.out) are also attached for convenience, Enjoy!


Created in April, 2022.
