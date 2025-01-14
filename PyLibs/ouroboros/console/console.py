from ..common import *

if sys.platform == 'win32':
    getch = ctypes.CFUNCTYPE(ctypes.c_int32)(('_getch', windll.msvcrt))

def pause(text = None):
    if text is not None:
        print(text)
    getch()

def setTitle(text):
    if sys.platform == 'win32':
        windll.kernel32.SetConsoleTitleW(str(text))

    else:
        raise NotImplementedError

def clear():
    os.system('cls')

if isinstance(sys.stdout, io.TextIOWrapper):
    class _flushstdout(type(sys.stdout)):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, **kwargs)

            self.stdout = args[0]

            for attr in dir(self.stdout):
                try:
                    setattr(self, attr, getattr(self.stdout, attr))
                except:
                    pass

            self.write = self._flush_write

        def _flush_write(self, *args):
            self.stdout.write(*args)
            self.stdout.flush()

    sys.stdout = _flushstdout(sys.stdout)
