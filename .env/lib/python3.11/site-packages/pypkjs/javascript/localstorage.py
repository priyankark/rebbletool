from __future__ import absolute_import
__author__ = 'katharine'

import STPyV8 as v8
import errno
import logging
import os
import os.path
import dbm as dumbdbm  # This is the only one that actually syncs data if the process dies before I can close().
logger = logging.getLogger("pypkjs.javascript.localstorage")

_storage_cache = {}  # This is used when filesystem-based storage is unavailable.


class LocalStorage(object):
    def __init__(self, runtime, persist_dir=None):
        self.storage = None
        if persist_dir is not None and False:  # TODO: enable persistence
            try:
                try:
                    os.makedirs(os.path.join(persist_dir, 'localstorage'))
                except OSError as e:
                    if e.errno != errno.EEXIST:
                        raise

                self.storage = dumbdbm.open(os.path.join(persist_dir, 'localstorage', str(runtime.pbw.uuid)), 'c')
            except IOError:
                pass
        if self.storage is None:
            logger.warning("Using transient store.")
            self.storage = _storage_cache.setdefault(str(runtime.pbw.uuid), {})

        runtime.register_syscall("localStorage.set", self.set)
        runtime.register_syscall("localStorage.get", self.get)
        runtime.register_syscall("localStorage.has", self.has)
        runtime.register_syscall("localStorage.delete", self.delete_)
        runtime.register_syscall("localStorage.keys", self.keys)
        runtime.register_syscall("localStorage.enumerate", self.enumerate)
        runtime.register_syscall("localStorage.clear", self.clear)
        runtime.register_syscall("localStorage.getItem", self.getItem)
        runtime.register_syscall("localStorage.setItem", self.setItem)
        runtime.register_syscall("localStorage.removeItem", self.removeItem)
        runtime.register_syscall("localStorage.key", self.key)
        
        runtime.run_js("""
        localStorage = new (function() {
            this.set = get_syscall_func('localStorage.set');
            this.get = get_syscall_func('localStorage.get');
            this.has = get_syscall_func('localStorage.has');
            this.delete = get_syscall_func('localStorage.delete');
            this.delete_ = get_syscall_func('localStorage.delete_');
            this.keys = get_syscall_func('localStorage.keys');
            this.enumerate = get_syscall_func('localStorage.enumerate');
            this.clear = get_syscall_func('localStorage.clear');
            this.getItem = get_syscall_func('localStorage.getItem'); 
            this.setItem = get_syscall_func('localStorage.setItem');
            this.removeItem = get_syscall_func('localStorage.removeItem');
        })();
        """)

    def get(self, p, name):
        return self.storage.get(str(name), v8.JSNull())

    def set(self, p, name, value):
        self.storage[str(name)] = str(value)
        return True

    def has(self, p, name):
        return name in self.storage

    def delete_(self, p, name):
        if name in self.storage:
            del self.storage[name]
            return True
        else:
            return False

    def keys(self, p):
        return v8.JSArray(self.storage.keys())

    def enumerate(self):
        return v8.JSArray(self.storage.keys())

    def clear(self, *args):
        self.storage.clear()

    def getItem(self, name, *args):
        return self.get(None, name)

    def setItem(self, name, value, *args):
        self.set(None, name, value)

    def removeItem(self, name, *args):
        return self.delete_(None, name)

    def key(self, index, *args):
        if len(self.storage) > index:
            return self.storage.keys()[index]
        else:
            return v8.JSNull()

    def _shutdown(self):
        if hasattr(self.storage, 'close'):
            self.storage.close()
