import builtins

class file:
    def __init__(self, filename, flag, mode):
        self.filename = filename
        self.flag = flag
        self.mode = mode
        self.f = builtins.open(self.filename, self.flag, self.mode)
    def read(self, count=0):
        if(self.f is not None):
            if(count > 0): return self.f.read(count)
            else: return self.f.read()
        else: raise RuntimeError('File is not opened')
    def readline(self):
        if(self.f is not None): return self.f.readline()
        else: raise RuntimeError('File is not opened')
    def write(self, str):
        if(self.f is not None): return self.f.write(str)
        else: raise RuntimeError('File is not opened')
    def close(self):
        if(self.f is not None):
            self.f.close()
            self.f = None
        else: raise RuntimeError('File is not opened')
    def flush(self):
        if(self.f is not None): return self.f.flush()
        else: raise RuntimeError('File is not opened')
    def tell(self):
        return self.f.tell()

def open(filename, flag, mode=777):
    f = file(filename, flag, mode)
    return f

_log_content_ = ''

def log(s, end='\n'):
    global _log_content_
    builtins.print(s, end=end)    
    if len(_log_content_) > 1024: _log_content_ = _log_content_[len(_log_content_) - 1024:]
    _log_content_ = _log_content_ + str(s) + end

def getLog():
    global _log_content_
    return _log_content_
def clearLog():
    global _log_content_
    _log_content_ = ''


