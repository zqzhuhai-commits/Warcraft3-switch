#!/usr/bin/env python3
"""Build the real Warcraft III runtime as one libnx application, with native saves."""
import argparse, importlib.util, os, pathlib, shutil, subprocess, json
ROOT = pathlib.Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('web_build', ROOT/'port/web/build-engine.py')
web = importlib.util.module_from_spec(spec)
spec.loader.exec_module(web)
p = argparse.ArgumentParser()
p.add_argument('--host', action='store_true', help='build static Linux executable for integration tests')
p.add_argument('--debug', action='store_true')
p.add_argument('--out', default='build/switch')
a = p.parse_args()
os.chdir(ROOT)
out = (ROOT/a.out).resolve(); out.mkdir(parents=True, exist_ok=True)
dkp = pathlib.Path(os.environ.get('DEVKITPRO', '/opt/devkitpro'))
os.environ.setdefault('DEVKITPRO', str(dkp))
cc = 'gcc' if a.host else str(dkp/'devkitA64/bin/aarch64-none-elf-gcc')
wc3='games/warcraft-3'
sources=web.sources
modules={
 'shared':sources('shared',exclude=('test.c',))+[ROOT/'vendor/blast/blast.c'],
 'test_harness':[ROOT/'shared/test.c'],
 'jass':sources(wc3+'/jass'),
 'renderer':sources('renderer',wc3+'/renderer'),
 'game':sources(wc3+'/game'),
 'menu':sources(wc3+'/menu'),
 'wc3_common':sources(wc3+'/common',exclude=('world_w3.c',)),
 'engine':sources('client','server','common','sound',exclude=('stb_vorbis.c','sv_routing.c'))+[ROOT/wc3/'common/world_w3.c'],
}
flags=['-O0' if a.debug else '-O2','-g','-fno-common','-ffunction-sections','-fdata-sections','-D_GNU_SOURCE','-DSDL_MAIN_HANDLED',
 '-I.','-Ishared','-Ishared/types','-I'+wc3,'-I'+wc3+'/common','-I'+wc3+'/game','-I'+wc3+'/game/api','-I'+wc3+'/game/skills',
 '-DWC3','-DUSE_FOGOFWAR','-DBZ_GAME="warcraft-3"','-DBZ_GL_ES3','-DBZ_MSAA_SAMPLES=0','-DBZ_COMPAT_WORKAROUNDS=1']
if a.host:
 flags+=subprocess.check_output(['pkg-config','--cflags','sdl2','glesv2'],text=True).split()
else:
 flags+=['-fPIE','-march=armv8-a+crc+crypto','-mtune=cortex-a57','-mtp=soft','-D__SWITCH__','-I'+str(dkp/'portlibs/switch/include'),'-I'+str(dkp/'libnx/include')]

def run(args):
 print('+',' '.join(map(str,args)),flush=True)
 subprocess.run(list(map(str,args)),check=True)

def compile(name,files,extra=()):
 unit=out/(name+'_unity.c');web.unity(unit, sorted(set(files)))
 obj=out/(name+'.o')
 # Incremental rebuild uses source mtimes and command identity; headers are included.
 cmd=[cc,*flags,*extra,'-c',unit,'-o',obj]
 stamp=out/(name+'.cmd')
 newest=max(x.stat().st_mtime for x in [*files,*ROOT.rglob('*.h')])
 if not obj.exists() or obj.stat().st_mtime<newest or not stamp.exists() or stamp.read_text()!=repr(cmd):
  run(cmd);stamp.write_text(repr(cmd))
 return obj
objects=[]
for name,files in modules.items():
 extra=[]
 if name in ('game','menu','engine'):extra+=['-DSTB_FDF_IMPLEMENTATION','-DSTB_FDF_GLOBALS']
 if name=='engine':
  extra+=['-DBZ_CLIENT_WORLD']
  if not a.host:extra+=['-Dmain=OpenRealm_Main']
 if name in ('game','menu'):extra+=['-include','port/web/src/web_fdf_namespace.h','-DBZ_WEB_FDF_'+name.upper()]
 if name=='renderer':extra+=['-include','port/web/src/web_renderer_namespace.h']
 objects.append(compile(name,files,extra))
for name in ('parser','sheet'):objects.append(compile(name,[ROOT/wc3/'sheet'/(name+'.c')]))
if a.host:
 run([cc,*objects,'-Wl,--gc-sections',*subprocess.check_output(['pkg-config','--libs','sdl2','glesv2'],text=True).split(),'-lz','-lm','-o',out/'openwarcraft3'])
else:
 # Isolate libnx headers from the engine's legacy aliases.
 objects.append(compile('platform',[ROOT/'port/switch/platform.c']))
 objects.append(compile('touch',[ROOT/'port/switch/touch.c']))
 objects.append(compile('filesystem',[ROOT/'port/switch/fs.c']))
 run([cc,'-fPIE','-specs='+str(dkp/'libnx/switch.specs'),'-march=armv8-a+crc+crypto','-mtp=soft',*objects,
  '-Wl,--gc-sections','-Wl,-Map,'+str(out/'openrealm.map'),
  '-L'+str(dkp/'portlibs/switch/lib'),'-L'+str(dkp/'libnx/lib'),'-lSDL2','-lGLESv2','-lEGL','-lglapi','-ldrm_nouveau','-lstdc++','-lnx','-lz','-lm','-o',out/'openrealm.elf'])
 tools=dkp/'tools/bin'
 romfs=out/'romfs';shutil.copytree(ROOT/'share',romfs/'share',dirs_exist_ok=True)
 shutil.copytree(ROOT/wc3/'share',romfs/'share/warcraft-3',dirs_exist_ok=True)
 run([tools/'nacptool','--create','魔兽争霸3冰封王座','小帆','0.1.2',out/'control.nacp','--titleid=0100F34C524D0000'])
 # Older nacptool versions fill only the first 12 language records.
 nacp=out/'control.nacp';metadata=bytearray(nacp.read_bytes())
 for language in range(16):metadata[language*0x300:(language+1)*0x300]=metadata[:0x300]
 metadata[0x302c:0x3030]=(0xffff).to_bytes(4,'little')
 nacp.write_bytes(metadata)
 run([tools/'elf2nro',out/'openrealm.elf',out/'openrealm.nro','--nacp='+str(out/'control.nacp'),'--icon='+str(ROOT/'port/switch/icon.jpg'),'--romfsdir='+str(romfs)])
 package=out/'package'
 (package/'exefs').mkdir(parents=True,exist_ok=True);(package/'control').mkdir(exist_ok=True)
 run([tools/'elf2nso',out/'openrealm.elf',package/'exefs/main'])
 run([tools/'npdmtool',ROOT/'port/switch/npdm.json',package/'exefs/main.npdm'])
 shutil.copy2(out/'control.nacp',package/'control/control.nacp')
 for language in ('AmericanEnglish','Japanese','French','German','Italian','Spanish','Chinese','Korean','Dutch','Portuguese','Russian','Taiwanese','BritishEnglish','CanadianFrench','LatinAmericanSpanish','SimplifiedChinese','TraditionalChinese'):
  shutil.copy2(ROOT/'port/switch/icon.jpg',package/('control/icon_'+language+'.dat'))
 shutil.copytree(romfs,package/'romfs',dirs_exist_ok=True)
 (out/'build-info.json').write_text(json.dumps({'upstream':'8853d069c21c94a981121066e1d78e967676fae4','hardwareTested':False,'retailDataIncluded':False},indent=2))
