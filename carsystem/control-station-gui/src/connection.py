from enum import Enum
import socket
import threading
import time

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
        self.running = 0
        self.threads = []

    def getConnectionType(self):
        return self.connection_type

    def connect(self):
        return False

    def stop(self):
        self.running = 0
        for t in self.threads:
            t.stop()


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
        while self.running:
            recv_data = self.conn.recv(1024)  # Should be ready to read
            if recv_data:
                self.input_queue.put(recv_data)

        self.logger.info("Finishing tcp read thread.")

    def write(self):
        self.logger.info("Starting tcp write thread.")
        while self.running:
            while not self.output_queue.empty():
                item = self.output_queue.get()
                s = item.bufferToHexString();
                self.logger.info(f"Sending message [%s]", s)
                s = s + '\n'
                self.conn.sendall(s.encode('ascii'))

            time.sleep(self.wait_seconds)
        self.logger.info("Finishing tcp write thread.")

    def connect(self):

        self.logger.info("Reading tcp connection properties.")
        host = self.config.getProperty(tag_tcp_server)
        port = self.config.getProperty(tag_tcp_port)

        self.logger.info(f"Host: {host} Port: {port}")

        if host is None or port is None:
            return False

        self.logger.info(f"Trying to establish connection to {host} on {port}")
        try:
            self.conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.conn.connect((host, port))
            self.conn.settimeout(2)
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


class MQTTClient(Connection):
    def __init__(self, input_queue, output_queue, config):
        self.connection_type = ConnectionType.MQTT
