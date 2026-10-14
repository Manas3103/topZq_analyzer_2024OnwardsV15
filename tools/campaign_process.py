"""Bounded POSIX subprocess capture and process-group cleanup."""
import os
import selectors
import signal
import subprocess
import time


class CommandError(RuntimeError):
    def __init__(self, code, detail, returncode=None, stdout='', stderr=''):
        super().__init__(detail)
        self.code, self.returncode, self.stdout, self.stderr = code, returncode, stdout, stderr


def run_process(args, timeout, cwd=None, env=None, log=None, capture_limit=262144, log_limit=128 * 1024 * 1024):
    if os.name != 'posix' or not 0 < timeout <= 86400: raise ValueError('POSIX and bounded positive timeout required')
    started = time.monotonic(); deadline = started + timeout
    proc = subprocess.Popen(args, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            cwd=cwd, env=env, start_new_session=True)
    selector = selectors.DefaultSelector(); out = bytearray(); err = bytearray(); count = 0
    stream = None
    try:
        if log is not None: stream = open(log, 'xb')
        selector.register(proc.stdout, selectors.EVENT_READ, out)
        selector.register(proc.stderr, selectors.EVENT_READ, err)
        while selector.get_map():
            remaining = deadline - time.monotonic()
            if remaining <= 0: raise CommandError('timeout', 'subprocess wall deadline exceeded')
            for key, _ in selector.select(min(remaining, 0.1)):
                block = os.read(key.fileobj.fileno(), 65536)
                if not block: selector.unregister(key.fileobj); continue
                count += len(block)
                if count > (log_limit if stream else capture_limit): raise CommandError('output_limit', 'subprocess output limit exceeded')
                if stream:
                    stream.write(block)
                    # Bounded tails only; complete processing log has its own limit.
                    key.data.extend(block); del key.data[:-4000]
                else: key.data.extend(block)
        remaining = deadline - time.monotonic()
        if remaining <= 0: raise CommandError('timeout', 'subprocess wall deadline exceeded')
        try: rc = proc.wait(timeout=remaining)
        except subprocess.TimeoutExpired: raise CommandError('timeout', 'subprocess wall deadline exceeded')
        stdout, stderr = out.decode('utf-8', errors='replace'), err.decode('utf-8', errors='replace')
        if rc: raise CommandError('nonzero_exit', f'subprocess exit {rc}', rc, stdout, stderr)
        return subprocess.CompletedProcess(args, rc, stdout, stderr)
    except CommandError as exc:
        exc.stdout = out.decode('utf-8', errors='replace')[-4000:]
        exc.stderr = err.decode('utf-8', errors='replace')[-4000:]
        raise
    finally:
        try: os.killpg(proc.pid, signal.SIGKILL)
        except ProcessLookupError: pass
        try: proc.wait(timeout=2)
        except subprocess.TimeoutExpired: pass
        selector.close(); proc.stdout.close(); proc.stderr.close()
        if stream: stream.close()
