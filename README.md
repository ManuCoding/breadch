# Breadcrumb CHooser

## Usage
Breadch is a command line chooser that is meant to be used with an external shell configuration.
An example configuration in `.bashrc`
```sh
bind -x '"\C-g": "breadch -a"'
alias b='cd $(breadch)'
```
By doing `ctrl`+`G`, it will add the current working directory to the saved breadcrumbs.
The breadcrumbs are stored in `~/.breadcrumbs`.
The command `b` would display a simple menu to quickly navigate to one of the breadcrumbs.
