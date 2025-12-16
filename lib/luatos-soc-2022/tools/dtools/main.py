#!/usr/bin/python3
#-*- coding: UTF-8 -*-

##Libraries that need to be used
import os, struct, sys, logging, subprocess, shutil, hashlib, requests
import uuid, json

#web related
import bottle
from bottle import request, post, static_file, response, get, abort, HTTPResponse

#monitoring related
from prometheus_client import Summary, Counter
import prometheus_client
REQUEST_TIME = Summary('request_processing_seconds', 'Time spent processing request')
DIFF_TIME = Summary('diff_processing_seconds', 'Time spent processing binpkg diff')

#log
logging.basicConfig(level=logging.DEBUG)

#Tencent Cloud COS integration
cos_client = None
cos_bucket = None

#Original difference file
@DIFF_TIME.time()
def diff_org(old_path, new_path, dst_path, tmpdir, chip="ec618"):
    #Remove target files, defensive
    if os.path.exists(dst_path):
        os.remove(dst_path)
    #Calculate SHA1 and compare
    old_sha1 = hashlib.sha1()
    new_sha1 = hashlib.sha1()
    if os.path.exists(os.path.join(tmpdir, "org.diff")) :
        os.remove(os.path.join(tmpdir, "org.diff"))
    with open(old_path, "rb") as f :
        old_sha1.update(f.read())
    with open(new_path, "rb") as f :
        new_sha1.update(f.read())
    #If Tencent Cloud COS information exists, try to obtain existing results.
    cos_path = "ec618/v1/origin/{}/{}.bin".format(old_sha1.hexdigest(), new_sha1.hexdigest())
    if cos_client :
        if cos_client.object_exists(cos_bucket, cos_path) :
            #Very good, I have done the difference before, just return the result directly
            cos_client.download_file(cos_bucket, cos_path, dst_path)
            shutil.copy(dst_path, os.path.join(tmpdir, "org.diff"))
            return True

    #Start executing the differential process
    cmd = []
    if os.name != "nt":
        cmd.append("wine")
    cmd.append("FotaToolkit.exe")
    cmd.append("-d")
    if os.name != "nt":
        cmd.append("config/{}.json".format(chip))
    else:
        cmd.append("config\\{}.json".format(chip))
    cmd.append("BINPKG")
    cmd.append(dst_path)
    cmd.append(old_path)
    cmd.append(new_path)
    #print(" ".join(cmd))
    #print(" tmpdir ", tmpdir)
    subprocess.check_call(" ".join(cmd), shell=True, cwd=tmpdir)
    shutil.copy(dst_path, os.path.join(tmpdir, "org.diff"))
    if cos_client :
        #Upload differential results to COS
        cos_client.upload_file(cos_bucket, cos_path, dst_path)
    return True

#QAT firmware is relatively simple, original differential file
def diff_qat(old_path, new_path, dst_path, tmpdir):
    diff_org(old_path, new_path, dst_path, tmpdir)

#For Hezhou AT firmware, you need to add a header
def diff_at(old_path, new_path, dst_path, tmpdir):
    #First generate an original differential file
    diff_org(old_path, new_path, dst_path, tmpdir)
    #Then read the data
    with open(dst_path, "rb") as f:
        data = f.read()
    #Add header and generate final file
    with open(dst_path, "wb") as f:
        #Write header
        head = struct.pack(">bIbI", 0x7E, 0x01, 0x7D, len(data))
        f.write(head)
        #Write raw data
        f.write(data)

#CSDK is also raw differential
def diff_csdk(old_path, new_path, dst_path, tmpdir):
    diff_org(old_path, new_path, dst_path, tmpdir)
    

