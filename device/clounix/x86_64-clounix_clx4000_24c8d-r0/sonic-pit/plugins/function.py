import subprocess

def run_command(cmd):
    proc = subprocess.Popen(
        cmd,
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE)
    out, err = proc.communicate()
    if err and proc.returncode != 0:
        return proc.returncode, err
    out = out.decode('utf-8')
    return 0, out.rstrip('\n')