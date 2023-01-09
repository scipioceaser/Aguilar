# Aguilar


Aguilar is a single command-line application that makes using C as a scripting language a lot easier. Aguilar is not itself a C compiler, and instead calls either the GNU or LLVM C compiler. It only works on Linux (Mac OS and WSL untested) at the moment.

Commands:

    - new [name]: Create a new project based on a predefined template.
    - build (file): Build either a file or a project based on whether it can find a config file.
    - sync: Update an existing repository with any changes made to template files.   
    - run [file]: Build a single file (application is stored in a cache) and run it.
    - install: Install the application in the user's bin folder.
    - help: Print everything you need to know.
    - zen: Print a zen of code.
