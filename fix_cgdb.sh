#!/bin/sh
if ! test -f ~/.inputrc.cgdb
then cat > ~/.inputrc.cgdb <<EOF
\$include ~/.inputrc
set enable-bracketed-paste off
EOF
fi
INPUTRC="$HOME/.inputrc.cgdb" exec "$(which -a cgdb | awk 'NR==2')" "$@"

