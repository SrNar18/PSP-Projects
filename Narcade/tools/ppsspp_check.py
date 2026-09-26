"""Small PPSSPP debugger client for reproducible local smoke checks.

Start portable PPSSPP with --debugger=19321. No game memory is changed.
"""
import base64,json,pathlib,sys,time
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'build/python-deps'))
import websocket

class PSP:
    def __init__(self,port=19321):
        self.ws=websocket.create_connection(f'ws://127.0.0.1:{port}/debugger',subprotocols=['debugger.ppsspp.org'],timeout=15)
        self.ticket=0
    def call(self,event,**kwargs):
        self.ticket+=1
        self.ws.send(json.dumps(dict(event=event,ticket=self.ticket,**kwargs)))
        while True:
            r=json.loads(self.ws.recv())
            if r.get('ticket')==self.ticket:
                if r['event']=='error':raise RuntimeError(r)
                return r
    def tap(self,button,duration=3):
        self.call('input.buttons.press',button=button,duration=duration);time.sleep(.18)
    def screenshot(self,name):
        status=self.call('cpu.status')
        print('CPU',status)
        if not status.get('stepping'):
            self.ws.send(json.dumps({'event':'cpu.stepping'}))
            while json.loads(self.ws.recv()).get('event')!='cpu.stepping':pass
        try:
            try:r=self.call('gpu.buffer.screenshot')
            except RuntimeError:r=self.call('gpu.buffer.renderColor')
            path=ROOT/'build/emulator'/f'{name}.png';path.parent.mkdir(parents=True,exist_ok=True)
            path.write_bytes(base64.b64decode(r['uri'].split(',')[1]));print(path)
        finally:
            self.ws.send(json.dumps({'event':'cpu.resume'}))
            while json.loads(self.ws.recv()).get('event')!='cpu.resume':pass

if __name__=='__main__':
    p=PSP();print(p.call('version',name='Narcade QA',version='1.0'))
    if len(sys.argv)>1 and sys.argv[1]=='status':
        print(p.call('cpu.status'));print(p.call('hle.thread.list'))
    elif len(sys.argv)>1 and sys.argv[1]=='input':
        p.tap(sys.argv[2],int(sys.argv[3]) if len(sys.argv)>3 else 12)
    elif len(sys.argv)>1 and sys.argv[1]=='start':
        p.screenshot('01-title');p.tap('cross');time.sleep(1);p.screenshot('02-story');p.tap('cross');time.sleep(1);p.screenshot('03-world')
    elif len(sys.argv)>1 and sys.argv[1]=='tap':
        p.tap(sys.argv[2],int(sys.argv[3]) if len(sys.argv)>3 else 3);p.screenshot('latest')
    # v2.9: 'hold up,cross 2.0' mantiene varios botones a la vez durante N segundos (input.buttons.send)
    elif len(sys.argv)>3 and sys.argv[1]=='hold':
        names=sys.argv[2].split(',');secs=float(sys.argv[3])
        p.call('input.buttons.send',buttons={n:True for n in names});time.sleep(secs)
        p.call('input.buttons.send',buttons={n:False for n in names})
    else:p.screenshot(sys.argv[1] if len(sys.argv)>1 else 'latest')
