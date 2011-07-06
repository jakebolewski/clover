#!/usr/bin/python
# -*- coding: utf-8 -*-

# embed.py <filename> <outfile>
# <filename> => <outfile>

import sys

infile = open(sys.argv[1], 'rb')
name = sys.argv[1].split('/')[-1].replace('.', '_')
outfile = open(sys.argv[2], 'w')

data = infile.read()

# Header
outfile.write('#ifndef __%s__\n' % name.upper())
outfile.write('#define __%s__\n' % name.upper())
outfile.write('\n')
outfile.write('const char embed_%s[] =\n' % name)

# Write it in chunks of 80 chars :
# |    "\x00..."            (4+1+1 + 4*chars ==> chars = 18)
index = 0

for c in data:
    if index == 0:
        outfile.write('    "')

    outfile.write('\\x%s' % ('%x' % ord(c)).rjust(2, '0'))
    index += 1

    if index == 18:
        index = 0
        outfile.write('"\n')

# We may need to terminate a line
if index != 0:
    outfile.write('";\n')
else:
    outfile.write(';\n')     # Alone on its line, poor semicolon

# Footer
outfile.write('\n')
outfile.write('#endif\n')

infile.close()
outfile.close()
