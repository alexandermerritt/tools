#! /usr/bin/env python

import os
import sys
import urllib2
import xml.etree.ElementTree as et
import math

# ------------------- PUT YOUR INFO HERE -------------------
API_KEY = ''
API_SECRET = ''

seedtag = 'social'
nimages = 500
perpage = 500
# ------------------- PUT YOUR INFO HERE -------------------

HOST = 'https://api.flickr.com'
API = '/services/rest'

msearch  = 'flickr.photos.search'
msizes   = 'flickr.photos.getSizes'
mcluster = 'flickr.tags.getClusters'

# fetch a raw URL
def geturl(urlstr):
    url = urllib2.urlopen(urlstr)
    page = url.read()
    url.close()
    return page

# query Flickr based on given function, returns page
def getpage(method, options):
    urlstr = '{}{}?method={}'.format(HOST, API, method)
    urlstr += '&api_key={}'.format(API_KEY)
    for opt in options.keys():
        urlstr += '&{}={}'.format(opt, options[opt])
    page = geturl(urlstr)
    rsp = et.fromstring(page)
    if rsp.attrib['stat'] == 'fail':
        msg = rsp[0].attrib['msg']
        raise Exception('Flickr: ' + msg)
    return page

# returns list of tags Flickr thinks are related
def getcluster(tag):
    tags = []
    options = { 'tag' : tag }
    page = getpage(mcluster, options)
    clusters = et.fromstring(page)[0]
    for t in clusters[0]: # just first one for now
        tags.append(t.text)
    return tags

def skipimage(etimage):
    attrib = etimage.attrib
    if 'herosjourneymythology' in attrib['owner']: # sketchy photos
        return True
    if '48312507@N02' in attrib['owner']:
        return True
    # add others
    return False

tags = getcluster(seedtag)
tagopt = ','.join(tags[0:20])
print(tagopt)

npages = int(math.ceil(float(nimages) / perpage))
for pagen in range(1,npages+1):
    options = { 'tags'          : tagopt,
                'safe_search'   : '1',
                'media'         : 'photos',
                'per_page'      : perpage,
                'page'          : pagen,
            }
    photos = et.fromstring(getpage(msearch, options))[0]
    for image in photos:
        if skipimage(image):
            continue
        imgid = image.attrib['id']
        options = { 'photo_id' : imgid } 
        #print(image.attrib)
        page = getpage(msizes, options)
        sizes = et.fromstring(page)[0]
        #size = sizes[-1].attrib # largest
        size = sizes[0].attrib # smallest
        print('{}: {}x{} {}'.format(nimages, size['width'], size['height'], imgid))
        fname = imgid + '.jpg'
        if os.path.exists(fname):
            continue
        img = geturl(size['source'])
        with open(fname, 'w') as imgf:
            imgf.write(img)
        nimages -= 1
        if nimages <= 0:
            break

