# Breadcrumb CHooser

## Usage
Breadch is a command line chooser that is meant to be used with an external shell configuration.
An example configuration in `.bashrc`
```sh
# `ctrl`+`G` will add the current working directory to the saved breadcrumbs
bind -x '"\C-g": "breadch -a"'

# `b` will display a simple menu to quickly navigate to a breadcrumb
alias b='cd $(breadch)'

# example prompt: [user@HAX * directory]$ 
export PS1='[\u@\h $(breadch -s)\W]\$ '
```
The breadcrumbs are stored in `~/.breadcrumbs`.
