#!/usr/bin/env python
# -*- coding: utf8 -*-
#
# A simple direct connect bot that periodically downloads
# filelists of all hub's users into a local directory.
#
# TODO:
#   * during the first launch bot shouldn't try to connect
#     to everyone at once.
#   * human interaction: chat, web search frontend
#
import os, sys, socket, threading, optparse, time, select, array, traceback

def log(msg):
    sys.stdout.write(msg)
    sys.stdout.flush()

def split_msg(message):
    i = message.find(' ')
    if i == -1:
        return message, ''
    else:
        return message[:i], message[(i+1):]

# stolen from some open source project
def lock2key2(lock):
    "Generates response to $Lock challenge from Direct Connect Servers"
    lock = array.array('B', lock)
    ll = len(lock)
    key = list('0'*ll)
    for n in xrange(1,ll):
        key[n] = lock[n]^lock[n-1]
    key[0] = lock[0] ^ lock[-1] ^ lock[-2] ^ 5
    for n in xrange(ll):
        key[n] = ((key[n] << 4) | (key[n] >> 4)) & 255
    result = ""
    for c in key:
        if c in (0, 5, 36, 96, 124, 126):
            result += "/%%DCN%.3i%%/" % c
        else:
            result += chr(c)
    return result


class Connection(object):
    """Parses DC messages from a TCP stream."""

    def __init__(self, sock):
        self.sock = sock
        self.buf = ''
        self.pos = 0

    def recv(self):
        res = ''
        start = self.pos
        while True:
            if self.pos == len(self.buf):
                res += self.buf[start:self.pos]
                self.buf = self.sock.recv(1024)
                if len(self.buf) == 0:
                    raise Exception('Connection closed')
                self.pos = 0
                start = 0

            if self.buf[self.pos] == '|':
                res += self.buf[start:self.pos]
                self.pos += 1
                return res

            self.pos += 1

    def recv_file(self, length, fobj):
        while length > 0:
            if self.pos == len(self.buf):
                self.buf = self.sock.recv(min(length, 1024))
                if len(self.buf) == 0:
                    raise Exception('Connection closed')
                self.pos = 0

            if len(self.buf) - self.pos < length:
                fobj.write(self.buf[self.pos:])
                length -= len(self.buf) - self.pos
                self.pos = len(self.buf)
            else:
                fobj.write(self.buf[self.pos:(self.pos + length)])
                self.pos += length
                break

    def send(self, str):
        log('Sending: %s\n' % str)
        self.sock.send(str)

    def close(self):
        self.sock.close()


class IndexRecord(object):
    def __init__(self, username):
        self.username = username
        self.last_check_initiated = None
        self.last_check_completed = None
        self.connected = False
        self.filename = None
        self.temp_filename = None


class Index(object):
    """Stores users list, users' informations, serializes it to disk."""

    def __init__(self, options):
        self.table = dict()
        self.options = options
        self.index_path = os.path.join(self.options.logs_dir, 'index')
        self.index_path_temp = self.index_path + '.temp'

    def find(self, name):
        return self.table.get(name, None)

    def create(self, name):
        assert name not in self.table
        if '\t' in name:
            return None
        rec = IndexRecord(name)
        rec.filename = os.path.join(self.options.logs_dir, self.clean_username(name) + '.xml.bz2')
        rec.temp_filename = rec.filename + '.temp'
        self.table[name] = rec
        return rec
    
    def clean_username(self, name):
        for ch in '/\\*?$ \t\r\n%&$!@^()[]':
            name = name.replace(ch, '_')
        if name.startswith('.'):
            name = '_' + name[1:]
        return name

    def load(self):
        self.table = dict()
        if os.path.exists(self.index_path):
            for line in file(self.index_path):
                name, tm, filename = line.strip().split('\t')
                rec = self.create(name)
                if rec is not None:
                    rec.last_check_completed = int(tm)

    def save(self):
        fp = file(self.index_path_temp, 'w')
        for key, rec in self.table.iteritems():
            if rec.last_check_completed is not None:
                fp.write('%s\t%s\t%s\n' % (rec.username, int(rec.last_check_completed), os.path.basename(rec.filename)))
        fp.close()
        os.rename(self.index_path_temp, self.index_path)


class PeerThread(object):
    """Communicates with a peer client."""

    def __init__(self, sock, parent):
        self.conn = Connection(sock)
        self.parent = parent
        self.peer_nick = None
        self.peer_lock = None
        self.user_rec = None

    def __call__(self):
        try:
            self.run()
        except:
            if self.user_rec is not None:
                self.parent.mutex.acquire()
                self.user_rec.connected = False
                self.parent.mutex.release()

            traceback.print_exc()
            sys.exit(1)
        self.conn.sock.close()

    def run(self):
        log('Incoming connection from %s\n' % (self.conn.sock.getpeername(),))
        self.conn.sock.settimeout(60)
        self.conn.send('$MyNick %s|$Lock %s|$Supports ADCGet XmlBZList|' %
                (self.parent.options.nick, 'EXTENDEDPROTOCOLABCABCABCABCABCABC Pk=DCPLUSPLUS0.706ABCABC'))

        while True:
            msg = self.conn.recv()
            log('Received from peer: %s\n' % msg)
            head, tail = split_msg(msg)
            if head == '$MyNick':
                self.peer_nick = tail
                self.parent.mutex.acquire()
                self.user_rec = self.parent.index.find(self.peer_nick)
                if self.user_rec is not None:
                    self.user_rec.connected = True
                self.parent.mutex.release()
                if self.user_rec is None:
                    return
            elif head == '$Lock':
                self.peer_lock = tail
            elif head == '$Supports':
                continue
            elif head == '$Direction':
                direction, num = tail.split()
                self.conn.send('$Direction Download %s|$Key %s|' % (int(num) + 42, lock2key2(self.peer_lock)))
                self.conn.send('$ADCGET file files.xml.bz2 0 -1|')
            elif head == '$ADCSND' and tail.startswith('file files.xml.bz2 0 '):
                filelen = int(tail.split()[3])
                fp = file(self.user_rec.temp_filename, 'wb')
                self.conn.recv_file(filelen, fp)
                fp.close()
                os.rename(self.user_rec.temp_filename, self.user_rec.filename)
                self.user_rec.last_check_completed = time.time()
                log('Successfully downloaded filelist from %s\n' % self.peer_nick)

                self.parent.mutex.acquire()
                self.parent.index.save()
                self.parent.mutex.release()
                return


