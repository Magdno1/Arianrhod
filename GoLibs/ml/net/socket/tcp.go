package socket

import (
    . "fmt"
    "net"
    "time"
)

var (
    AlreadyConnectedError   = Errorf("already connected to server")
)

type TcpSocket struct {
    conn            net.Conn
    ReadTimeout     time.Duration
    WriteTimeout    time.Duration
}

func NewTcpSocket() Socket {
    tcp := &TcpSocket{}
    return tcp
}

func (self *TcpSocket) Connect(host string, port int, timeout time.Duration) {
    var err error

    if self.conn != nil {
        RaiseSocketError(AlreadyConnectedError)
    }

    switch {
        case timeout <= 0:
            self.conn, err = net.Dial("tcp", Sprintf("%s:%d", host, port))

        default:
            self.conn, err = net.DialTimeout("tcp", Sprintf("%s:%d", host, port), timeout)
    }

    RaiseSocketError(err)
}

func (self *TcpSocket) Read(n int) (buf []byte) {
    var err error

    if n == 0 {
        return
    }

    buf = make([]byte, n)

    if self.ReadTimeout > 0 {
        self.conn.SetReadDeadline(time.Now().Add(self.ReadTimeout))
    }

    n, err = self.conn.Read(buf)
    RaiseSocketError(err)

    return buf[:n]
}

func (self *TcpSocket) ReadAll(n int) (buf []byte) {
    if n == 0 {
        return
    }

    blockSize := 0x1000
    for n > 0 {
        if n < 0x1000 {
            blockSize = n
        }

        block := self.Read(blockSize)
        buf = append(buf, block...)
        n -= len(block)
    }

    return buf
}

func (self *TcpSocket) Write(buf []byte) (n int) {
    if len(buf) == 0 {
        return 0
    }

    if self.WriteTimeout > 0 {
        self.conn.SetWriteDeadline(time.Now().Add(self.WriteTimeout))
    }

    var err error

    for n != len(buf) {
        sent := 0
        sent, err = self.conn.Write(buf)
        RaiseSocketError(err)
        n += sent
    }

    return n
}

func (self *TcpSocket) Close() {
    if self.conn == nil {
        return
    }

    RaiseSocketError(self.conn.Close())
    self.conn = nil
}

func (self *TcpSocket) LocalAddr() net.Addr {
    return self.conn.LocalAddr()
}

func (self *TcpSocket) RemoteAddr() net.Addr {
    return self.conn.RemoteAddr()
}

func (self *TcpSocket) SetTimeout(t time.Duration) {
    self.SetReadTimeout(t)
    self.SetWriteTimeout(t)
}

func (self *TcpSocket) SetReadTimeout(t time.Duration) {
    self.ReadTimeout = t
}

func (self *TcpSocket) SetWriteTimeout(t time.Duration) {
    self.WriteTimeout = t
}

func (self *TcpSocket) SetDeadline(t time.Time) {
    RaiseSocketError(self.conn.SetDeadline(t))
}

func (self *TcpSocket) SetReadDeadline(t time.Time) {
    RaiseSocketError(self.conn.SetReadDeadline(t))
    self.SetTimeout(0)
}

func (self *TcpSocket) SetWriteDeadline(t time.Time) {
    RaiseSocketError(self.conn.SetWriteDeadline(t))
    self.SetTimeout(0)
}
