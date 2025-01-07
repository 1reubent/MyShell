#!/bin/bash
cat testdir/bar
echo echo | which ls
ls | wc -l
  # wc counts the number of lines in its input

echo "Hello, world!" > outputs/output.txt

sort -r < inputs/input.txt
ls | grep txt > outputs/textfiles.txt