#Difference of LuatOS files
def diff_soc(old_path, new_path, dst_path, cwd="."):
    tmpdir = os.path.join(os.path.abspath(cwd), "soctmp")
    if os.path.exists(tmpdir) :
        shutil.rmtree(tmpdir)
    os.makedirs(tmpdir)
    def tmpp(path) :
        return os.path.join(tmpdir, path)
    
    import py7zr, json
    old_param = None
    old_binpkg = None
    #old_script = None
    new_param = None
    new_binpkg = None
    new_script = None
    with py7zr.SevenZipFile(old_path, 'r') as zip:
        for fname, bio in zip.readall().items():
            if str(fname).endswith("info.json") :
                fdata = bio.read()
                old_param = json.loads(fdata.decode('utf-8'))
            if str(fname).endswith(".binpkg") :
                old_binpkg = bio.read()
            #if str(fname).endswith("script.bin") :
            #old_script = bio.read()
    with py7zr.SevenZipFile(new_path, 'r') as zip:
        for fname, bio in zip.readall().items():
            if str(fname).endswith("info.json") :
                fdata = bio.read()
                new_param = json.loads(fdata.decode('utf-8'))
            if str(fname).endswith(".binpkg") :
                new_binpkg = bio.read()
            if str(fname).endswith("script.bin") :
                new_script = bio.read()
    if not old_param or not old_binpkg:
        logging.warn("The old version is not SOC firmware!!")
        return
    if not new_param or not new_binpkg or not new_script:
        logging.warn("The new version is not SOC firmware!!")
        return
    script_only = new_binpkg == old_binpkg
    if script_only :
        with open(tmpp("delta.par"), "wb+") as f :
            pass
    else:
        with open(tmpp("old2.binpkg"), "wb+") as f :
            f.write(old_binpkg)
        with open(tmpp("new2.binpkg"), "wb+") as f :
            f.write(new_binpkg)
        diff_org(tmpp("old2.binpkg"), tmpp("new2.binpkg"), tmpp("delta.par"), cwd)
    
    with open(tmpp("script.bin"), "wb+") as f :
        f.write(new_script)
    #First package script.bin
    #cmd = "{} zip_file {} {} \"{}\" \"{}\" {} 1".format(str(soc_exe_path),
    # new_param['fota']['magic_num'], 
    # new_param["download"]["script_addr"], 
    #str(new_path.with_name(new_param["script"]["file"])), str(new_path.with_name("script_fota.zip")), str(new_param['fota']['block_len']))
    cmd = []
    if os.name != "nt":
        cmd.append("wine")
    cmd.append("soc_tools.exe")
    cmd.append("zip_file")
    cmd.append(str(new_param['fota']['magic_num']))
    cmd.append(str(new_param["download"]["script_addr"]))
    cmd.append(tmpp("script.bin"))
    cmd.append(tmpp("script_fota.zip"))
    cmd.append(str(new_param['fota']['block_len']))
    subprocess.check_call(" ".join(cmd), shell=True, cwd=cwd)

    ## Then package the overall differential package
    #cmd = "{} make_ota_file {} 0 0 0 0 0 \"{}\" \"{}\" \"{}\"".format(str(soc_exe_path),
    # new_param['fota']['magic_num'], 
    # str(new_path.with_name("script_fota.zip")), 
    # str(exe_path.with_name("delta.par")), 
    #str(Path(out_path, "output.sota")))
    cmd = []
    if os.name != "nt":
        cmd.append("wine")
    cmd.append("soc_tools.exe")
    cmd.append("make_ota_file")
    cmd.append(str(new_param['fota']['magic_num']))
    cmd.append("0")
    cmd.append("0")
    cmd.append("0")
    cmd.append("0")
    cmd.append("0")
    cmd.append(tmpp("script_fota.zip"))
    cmd.append(tmpp("delta.par"))
    cmd.append(tmpp("output.sota"))
    subprocess.check_call(" ".join(cmd), shell=True, cwd=cwd)

    shutil.copy(tmpp("output.sota"), dst_path)
    logging.info("done soc diff")

