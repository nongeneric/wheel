from subprocess import check_output, check_call
import sys

rev = "tip"
if len(sys.argv) > 1:
    rev = sys.argv[1]

dirname = "wheel-" + rev
archive = dirname + ".tar.bz2"
tmpdir = "/tmp/" + dirname
check_call(["rm", "-rf", tmpdir])
check_call(["hg", "clone", "-u", rev, ".", tmpdir])
check_call(["rm", "-rf", tmpdir + "/.hg"])
check_call(["rm", "-rf", tmpdir + "/.hgignore"])
check_call(["tar", "cjf", archive, dirname], cwd="/tmp")
print("/tmp/" + archive)
