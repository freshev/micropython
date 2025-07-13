import sys
import machine
import filesys
import os
import time
import settings

def a2s(array):
        if('encode' in dir('')): return array.decode('utf-8') #python >= 2.7
        else: return array #python <= 1.5.2
#def s2a(str):
#        if('encode' in dir('')): return str.encode('utf-8') #python >= 2.7
#        else: return str #python <= 1.5.2

def check_key_nv(var, key, notvalue): # check key existance and value != notvalue
    return ((('has_key' in dir(var)) and var.has_key(key)) or (('has_key' not in dir(var)) and key in var)) and var[key] != notvalue

def create_version(file, ver, debug = 0, force = 0):
    if file not in os.listdir() or force == 1:
        try:
            if(debug): filesys.log('Create ' + file + ' ', end='')
            cr_version = filesys.open(file, 'w')
            cr_version.write(ver)
            cr_version.close()
            if(debug): filesys.log('ok')
        except:
            if(debug): filesys.log('nok')


def request_version(host, project, query, to_uncompress = 0, timeout=5, debug=0):
    import urequests
    rv = '0'
    ctry = 0
    while(1):
        if ctry == 10: break
        try:
            resp = urequests.get('%s/%s/version?query=%s' % (host, project, query), timeout=timeout)
            resp_code = resp.status_code
            if to_uncompress == 0 or check_key_nv(resp.headers, 'Content-Encoding', 'deflate'):
                resp_text = resp.content
            else:
                import deflate
                import io
                resp_text = deflate.DeflateIO(io.BytesIO(resp.content), deflate.ZLIB).read()
            resp.close()
            if resp_code == 200: rv = a2s(resp_text).strip(); break
            else:
                if(debug): filesys.log('%s/%s/version nok' % (host, project))
        except:
            type, ex, traceback = sys.exc_info()
            if(debug): filesys.log('Exc RV: %s' % str(ex))
            ctry = ctry + 1
    return rv

def check_version(host, project, query, to_uncompress = 0, timeout=5, debug=0):
    pv = project + '.version'
    cv = ''
    try:
        create_version(pv, 'v0', debug)
        if pv in os.listdir():
            cvf = filesys.open(pv, 'r')
            cv = cvf.readline().strip()
            cvf.close()
        if(debug): filesys.log('Local %s: %s' % (project, cv))
        rv = request_version(host, project, query, to_uncompress, timeout, debug)
        return cv != rv, rv
    except:
        type, ex, traceback = sys.exc_info()
        if(debug): filesys.log('Exc CV: %s' % str(ex))
        return 0, cv

def ota_update(host, project, filenames, query, update=0, use_version_prefix=1, to_uncompress=0, user=None, passwd=None, debug=0, timeout=5):
    wd = machine.WDT(0,-1)
    list = os.listdir()
    for filename in filenames:
        if filename not in list and (filename + 'o') not in list:
            if(debug): filesys.log('Force %s' % filename)
            update = 1

    if int(update) != 0:
        update = 1
        res = 0

        if settings.gprs_on(debug=debug):
            if(debug): filesys.log('Get OTA %s' % project)
            all_found = 1
            if use_version_prefix: separator = '_'
            else: separator = '/'

            import urequests
            try:
                if project == 'Device' and os.uname().sysname != 'Telit':
                    rv = request_version(host, 'Device_FW', query, to_uncompress, timeout, debug)
                    if rv != os.uname().release:
                        res = machine.OTA(rv, '?query=%s' % query)
                        if res == 1:
                            filesys.log('FOTA ok')
                            for i in range(1, 60): wd.feed(); time.sleep(60)
                        else: filesys.log('FOTA nok')
                    wd.feed()

                changed, rv = check_version(host, project, query, to_uncompress, timeout, debug)
                if changed: 
                    for fn in filenames: 
                        if fn in list: os.remove(fn)
                        if (fn + 'o') in list: os.remove(fn + 'o')                        
                wd.feed()
                if(debug): filesys.log('Remote %s: %s' % (project, rv))

                list = os.listdir()
                for filename in filenames:
                    if filename not in list and (filename + 'o') not in list:
                        url = '%s/%s/%s%s%s?query=%s' % (host, project, rv, separator, filename, query)
                        if(debug): filesys.log('Get %s ' % url , end = '')
                        resp = urequests.get(url, timeout=timeout)
                        resp_code = resp.status_code
                        wd.feed()

                        if to_uncompress == 0 or check_key_nv(resp.headers, 'Content-Encoding', 'deflate'):
                            resp_text = resp.content
                        else:
                            import deflate
                            import io
                            resp_text = deflate.DeflateIO(io.BytesIO(resp.content), deflate.ZLIB).read()
                        resp.close()
                        wd.feed()

                        if resp_code != 200:
                            all_found = 0
                            if(debug): filesys.log('nok')
                            continue
                        if(debug): filesys.log('ok')
                        create_version('tmp_%s' % filename, resp_text, debug, 1)
                        wd.feed()

                file_version = project + '.version'
                all_ok = 1
                if all_found:
                    for filename in filenames:
                        if filename not in list and (filename + 'o') not in list:
                            if(debug): filesys.log('Ren tmp_%s to %s ' % (filename, filename), end = '')
                            try:
                                os.rename('tmp_%s' % filename, filename)
                                if(debug): filesys.log('ok')
                                changed = 1
                            except:
                                all_ok = 0
                                if(debug): filesys.log('nok')

                    if changed == 1:
                        os.remove(file_version)
                        create_version(file_version, rv, debug)
                        machine.reset()
                else: os.remove(file_version)
                if all_ok == 1: res = 1

            except:
                type, ex, traceback = sys.exc_info()
                if(debug): filesys.log('Exc UOTA: %s' % str(ex))

            if(debug): filesys.log('OTA ' + project + ' done')
            wd.feed()
        return res
    else: return 0