"""
Full upgrade
"""
def do_full(old_path, new_path, dst_path, cwd="."):
    #Create temporary directory
    tmpdir = os.path.join(os.path.abspath(cwd), "fulltmp")
    if os.path.exists(tmpdir) :
        shutil.rmtree(tmpdir)
    os.makedirs(tmpdir)

    #Unpack the binpkg file
    cmd = []
    if os.name != "nt":
        cmd.append("wine")
    cmd.append(os.path.join("dep","fcelf.exe"))
    cmd.append("-E")
    cmd.append("-input")
    cmd.append(new_path)
    cmd.append("-dir")
    cmd.append(tmpdir)
    cmd.append("-info")
    cmd.append(os.path.join(tmpdir, "imageinfo.json"))
    subprocess.check_call(" ".join(cmd), shell=True, cwd=cwd)

    #Read imgeinfo.json
    with open(os.path.join(tmpdir, "imageinfo.json"), "r") as f :
        imageinfo = json.loads(f.read())
    bin_data = bytes()
    for ii in imageinfo["imageinfo"] :
        if ii["type"] == "AP":
            pass
            #cmd = "{} zip_file {} {} \"{}\" \"{}\" {} 1".format(str(soc_exe_path), "eaf18c16", "00024000", str(work_path.with_name(ap_name)), str(work_path.with_name("ap.zip")), "40000")
            cmd = []
            if os.name != "nt":
                cmd.append("wine")
            cmd.append("soc_tools.exe")
            cmd.append("zip_file")
            cmd.append("eaf18c16")
            cmd.append("00024000")
            cmd.append(os.path.join(tmpdir, ii["file"]))
            cmd.append(os.path.join(tmpdir, "ap.zip"))
            cmd.append("40000")
            subprocess.check_call(" ".join(cmd), shell=True, cwd=cwd)
            with open(os.path.join(tmpdir, "ap.zip"), "rb") as f :
                bin_data += f.read()
        elif ii["type"] == "CP":
            #cmd = "{} zip_file {} {} \"{}\" \"{}\" {} 1".format(str(soc_exe_path), "eaf18c16", "80000000", str(work_path.with_name("cp-demo-flash.bin")), str(work_path.with_name("cp.zip")), "40000")
            cmd = []
            if os.name != "nt":
                cmd.append("wine")
            cmd.append("soc_tools.exe")
            cmd.append("zip_file")
            cmd.append("eaf18c16")
            cmd.append("80000000")
            cmd.append(os.path.join(tmpdir, ii["file"]))
            cmd.append(os.path.join(tmpdir, "cp.zip"))
            cmd.append("40000")
            subprocess.check_call(" ".join(cmd), shell=True, cwd=cwd)
            with open(os.path.join(tmpdir, "cp.zip"), "rb") as f :
                bin_data += f.read()
    with open(os.path.join(tmpdir, "total.zip"), "wb+") as f :
        f.write(bin_data)

    #cmd = "{} make_ota_file {} 4294967295 0 0 0 {} \"{}\" \"{}\" \"{}\"".format(str(soc_exe_path), "eaf18c16", str(hex(version)), str(work_path.with_name("total.zip")), DUMMPY_BIN_PATH, str(Path(out_path, "{}_{}_{}.sota".format(file_name, version, file_suffix))))
    cmd = []
    if os.name != "nt":
        cmd.append("wine")
    cmd.append("soc_tools.exe")
    cmd.append("make_ota_file")
    cmd.append("eaf18c16")
    cmd.append("4294967295")
    cmd.append("0")
    cmd.append("0")
    cmd.append("0")
    cmd.append("0x113a695d")
    cmd.append(os.path.join(tmpdir, "total.zip"))
    with open(os.path.join(tmpdir, "dummy.bin"), "w+") as f :
        pass
    cmd.append(os.path.join(tmpdir, "dummy.bin"))
    cmd.append(os.path.join(tmpdir, "out.sota"))

    logging.info(" ".join(cmd))
    subprocess.check_call(" ".join(cmd), shell=True, cwd=cwd)
    shutil.copy(os.path.join(tmpdir, "out.sota"), dst_path)
    
    #soc_tools.exe make_ota_file eaf18c16 4294967295 0 0 0 0x113a695d
    #soc_tools.exe make_ota_file eaf18c16 4294967295 0 0 0 0x113a695d
    logging.info("Complete the production of full upgrade update package")

def do_mode(mode, old_path, new_path, dst_path, is_web, tmpdir=".") :

    tmpdir = os.path.abspath(tmpdir)
    #Execute according to different modes
    if mode == "org":
        logging.info("Perform raw difference")
        diff_org(old_path, new_path, dst_path, tmpdir)
    elif mode == "qat" :
        logging.info("Perform QAT difference")
        diff_qat(old_path, new_path, dst_path, tmpdir)
    elif mode == "at" :
        logging.info("Perform AT differential")
        diff_at(old_path, new_path, dst_path, tmpdir)
    elif mode == "csdk" :
        logging.info("Perform CSDK difference")
        diff_csdk(old_path, new_path, dst_path, tmpdir)
    elif mode == "soc" :
        logging.info("Perform LuatOS differential")
        diff_soc(old_path, new_path, dst_path, tmpdir)
    elif mode == "full" :
        logging.info("Perform full upgrade")
        do_full(old_path, new_path, dst_path, tmpdir)
    else:
        print("未知模式, 未支持" + mode)
        if not is_web :
            sys.exit(1)

