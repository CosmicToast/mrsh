#!/bin/sh

echo ""
echo "Tilde Expansion"
echo ~ ~/stuff ~/"stuff" "~/stuff" '~/stuff' # ~"/stuff"
echo ~root
#a=~/stuff
#echo $a
#a=~/foo:~/bar:~/baz
#echo $a

echo ""
echo "Parameter Expansion"
a=a
b=B
hello=hello
null=""
echo $a ${b} ">$a<"
echo \$a '$a'
echo ${a:-BAD} ${idontexist:-GOOD} ${null:-GOOD} ${idontexist:-}
echo ${a-BAD} ${idontexist-GOOD} ${null-BAD} ${null-}
echo ${a:+GOOD} ${idontexist:+BAD} ${null:+BAD} ${idontexist:+}
echo ${a+GOOD} ${idontexist+BAD} ${null-GOOD} ${null+}
echo ${#hello} ${#null} ${#idontexist}

echo ""
echo "Command Substitution"
echo $(echo asdf)
echo `echo asdf`

echo ""
echo "Arithmetic Expansion"
#echo $((1+2))

# Field Splitting
# Pathname Expansion
# Quote Removal