class Bot(object):
    def __init__(self, options):
        self.conn = None
        self.conn_state = 0
        self.mutex = threading.Lock()
        self.active_users = set()
        self.index = Index(options)
        self.index.load()
        self.send_queue = []
        self.options = options
        self.local_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.local_sock.bind(('0.0.0.0', 0))
        self.local_sock.listen(5)
        if not os.path.exists(options.logs_dir):
            os.makedirs(options.logs_dir)

    def connect(self):
        try:
            if self.conn is not None:
                self.conn.close()
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((self.options.server, int(self.options.port)))
            self.conn = Connection(sock)
            self.conn_state = 1
        except:
            self.conn = None
            self.conn_state = 0
            raise

    def run(self):
        while True:
            if self.conn is None:
                try:
                    log('Connecting to server\n')
                    self.connect()
                except:
                    traceback.print_exc()
                    log('Connection failed. Retrying in 60 seconds\n')
                    time.sleep(60)
                    continue

            ready = select.select([self.conn.sock.fileno(), self.local_sock.fileno()], [], [], 1)[0]

            if self.local_sock.fileno() in ready:
                peer_sock, peer_addr = self.local_sock.accept()
                threading.Thread(target=PeerThread(peer_sock, self)).start()

            try:
                message = None
                if self.conn.sock.fileno() in ready:
                    message = self.conn.recv()
            except:
                traceback.print_exc()
                self.conn = None
                self.conn_state = 0
                log('Trying to reconnect to server in 60 seconds\n')
                time.sleep(60)
                continue

            self.mutex.acquire()
            if message is not None:
                self.dispatch(message)
            for user in self.active_users:
                self.check_user(user)
            send_data = ''.join(self.send_queue)
            self.send_queue = []
            self.mutex.release()

            if len(send_data) != 0:
                self.conn.send(send_data)

    def dispatch(self, message):
        log('Received from server: %s\n' % message)
        sys.stdout.flush()

        head, tail = split_msg(message)

        if head == '$Lock':
            if self.conn_state == 1:
                self.enqueue('$ValidateNick %s|' % self.options.nick)
        elif head == '$HubName':
            pass
        elif head == '$ValidateDenide':
            raise Exception('Nick "%s" is already used')
        elif head == '$Hello':
            if self.conn_state == 1 and tail.strip() == self.options.nick:
                self.conn_state = 2
                self.enqueue('$MyINFO $ALL %s %s$ $%s $%s $ 0 $|' %
                    (self.options.nick, self.options.interest, self.options.speed, self.options.email))
            self.enqueue('$GetNickList|')
        elif head == '$NickList':
            self.active_users = set([s for s in tail.split('$$') if len(s) > 0])

    def check_user(self, user):
        if user == self.options.nick:
            return

        user_rec = self.index.find(user)
        if user_rec is None:
            user_rec = self.index.create(user)
            if user_rec is None:
                return

        if user_rec.connected:
            return
        if user_rec.last_check_completed is not None:
            if (time.time() - user_rec.last_check_completed) < int(self.options.recheck_time):
                return
        if user_rec.last_check_initiated is not None:
            if (time.time() - user_rec.last_check_initiated) < int(self.options.recheck_time_after_failure):
                return

        user_rec.last_check_initiated = time.time()
        self.enqueue('$ConnectToMe %s %s:%s|' % (user, self.conn.sock.getsockname()[0], self.local_sock.getsockname()[1]))

    def enqueue(self, msg):
        assert msg.endswith('|')
        self.send_queue.append(msg)


def main():
    parser = optparse.OptionParser()
    parser.add_option('--nick', dest='nick', default='indexbot')
    parser.add_option('--interest', dest='interest', default='<indexbot V:0.1>')
    parser.add_option('--speed', dest='speed', default='1000')
    parser.add_option('--email', dest='email', default='')
    parser.add_option('--server', dest='server', default='192.168.80.1')
    parser.add_option('--port', dest='port', default='411')
    parser.add_option('--recheck', dest='recheck_time', default='7200')
    parser.add_option('--recheck-after-failure', dest='recheck_time_after_failure', default='300')
    parser.add_option('--logs-dir', dest='logs_dir', default='/tmp/dc-indexbot')
    (options, args) = parser.parse_args()

    bot = Bot(options)
    try:
        bot.run()
    except:
        traceback.print_exc()
        sys.exit(1)

if __name__ == '__main__':
    main()
