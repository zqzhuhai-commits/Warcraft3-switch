#!/usr/bin/env python3
"""Package a native build using a private, local keyset; never echo key/tool output."""
import os, argparse, hashlib, pathlib, shutil, subprocess, tempfile
p = argparse.ArgumentParser()
p.add_argument('--build', type=pathlib.Path, default=pathlib.Path('build/switch'))
p.add_argument('--keyset', type=pathlib.Path, required=True)
p.add_argument('--tool', default='hacbrewpack')
p.add_argument('--out', type=pathlib.Path, default=pathlib.Path('build/switch/nsp'))
a = p.parse_args()
base = a.build.resolve() / 'package'
for name, magic in [('exefs/main', b'NSO0'), ('exefs/main.npdm', b'META')]:
    path = base / name
    if not path.is_file() or path.read_bytes()[:4] != magic:
        p.error('Missing or invalid native build input: ' + name)
if not (base/'control/control.nacp').is_file() or not (base/'romfs/share/warcraft-3/config.cfg').is_file():
    p.error('Control or embedded engine defaults are missing')
if not a.keyset.is_file():
    p.error('The private local keyset does not exist')
a.out.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='openrealm-nsp-') as tmp:
    temp = pathlib.Path(tmp)
    cmd = [a.tool, '--keyset', str(a.keyset.resolve()), '--exefsdir', str(base/'exefs'),
           '--romfsdir', str(base/'romfs'), '--controldir', str(base/'control'), '--nologo',
           '--nspdir', str(temp/'nsp'), '--ncadir', str(temp/'nca'),
           '--tempdir', str(temp/'work'), '--backupdir', str(temp/'backup')]
    env = dict(os.environ, HACBREWPACK_TITLE_VERSION=str(7 << 16))
    result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)
    files = list((temp/'nsp').glob('*.nsp'))
    if result.returncode or len(files) != 1:
        raise SystemExit('Packaging failed. Check local tools/keyset; tool output is withheld to protect key material.')
    target = a.out/'魔兽争霸3冰封王座-Switch-0.1.7.nsp'
    shutil.copyfile(files[0], target)
    print(str(target))
    print('SHA256:', hashlib.sha256(target.read_bytes()).hexdigest())
    print('Packaging complete. Switch hardware behavior is unverified.')
