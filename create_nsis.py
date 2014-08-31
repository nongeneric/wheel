from subprocess import check_output, check_call
import os

def get_full_path(dll):
	all = check_output(['where', dll]).strip().split()
	filtered = filter(lambda p: "mingw-w64" in p, all)
	if len(filtered) == 0:
		return None
	return filtered[0]

def get_deps(dll):
    return check_output("objdump -p " + dll + " | awk '/DLL Name/ { print $3 }'", shell=True).strip().split()

known = set()
def get_relevant_deps(exe):
    if exe in known:
        return []
    print exe
    deps = map(get_full_path, get_deps(exe))
    deps = filter(lambda p: p is not None, deps)
    known.add(exe)
    for d in deps:
        deps.extend(get_relevant_deps(d))
    return deps
	
template = 'installer.nsi'
patched = 'installer_patched.nsi'

deps = get_relevant_deps("desktop.exe")
deps.extend(['desktop.exe', 'cube.ply', 'config.xml', 'lang.en.xml', 'lang.ru.xml', 'LiberationSans-Regular.ttf'])
files_list = map(lambda d: "  File \"{}\"\n".format(d), deps)
files_str = reduce(lambda l, r: l + r, files_list)

deps.remove('config.xml')
deps = map(lambda d: os.path.split(d)[1], deps)
delete_list = map(lambda d: "  Delete \"$INSTDIR\\{}\"\n".format(d), deps)
delete_str = reduce(lambda l, r: l + r, delete_list)

body = open(template, 'r').read()
body = body.replace('{files}', files_str)
body = body.replace('{delete}', delete_str)
open(patched, 'w').write(body)
check_call(['makensis', 'installer_patched.nsi'])