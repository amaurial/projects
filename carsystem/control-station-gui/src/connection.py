from enum import Enum
import socket
import threading
import time
import datetime

tag_tcp_server = "tcp_server"
tag_tcp_port = "tcp_port"


class ConnectionType(Enum):
    NONE = 0
    TCP = 1
    MQTT = 2


class Connection:
    def __init__(self, facilities, input_queue, output_queue):
        self.connection_type = ConnectionType.NONE
        self.input_queue = input_queue
        self.output_queue = output_queue
        self.config = facilities["configuration"]
        self.logger = facilities["logger"]
        self.builder = facilities["builder"]
        self.running = 0
        self.threads = []
        self.host = ""
        self.port = 0

    def getConnectionType(self):
        return self.connection_type

    def connect(self):
        return False

    def stop(self):
        self.running = 0

    def isConnected(self):
        if self.running == 0:
            return False
        return True


class TcpClient(Connection):
    def __init__(self, facilities, input_queue, output_queue):
        super().__init__(facilities, input_queue, output_queue)
        self.connection_type = ConnectionType.TCP
        self.conn = None
        self.wait_seconds = 0.001

    def read(self):
        self.logger.info("Starting tcp read thread.")
        label = self.builder.get_object("label_status_2")

        while self.running:
            try:
                recv_data = self.conn.recv(1024)  # Should be ready to read
                if recv_data:
                    self.input_queue.put(recv_data)
                    now = datetime.datetime.now()
                    label.set_text("Last input message time " + now.strftime("%d-%m-%Y %H:%M:%S"))
            except socket.timeout:
                pass

        self.logger.info("Finishing tcp read thread.")

    def write(self):
        self.logger.info("Starting tcp write thread.")
        label = self.builder.get_object("label_status_3")
        while self.running:
            while not self.output_queue.empty():
                item = self.output_queue.get()
                s = item.bufferToHexString()
                self.logger.info(f"Sending message [%s]", s)
                s = s + '\n'
                self.conn.sendall(s.encode('ascii'))

                now = datetime.datetime.now()
                label.set_text("Last output message time " + now.strftime("%d-%m-%Y %H:%M:%S"))

            time.sleep(self.wait_seconds)
        self.logger.info("Finishing tcp write thread.")

    def connect(self):

        self.logger.info("Reading tcp connection properties.")
        self.host = self.config.getProperty(tag_tcp_server)
        self.port = self.config.getProperty(tag_tcp_port)

        self.logger.info(f"Host: {self.host} Port: {self.port}")

        if self.host is None or self.port is None:
            return False

        self.logger.info(f"Trying to establish connection to {self.host} on {self.port}")
        try:
            self.conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.conn.connect((self.host, self.port))
            self.conn.settimeout(10)
            # start threads
            self.running = 1
            self.logger.info("Starting TCP read and write threads.")
            rthread = threading.Thread(target=self.read)
            wthread = threading.Thread(target=self.write)

            rthread.start()
            wthread.start()
            self.threads.append(rthread)
            self.threads.append(wthread)
        except Exception as e:
            self.logger.error(e)
            return False

        return True

    def get_host(self):
        return self.host

    def get_port(self):
        return self.port


class MQTTClient(Connection):
    def __init__(self, input_queue, output_queue, config):
        self.connection_type = ConnectionType.MQTT
