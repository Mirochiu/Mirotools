#!/usr/bin/python
import sys, os
from hashlib import sha1

# emulate the behavior
# > git hash-object <file-path>
def githash(filepath):
    filesize_bytes = os.path.getsize(filepath)
    s = sha1()
    s.update(("blob %u\0" % filesize_bytes).encode('utf-8'))
    with open(filepath, 'rb') as f:
        s.update(f.read())
    return s.hexdigest()

if __name__ == '__main__':
    argc = len(sys.argv)
    if argc<=1:
        print 'Should give at least one file path!'
        sys.exit(1)
    for f in sys.argv[1:]:
        print f,"\t",githash(f)