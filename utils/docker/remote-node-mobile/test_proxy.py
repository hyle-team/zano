#!/usr/bin/env python3
"""Exercise the real nginx runtime against a disposable local backend."""
import concurrent.futures
import http.client
import json
from pathlib import Path
import subprocess
import tempfile
import time
import uuid

ROOT = Path(__file__).resolve().parents[3]
IMAGE = "zano-remote-node:proxy-test"
SUFFIX = uuid.uuid4().hex[:10]
BACKEND = "zano-proxy-backend-" + SUFFIX
PROXY = "zano-proxy-" + SUFFIX

def docker(*args, check=True):
    return subprocess.run(["docker", *args], check=check, text=True, capture_output=True).stdout.strip()

BACKEND_SOURCE = """
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json,time
class Handler(BaseHTTPRequestHandler):
    def log_message(self,*args): pass
    def do_GET(self):
        if self.headers.get('X-Test-Hold'): time.sleep(8)
        data=json.dumps({'path':self.path,'forwarded':self.headers.get('X-Forwarded-For'),'real':self.headers.get('X-Real-IP')}).encode()
        self.send_response(200);self.send_header('Content-Length',str(len(data)));self.end_headers()
        try:self.wfile.write(data)
        except BrokenPipeError:pass
    do_POST=do_GET
ThreadingHTTPServer(('127.0.0.1',11211),Handler).serve_forever()
"""

def main():
    build = subprocess.run(["docker", "build", "--target", "runtime-base", "-f",
        "utils/docker/remote-node-mobile/Dockerfile", "-t", IMAGE, "."], cwd=ROOT)
    if build.returncode: raise SystemExit(build.returncode)
    with tempfile.TemporaryDirectory(prefix="zano-proxy-test-") as tmp:
        fixture=Path(tmp)
        (fixture/"backend.py").write_text(BACKEND_SOURCE)
        (fixture/"general_info").write_text("fixture")
        (fixture/"config.json").write_text("{\"fixture\":true}")
        try:
            docker("run","-d","--name",BACKEND,"--network","bridge",
                "-v",str(fixture/"backend.py")+":/backend.py:ro",
                "python:3.12-alpine","python3","/backend.py")
            address=docker("inspect","--format","{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}",BACKEND)
            docker("run","-d","--name",PROXY,"--network","container:"+BACKEND,
                "--cap-drop=ALL","--security-opt","no-new-privileges:true",
                "--read-only","--tmpfs","/tmp:rw,nosuid,nodev,size=64m",
                "-v",str(fixture/"config.json")+":/home/mobile_app_config/general_info/config.json:ro",
                "--entrypoint","/bin/bash",IMAGE,"-c",
                "mkdir -p /tmp/nginx/client /tmp/nginx/proxy; exec nginx -g 'daemon off;'")

            def request(path="/getheight",headers=None,body=None,method="GET"):
                c=http.client.HTTPConnection(address,33340,timeout=12)
                try:
                    c.request(method,path,body=body,headers=headers or {})
                    r=c.getresponse(); data=r.read()
                    return r.status,data,dict(r.getheaders())
                finally:c.close()
            for _ in range(50):
                try:
                    if request()[0]==200:break
                except OSError:pass
                time.sleep(.1)
            else:raise AssertionError("nginx did not start")
            time.sleep(.3)
            count=0
            allowed=["/json_rpc","/getheight","/gettransactions","/sendrawtransaction",
                "/getinfo","/getblocks.bin","/get_o_indexes.bin","/getrandom_outs.bin",
                "/getrandom_outs3.bin","/getrandom_outs4.bin","/get_tx_pool.bin",
                "/check_keyimages.bin","/get_pos_details.bin"]
            for path in allowed:
                status,_,_=request(path,method="POST",body="{}")
                assert status==200,(path,status);count+=1
                time.sleep(.11)
            denied=["/","/unknown","/getheight/suffix","/getheightSuffix",
                "/json_rpc/anything","/getblocks.bin.extra","/getheight%2fsuffix",
                "/set_maintainers_info.bin","/start_mining","/stop_mining","/force_relay",
                "/general_info/other"]
            for path in denied:
                assert request(path)[0]==404,path;count+=1
            assert request("/general_info/config.json?_t=123")[1]==b'{"fixture":true}';count+=1
            assert request("/getheight",method="PUT")[0]==403;count+=1
            assert request("/getheight",method="POST",body=b"x"*(1024*1024+1))[0]==413;count+=1
            status,body,headers=request(headers={"X-Forwarded-For":"203.0.113.7","X-Real-IP":"203.0.113.8"})
            assert status==200,status
            seen=json.loads(body)
            assert seen["forwarded"]!="203.0.113.7" and seen["real"]!="203.0.113.8",seen
            assert "nginx/" not in headers.get("Server","");count+=1

            time.sleep(2.2)
            with concurrent.futures.ThreadPoolExecutor(max_workers=45) as pool:
                futures=[pool.submit(request,headers={"X-Forwarded-For":f"198.51.100.{i+1}"}) for i in range(45)]
                statuses=[f.result()[0] for f in futures]
            assert 429 in statuses and 200 in statuses,statuses
            assert statuses.count(200)<=25,statuses;count+=1

            # Slow requests stay below the request-rate limit but exceed 30 active
            # connections, exercising the independent connection limit.
            time.sleep(2.2)
            with concurrent.futures.ThreadPoolExecutor(max_workers=35) as pool:
                futures=[]
                for i in range(35):
                    futures.append(pool.submit(request,headers={"X-Test-Hold":"1"}))
                    time.sleep(.11)
                statuses=[f.result()[0] for f in futures]
            assert statuses.count(200)==30 and statuses.count(429)==5,statuses;count+=1

            # A narrow trusted-proxy rule changes identity only for that peer.
            peer=json.loads(request()[1])["real"]
            (fixture/"trusted.conf").write_text("set_real_ip_from "+peer+";\n".replace("\\n","\n"))
            docker("rm","-f",PROXY)
            docker("run","-d","--name",PROXY,"--network","container:"+BACKEND,
                "-v",str(fixture/"trusted.conf")+":/etc/nginx/trusted-proxies/test.conf:ro",
                "--entrypoint","/bin/bash",IMAGE,"-c",
                "mkdir -p /tmp/nginx/client /tmp/nginx/proxy; exec nginx -g 'daemon off;'")
            for _ in range(50):
                try:
                    status,body,_=request(headers={"X-Forwarded-For":"203.0.113.9"})
                    if status==200:break
                except OSError:pass
                time.sleep(.1)
            seen=json.loads(body)
            assert seen["real"]=="203.0.113.9" and seen["forwarded"]=="203.0.113.9",seen
            count+=1
            print(json.dumps({"passed":count,"allowedEndpoints":len(allowed),
                "deniedPaths":len(denied),"rateLimit":True,"connectionLimit":True,
                "untrustedHeadersIgnored":True,"trustedProxy":True}))
        finally:
            docker("rm","-f",PROXY,check=False)
            docker("rm","-f",BACKEND,check=False)

if __name__=="__main__":main()
