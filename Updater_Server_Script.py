import socket
import time
import threading
import tkinter as tk
from tkinter import filedialog
from pathlib import Path
import binascii


############################
global block_list
global try_num
global lines_num
global end_of_lines
block_list = []


#configuration##############
lines_num = 20
try_num = 5
end_of_lines = "next"
############################


def client_handel(c,addr,file_path,version):
    print("--------------------------------------------------------------------------------")
    print("address= " , addr)
    print("client connected\ndiscription= " , c)
    try:
        c.sendall(bytes("send master id" , "utf-8"))
        data = c.recv(1024)
        print(data)
        if data == b'master id' :
            c.sendall(bytes("send atentication information" , "utf-8"))
            data=c.recv(1024)
            print(data)    
            if data == b'username:user , password:pass' :
                c.sendall(bytes("software version?" , "utf-8"))
                data = c.recv(1024)
                print(data)
                if data != bytes(version , "utf-8"):
                    w = try_num
                    while w:
                        c.sendall(bytes("version=" + version , "utf-8"))
                        data = c.recv(1024)
                        print (data)
                        if data == b'data recived sucsessfully' :
                            break
                        w = w - 1
                    if not(w):
                        c.close()
                        print("client dose not aviable or poor connection cannot send data")
                        print("--------------------------------------------------------------------------------")
                        return
                    a = lines_num
                    send_buf = b''
                    with open(file_path) as file:#, mode='rb'
                        for line in file:
                            #if end_of_lines in line.decode("latin1"):
                            a = a - 1
                            send_buf = send_buf + binascii.unhexlify(''.join(line.split()))
                            if not(a) and send_buf != b'':
                                #print(send_buf)
                                w = try_num
                                while w:
                                    c.sendall(bytes("begin" , "utf-8"))
                                    c.sendall(send_buf)
                                    c.sendall(bytes("end" , "utf-8"))
                                    data = c.recv(1024)
                                    print(data)
                                    if data == b'data recived sucsessfully' :
                                        break
                                    w = w - 1
                                if not(w):
                                    c.close()
                                    print("client dose not aviable or poor connection cannot send data")
                                    print("--------------------------------------------------------------------------------")
                                    return
                                send_buf = b''
                                a = lines_num
                        if send_buf != b'':
                            w = try_num
                            while w:
                                c.sendall(bytes("begin" , "utf-8"))
                                c.sendall(send_buf)
                                c.sendall(bytes("end" , "utf-8"))
                                data = c.recv(1024)
                                print(data)
                                if data == b'data recived sucsessfully' :
                                    break
                                w = w - 1
                            if not(w):
                                c.close()
                                print("client dose not aviable or poor connection cannot send data")
                                print("--------------------------------------------------------------------------------")
                                return
                else:
                    c.sendall(bytes("dont need update" , "utf-8"))
                    #time.sleep(0.5)
                    c.close()
                    print("master ",data.decode("utf-8"),"dont need update")
                    print("--------------------------------------------------------------------------------")
                    return
            else:
                c.close()
                block_list.append(str(addr[0]))
                print("password atentication failed")
                print("--------------------------------------------------------------------------------")
                return
        else:
            c.close()
            block_list.append(str(addr[0]))
            print("invalid master id")
            print("--------------------------------------------------------------------------------")
            return
    except  BaseException as err:
        print("eror=  ",err)
        c.close()
        print("somting wrong cannot send data")
        print("--------------------------------------------------------------------------------")
        return
    c.sendall(bytes("sending all data sucsessfully" , "utf-8"))
    c.close()
    print("master ",data.decode("utf-8"),"was updaed sucsesfully")
    print("--------------------------------------------------------------------------------")
    return


sock = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
ip = socket.gethostbyname("192.168.200.40")
port = 2841
sock.bind((ip,port))
sock.listen(1000)
socket.setdefaulttimeout(4000)
print("please select your update file....")
root = tk.Tk()
root.withdraw()
file_path = filedialog.askopenfilename()
version = str(Path(file_path).stem)
while True:
    c , addr = sock.accept()
    if block_list.count(str(addr[0])) < 3:
        t = threading.Thread(target=client_handel, args = (c,addr,file_path,version))
        t.daemon = True
        t.start()
    else:
        c.close()
        print("address ",str(addr[0])," was blocked")
        print("--------------------------------------------------------------------------------")



