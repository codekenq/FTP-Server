#!/usr/bin/env python2.7
#-*-coding:utf8-*-

import os
import socket
import sys
from multiprocessing import Process

def run_proc(sock):
    while(True):
        s = sock.recv(1024)
        print s

def initClient(ip, port):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect((ip, int(port)))
    except socket.error, arg:
        (errno, err_msg) = arg
        print "Connect to server (%s, %s) failed: %s, errno=%d" % (ip, port, err_msg, errno)
        os._exit()
    return s

if __name__ == '__main__':
    ip = raw_input('请输入ip\n')
    port = raw_input('请输入port\n')

    s = initClient(ip, port)
    p = Process(target = run_proc, args = (s,))
    p.start()
    while(True):
        cmd = raw_input()
        s.send(cmd)
