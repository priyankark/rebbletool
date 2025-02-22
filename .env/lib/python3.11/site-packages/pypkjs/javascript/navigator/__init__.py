from __future__ import absolute_import
__author__ = 'katharine'

from .geolocation import Geolocation

class Navigator(object):
    def __init__(self, runtime):

        self._runtime = runtime
        self._runtime = runtime
        self.geolocation = Geolocation(runtime)
        
        runtime.register_syscall('navigator.geolocation.getCurrentPosition', self.geolocation.getCurrentPosition)
        runtime.register_syscall('navigator.geolocation.watchPosition', self.geolocation.watchPosition)
        runtime.register_syscall('navigator.geolocation.clearWatch', self.geolocation.clearWatch)
        

        runtime.run_js("""
            navigator = new (function() {
                this.language = 'en-GB';
                this.geolocation = new (function() {
                    this.getCurrentPosition = get_syscall_func('navigator.geolocation.getCurrentPosition');
                    this.watchPosition = get_syscall_func('navigator.geolocation.watchPosition');
                    this.clearWatch = get_syscall_func('navigator.geolocation.clearWatch');
                })();
            })();
        """)

