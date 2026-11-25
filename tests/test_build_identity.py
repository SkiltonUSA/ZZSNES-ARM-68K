import hashlib,importlib.util,json,tempfile,subprocess,sys
from pathlib import Path
r=Path(__file__).resolve().parents[1];spec=importlib.util.spec_from_file_location('identity',r/'scripts/build_identity.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
with tempfile.TemporaryDirectory() as tmp:
 root=Path(tmp).resolve();out=root/'candidate';out.mkdir();(out/'player').mkdir();(out/'player/code.c').write_text('new input');(out/'player/core.a').write_bytes(b'library');(out/'App').write_bytes(b'new build')
 record=m.record_build(root,out,'Test','2.0',['App'],[['cc',str(out/'player/code.c')]])
 assert record['artifacts']['App']==hashlib.sha256(b'new build').hexdigest()
 assert record['version']=='2.0' and record['output']=='candidate'
 assert set(record['staged_inputs'])=={'player/code.c','player/core.a'}
 assert record['commands']==[['cc','<repo>/candidate/player/code.c']]
 (out/'build.json').unlink()
 try:m.record_build(root,out,'Test','2.0',['missing'],[])
 except FileNotFoundError:pass
 else:raise AssertionError('Missing artifact recorded as success')
 assert not (out/'build.json').exists()
print('PASS build identities: actual artifact/source/library hashes, relative paths, missing output refusal')

with tempfile.TemporaryDirectory(dir=r/'.context',prefix='identity-test-') as tmp:
    output=Path(tmp);marker=output/'keep';marker.write_bytes(b'recorded build')
    result=subprocess.run([sys.executable,str(r/'scripts/build_current.py'),'--output-dir',str(output)],capture_output=True,text=True)
    assert result.returncode==2 and 'new directory' in result.stderr
    assert marker.read_bytes()==b'recorded build' and list(output.iterdir())==[marker]
print('PASS fresh-output guard: recorded build directory refused without mutation')
