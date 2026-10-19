#!/usr/bin/env python3
"""Original synthetic checks for the opt-in expanded ZZSNES build."""
import os,subprocess
from pathlib import Path
from compatibility_native import build
ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'development/expanded-20261019';O=ROOT/'.context/expanded-features'
def main():
 env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
 exe=build(E/'tests/features.c',O,fx3=True,expanded=True)
 subprocess.run([exe],check=True,env=env)
 exe=build(E/'tests/options.c',O,fx3=True,expanded=True)
 subprocess.run([exe],check=True,env=env)
 exe=build(ROOT/'tests/amiga/pocketsnes/fx3.c',O,fx3=True,expanded=True)
 subprocess.run([exe],check=True,env=env)
 subprocess.run(['cc','-std=c99','-O2','-g','-fsanitize=address,undefined','-I'+str(E/'source'),'-I'+str(E/'common'),'-I'+str(ROOT/'development/borders-20261016/source'),str(E/'tests/preferences.c'),'-o',str(O/'preferences')],check=True)
 subprocess.run([O/'preferences'],check=True,env=env)
 subprocess.run(['cc','-std=c99','-O2','-g','-fsanitize=address,undefined','-I'+str(E/'common'),str(E/'tests/option_preferences.c'),'-o',str(O/'option-preferences')],check=True)
 subprocess.run([O/'option-preferences'],check=True,env=env)
if __name__=='__main__':main()
