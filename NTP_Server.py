import socket
import struct
import time

def RequestTimefromNtp(addr):
    addr=socket.gethostbyname(addr)
    REF_TIME_1970 = 2208988800  # Reference time
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    ntp_req = b'\x4b' + 47 * b'\x01'#=b'\x1b\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
    #for byte in ntp_req:
        #print(byte, end=' ')
    sock.connect((addr , 123))
    sock.sendall(ntp_req)
    data= sock.recv(1024)
    #print(data)
    if data:
        t = struct.unpack('!12I', data)[10]
        #for byte in data:
            #print(hex(byte), end=' ')
        #print(struct.unpack('!12I', data))
        t -= REF_TIME_1970
    return time.ctime(t), t

if __name__ == "__main__":
    print(RequestTimefromNtp('0.ir.pool.ntp.org'))
