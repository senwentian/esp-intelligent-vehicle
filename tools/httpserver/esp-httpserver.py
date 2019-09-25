from http.server import HTTPServer, BaseHTTPRequestHandler
import json
import socket
import cgi
from cgi import parse_header, parse_multipart
import urllib
import io,shutil
import re
import os
import time
import datetime

def mkdir(path):
    path=path.strip()
    path=path.rstrip("\\")
    isExists=os.path.exists(path)

    if not isExists:
        os.makedirs(path)

def esp_open_image():
    imagedir = os.getcwd() + '/jpg'
    mkdir(imagedir)
    filename = 'SKR-' + datetime.datetime.now().strftime('%Y%m%d%H%M%S.jpg')
    return open(imagedir + '/' + filename, 'w+')

class Resquest(BaseHTTPRequestHandler):
    def handler(self):
        print("data:", self.rfile.readline().decode())
        self.wfile.write(self.rfile.readline())

    def do_GET(self):
        print(self.requestline)
        if self.path != '/hello':
            self.send_error(404, "Page not Found!")
            return

        self.send_response(200)
        self.send_header('Content-type', 'image/jpeg')
        self.end_headers()
        self.wfile.write('')

    def do_POST(self):
        length = int(self.headers['content-length'])
        pic_data = self.rfile.read(length)

        print(self.headers)
        # print(self.command)

        self.pic = esp_open_image()
        self.pic.write(pic_data)
        self.pic.close()

        self.send_response(200)
        self.send_header('Content-type', 'text/plain')
        self.end_headers()
        self.wfile.write('')

if __name__ == '__main__':
    host = ('127.0.0.1', 8070)
    server = HTTPServer(host, Resquest)
    print("Starting server, listen at: %s:%s" % host)
    server.serve_forever()
