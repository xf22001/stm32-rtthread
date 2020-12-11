# -*- coding: utf-8 -*-
#!/usr/bin/env python
#================================================================
#   
#   
#   文件名称：tcp_client.py
#   创 建 者：肖飞
#   创建日期：2020年12月07日 星期一 15时19分18秒
#   修改日期：2020年12月07日 星期一 16时32分13秒
#   描    述：
#
#================================================================
import sys
import socket
import optparse

def test_loop(server, port, loop):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((server, port))
    i = 0
    while True:
        if i <= 10:
            content = 'send string' + str(i)
            s.send(content.encode())
            print(s.recv(32).decode())
        else:
            if loop <= 10:
                content = 'Q'
            else:
                content = 'exit'

            s.send(content.encode())

        i = i + 1

    s.close()


def main(argv):
    options = optparse.OptionParser()
    options.add_option('-s', '--server', dest='server', help='server', default=None)
    options.add_option('-p', '--port', type = int, dest='port', help='port', default=None)
    opts, args = options.parse_args(argv)
    print('opts:%s' %(opts))
    print('args:%s' %(args))
    if len(args):
        options.print_help()
        return

    if opts.server and opts.port:
        i = 0
        while(True):
            try:
                i += 1
                test_loop(opts.server, opts.port, i)
            except BrokenPipeError as e:
                pass
    else:
        options.print_help()

if '__main__' == __name__:
    main(sys.argv[1:])
