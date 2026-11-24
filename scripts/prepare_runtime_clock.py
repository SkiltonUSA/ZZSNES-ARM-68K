"""Restore the released 666/800 MHz host calibration policy in fresh builds.
Guest clocks and PCM synthesis rates are unchanged. This does not overclock.
"""
def prepare(stage):
    path = stage / 'host.c'
    text = path.read_text()
    old = 'if(rate<320000000u||rate>345000000u)goto done;'
    new = 'if(!((rate>=320000000u&&rate<=345000000u)||(rate>=396000000u&&rate<=404000000u))){printf("Unsupported ARM timer: %u Hz\\n",rate);goto done;}'
    if text.count(old) != 1:
        raise ValueError('Expected the original host calibration gate exactly once')
    path.write_text(text.replace(old, new))
