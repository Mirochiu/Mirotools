#!/usr/bin/python
import os, sys
import argparse

class ArgumentParser(argparse.ArgumentParser):
    def error(self, message):
        parser.print_help()
        self.exit(2, '%s: error: %s\n' % (self.prog, message))

MAX_READ_NUM = 10*1024*1024

if __name__ == '__main__':
    parser = ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter, conflict_handler='resolve')
    group = parser.add_argument_group('essential arguments')
    group.add_argument('-s', dest='skip',   type=int, default=0,     help='skip bytes')
    group.add_argument('-n', dest='length', required=True, type=int, help='copy byte length')
    group.add_argument('inputname',  type=argparse.FileType('rb'),   help='the input file path')
    group.add_argument('outputname', type=argparse.FileType('wb'),   help='the output file path')

    read_args = parser.parse_args()

    if read_args.length <= 0:
        print >> sys.stderr, 'ERROR: length(',read_args.length,')<=0'
        sys.exit()

    print 'skip bytes ',read_args.skip
    print 'copy length ',read_args.length

    if read_args.length > MAX_READ_NUM:
        read_length = MAX_READ_NUM
    else:
        read_length = read_args.length

    if read_args.skip >= 0:
        read_args.inputname.seek(read_args.skip, 0)
        data = bytearray(read_length)
        ret = read_args.inputname.readinto(data)
        if ret == None:
            print >> sys.stderr, 'ERROR: cannot read file data'
        else:
            while ret != None:
                flag_no_data_written = 0
                read_args.outputname.write(data)
                #print len(data)
                read_args.length -= read_length
                if read_args.length <= 0:
                    break
                elif read_args.length < MAX_READ_NUM:
                    del data
                    data = bytearray(read_args.length)
                ret = read_args.inputname.readinto(data)
        del data

    read_args.inputname.close()
    read_args.outputname.flush()
    read_args.outputname.close()
    del read_args.inputname
    del read_args.outputname