#---------------------------------------------------------------------------
#---------------------------------------------------------------------------
def http_api_work(mode, tmpdir=None):
    old_path = os.path.join(tmpdir, "old.binpkg")
    new_path = os.path.join(tmpdir, "new.binpkg")
    dst_path = os.path.join(tmpdir, "diff.bin")
    if os.path.exists(dst_path):
        os.remove(dst_path)
    if os.path.exists(old_path) :
        os.remove(old_path)
    if os.path.exists(new_path) :
        os.remove(new_path)
    logging.info("old " + old_path)
    logging.info("new " + new_path)
    logging.info("dst " + dst_path)

    #Read old and new firmware
    if "oldurl" in request.params and  len(request.params["oldurl"]) > 10 :
        #URL download method
        oldurl = request.params["oldurl"]
        resp = requests.request("GET", oldurl)
        if resp.status_code != 200 :
            logging.warn("URL cannot be accessed" + oldurl)
            return None, "老版本URL无法访问"
        with open(old_path, "wb") as f :
            f.write(resp.content)
    elif "old" in request.files :
        #local file mode
        oldBinbkg = request.files.get("old")
        oldBinbkg.save(old_path, overwrite=True)
    else :
        logging.warn("At least provide old or oldurl parameters")
        return None, "起码要提供old或者oldurl参数"
    if "newurl" in request.params and  len(request.params["newurl"]) > 10 :
        newurl = request.params["newurl"]
        resp = requests.request("GET", newurl)
        if resp.status_code != 200 :
            logging.warn("URL cannot be accessed" + newurl)
            return None, "新版本URL无法访问"
        with open(new_path, "wb") as f :
            f.write(resp.content)
    elif "new" in request.files :
        newBinbkg = request.files.get("new")
        newBinbkg.save(new_path, overwrite=True)
    else :
        logging.warn("At least provide new or newurl parameters")
        return None, "起码要提供new或者newurl参数"
    if "mode" in request.params :
        mode = request.params["mode"]
    do_mode(mode, old_path, new_path, dst_path, True, tmpdir=tmpdir)
    return True, None

@REQUEST_TIME.time()
@post("/api/diff/<mode>")
def http_api(mode):
    #First, create a temporary directory
    tmpdir = "/work/" + str(uuid.uuid4()) + "/"
    data = None
    result = None
    msg = None
    headers = {}
    try :
        if os.path.exists(tmpdir) :
            shutil.rmtree(tmpdir)
        shutil.copytree(os.path.abspath("."), tmpdir)
        os.chmod(tmpdir, 0o777)
        logging.info(" ".join(os.listdir(tmpdir)))
        result, msg = http_api_work(mode, tmpdir)
        if os.path.exists(os.path.join(tmpdir, "diff.bin")):
            with open(os.path.join(tmpdir, "diff.bin"), "rb") as f :
                data = f.read()
        if os.path.exists(os.path.join(tmpdir, "org.diff")) :
            fstat = os.stat(os.path.join(tmpdir, "org.diff"))
            #response.add_header("X-ECFOTA-FW-DIFF-SIZE", str(fstat.st_size))
            headers["X-Delta-Size"] = str(fstat.st_size)
    except:
        import traceback
        traceback.print_exc()
    #print("Execution completed")
    if os.path.exists(tmpdir) :
        shutil.rmtree(tmpdir)
    #print("judgment result", result)
    if result and data :
        headers["Content-Length"] = str(len(data))
        headers["Content-Type"] = "application/octet-stream"
        headers['Content-Disposition'] = 'attachment; filename="diff.bin"'
        return HTTPResponse(body=data, headers=headers)
    else :
        return abort(text=(msg or "Server Error"))

@get("/")
def index_page():
    return static_file("index.html", root=".")

@get("/heartcheck")
def heartcheck():
    return HTTPResponse("ok")

@get("/metrics")
def get_metrics():
    return HTTPResponse(prometheus_client.generate_latest())

def start_web():
    bottle.run(host="0.0.0.0", port=9000, server="cheroot")

#------------------------------------------------------------------------------------------

def main():
    if len(sys.argv) < 2 :
        return
    if "COS_ENABLE" in os.environ and os.environ["COS_ENABLE"] == "1":
        from qcloud_cos import CosConfig
        from qcloud_cos import CosS3Client
        secret_id = os.environ['COS_SECRET_ID']
        secret_key = os.environ['COS_SECRET_KEY']
        region = os.environ['COS_SECRET_REGION']
        token = None
        scheme = 'https'
        config = CosConfig(Region=region, SecretId=secret_id, SecretKey=secret_key, Token=token, Scheme=scheme)
        global cos_client
        global cos_bucket
        cos_client = CosS3Client(config)
        cos_bucket = os.environ['COS_SECRET_BUCKET']
        logging.info("Enable Tencent COS support")
    mode = sys.argv[1]
    if mode == "web" :
        start_web()
        return
    if len(sys.argv) < 5 :
        logging.info("requires mode old version path new version path target output file path")
        logging.info("示例: python main.py qat old.binpkg new.binpkg diff.bin")
        logging.info("Optional modes are: at qat csdk org soc")
        sys.exit(1)
    do_mode(mode, sys.argv[2], sys.argv[3], sys.argv[4], False)

if __name__ == "__main__":
    main()
