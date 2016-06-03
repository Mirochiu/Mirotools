#!/usr/bin/python
import os, sys
import argparse

# Solution from
# http://stackoverflow.com/questions/5943249/python-argparse-and-controlling-overriding-the-exit-status-code
class ArgumentParser(argparse.ArgumentParser):
    def error(self, message):
        # we rewrite the error function of argparse, because
        # argparse does not print the usage while input invalid argument(s)
        parser.print_help()
        self.exit(2, '%s: error: %s\n' % (self.prog, message))
        #super(ArgumentParser, self).error(message)

 # the range of pid is from 0 to 0x1fff
MIN_PID = 0
MAX_PID = 0x1fff
TS_PKT_LEN = 188
START_SYNC_CHECK_NUM = 1024*188
FLAG_DELAY_WRITE_TO_FILE = True

if __name__ == '__main__':
    # argparse from https://docs.python.org/2/library/argparse.html

    # add arg: prog='if you want to change the program name'
    #          prefix_chars='+/' for different option prefix
    parser = ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        conflict_handler='resolve')

    # add arg: nargs=
    #        None, or <not set> for 1 arg, e.g. -p PID
    #        '?' for at most 1 arg, e.g. -p [PID]
    #        '+' for at least 1 arg, e.g. -p PID [PID ...]
    #        '*' for any args, e.g. -p [PID [PID ...]]
    #        argparse.REMAINDER for all remaind arguments ....
    group = parser.add_argument_group('essential arguments')
    group.add_argument('-p', '--pid', type=int, required=True, default=argparse.SUPPRESS, help='the specified PID of ts data')
    group.add_argument('filename',    type=argparse.FileType('rb'), help='the input TS file path')
    parser.add_argument('-o', '--output', default='extracted.ts', type=argparse.FileType('wb'), nargs='?',
        dest='outfilename', help='the output TS file path')

    # you can test the args results
    #read_args = parser.parse_args(['extractpid.py','-p','12', '-pid', '22'])
    read_args = parser.parse_args()

    # debug
    #print(type(read_args))
    #print(read_args)
    #print('path=',read_args.filename)
    #print('pid=',read_args.pid)
    #print('output=',read_args.outfilename)

    if read_args.pid<0 or read_args.pid>MAX_PID:
        print >> sys.stderr, 'Invalid transport stream PID=%d, PID should be between 0 and 0x%x' % (read_args.pid, MAX_PID)
    else:
        print 'Specified the PID is %d(0x%04X)' % (read_args.pid,read_args.pid)

    # find first sync byte
    first_sync_byte_pos = -1
    if read_args.filename != None:
        data = bytearray(START_SYNC_CHECK_NUM)
        ret = read_args.filename.readinto(data)
        if ret != None:
            first_sync_byte_pos = data.find(bytearray(b'\x47'))
        del data

    # pick up the specified PID
    if first_sync_byte_pos >= 0:
        print 'Found ts sync byte start @',first_sync_byte_pos
        read_args.filename.seek(first_sync_byte_pos, 0)
        byte_cnt = first_sync_byte_pos
        pkt_cnt = 0     # Total TS packet count
        pid_pkt_cnt = 0 # speficied PID packet count
        sind_cnt = 0    # TS payload start indicator counter
        dropped_pid_pkt_cnt = 0
        written_len = 0
        cc_error_cnt = 0
        last_cc = -1

        if FLAG_DELAY_WRITE_TO_FILE:
            has_payload_packet_discont = 0
            concated_payload_data = bytearray()

        data = bytearray(TS_PKT_LEN)
        ret = read_args.filename.readinto(data)
        while ret==TS_PKT_LEN:
            if data[0] != 0x47:
                print >> sys.stderr, 'Invalid transport stream sync byte(%d)  @ %d' % (data[0],byte_cnt)
                break
            pid = ((data[1]<<8)|data[2]) & 0x1fff
            if pid == read_args.pid:
                pid_pkt_cnt += 1
                # debug
                #print "Found specified PID, header:%02x,%02x,%02x,%02x @ %d" % (data[0],data[1],data[2],data[3],byte_cnt)

                # if you want to extract specified pid TS packet, write out the data here.
                #read_args.outfilename.write(data)
                #written_len += len(data)

                # check adaption field
                has_adafield = data[3]&0x20
                discontinuity_indicator = 0
                if has_adafield:
                    # ISO 13818-1 Section 2.4.3.5
                    # adaption field len=0 means inserting a single suffing byte.
                    if data[5] == 0:
                        data[5] = 1
                    # ISO 13818-1 Section 2.4.3.5
                    # adapt_field_length is an 8-bytes fiedl specfiying the number of bytes in
                    # the adaption_field immediately following the adaption_field_length.
                    # So, payload offset is
                    # TS header + adaption field length + 1 (adaption field length itself)
                    payload_offset = 4 + data[5] + 1
                    if payload_offset>=5:
                        discontinuity_indicator = data[6] & 0x80
                        random_access_indicator = data[6] & 0x40
                        es_priority_indicator = data[6] & 0x20
                else:
                    payload_offset = 4  # TS header length

                # check payload data
                has_payload = data[3]&0x10
                if has_payload:
                    # check payload cc error
                    cc = data[3]&0xf # 4-bit
                    if last_cc == -1 or discontinuity_indicator:
                        expected_cc = cc
                    else:
                        expected_cc = (last_cc+1) & 0xf
                    if expected_cc != cc:
                        print >> sys.stderr, 'continue counter error, expected(%d) got (%d)  @ %d' % (expected_cc,cc)
                        cc_error_cnt += 1
                    last_cc = cc

                    # check payload status and determine the write out length
                    is_start_indicator = data[1]&0x40
                    if is_start_indicator:
                        sind_cnt +=1
                        write_data_len = 188-payload_offset
                        print "Found start indicator[%d] @ %-d RAW:%02X,%02X,%02X,%02X,%02X,%02X" % (
                            sind_cnt, byte_cnt,
                            data[payload_offset],data[payload_offset+1],data[payload_offset+2],
                            data[payload_offset+3],data[payload_offset+4],data[payload_offset+5])

                        if FLAG_DELAY_WRITE_TO_FILE:
                            if not has_payload_packet_discont:
                                read_args.outfilename.write(concated_payload_data)
                                written_len += len(concated_payload_data)
                            # reset delay write buffer
                            del concated_payload_data
                            concated_payload_data = bytearray()
                            has_payload_packet_discont = 0
                    else:
                        if sind_cnt <= 0:
                            write_data_len = 0 # drop data if the first start indicator not found
                            dropped_pid_pkt_cnt += 1
                            print "Dropped the specified PID packet data @ %-d " % (byte_cnt)
                        else:
                            write_data_len = 188-payload_offset

                            if FLAG_DELAY_WRITE_TO_FILE:
                                if discontinuity_indicator or (expected_cc != cc):
                                    has_payload_packet_discont = 1
                                    print "payload data discontinuity @ %-d, drop until the next start indicator" % (byte_cnt)

                    # write out the payload data
                    if write_data_len:
                        if FLAG_DELAY_WRITE_TO_FILE:
                            concated_payload_data += data[-write_data_len:]
                        else:
                            read_args.outfilename.write(data[-write_data_len:])
                            written_len += write_data_len
            pkt_cnt += 1
            byte_cnt += TS_PKT_LEN
            ret = read_args.filename.readinto(data)

        if data != None:
            del data
        if ret!=None and ret!=TS_PKT_LEN:
            print >> sys.stderr, 'WARNING: Not enough data(%d) to parse TS packet(188) @ %d' % (ret,byte_cnt)

        # print information
        print 'Specified PID packet count: %d' % pid_pkt_cnt
        print '              dropped packet count: %d' % dropped_pid_pkt_cnt
        print '              start indicator count: %d' % sind_cnt
        print '              written payload bytes: %d' % written_len
        print '              continuity counter error count: %d' % cc_error_cnt
        print 'Total parsed TS packet count: %d' % pkt_cnt
        if FLAG_DELAY_WRITE_TO_FILE and len(concated_payload_data)>0:
            print 'Found incompleted payload data, length(%d)' % len(concated_payload_data)
        else:
            print '(without dropped the packet discontinuity)'
    else:
        print >> sys.stderr, 'Cannot find the first SYNC byte of transport stream'

    read_args.filename.close()
    read_args.outfilename.flush()
    read_args.outfilename.close()
    del read_args.filename
    del read_args.outfilename
