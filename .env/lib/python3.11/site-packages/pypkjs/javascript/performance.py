from __future__ import absolute_import
__author__ = 'katharine'

import time

class Performance(object):
    # This is an approximation for now
    def __init__(self, runtime):
        runtime.register_syscall("performance.now", lambda: time.time())
        runtime.run_js("""
             performance = new (function() {
                this.now = get_syscall_func('performance.now');
            })();
        """)