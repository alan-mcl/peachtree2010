
import sys
import urllib
import urllib2
import cookielib
import os.path
import urllib2

COOKIEFILE = 'cookies.lwp'

cj = None
urlopen = urllib2.urlopen
Request = urllib2.Request
cj = cookielib.LWPCookieJar()    
    
if os.path.isfile(COOKIEFILE):
	# if we have a cookie file already saved
	# then load the cookies into the Cookie Jar
	cj.load(COOKIEFILE)


auth_handler = urllib2.HTTPPasswordMgrWithDefaultRealm()
auth_handler.add_password(realm=None,
									uri='http://www.testrugby.com/s1408',
									user='peachtree',
									passwd='6640baycircle')

# if we use cookielib
# then we get the HTTPCookieProcessor
# and install the opener in urllib2
opener = urllib2.build_opener(urllib2.HTTPCookieProcessor(cj), urllib2.HTTPBasicAuthHandler(auth_handler))
urllib2.install_opener(opener)
        

theurl = 'http://www.testrugby.com/s1408'
# an example url that sets a cookie,
# try different urls here and see the cookie collection you can make !

#txdata = urllib.urlencode({"login":"peachtree", "password":"6640baycircle"})
txdata = None
# if we were making a POST type request,
# we could encode a dictionary of values here,
# using urllib.urlencode(somedict)

txheaders =  {'User-agent' : 'Mozilla/4.0 (compatible; MSIE 5.5; Windows NT)'}
# fake a user agent, some websites (like google) don't like automated exploration

try:
	req = Request(theurl, txdata, txheaders)
	# create a request object

	handle = urlopen(req)
	
	print handle.read()
	# and open it to return a handle on the url

except IOError, e:
	print 'We failed to open "%s".' % theurl
	if hasattr(e, 'code'):
		print 'We failed with error code - %s.' % e.code
	elif hasattr(e, 'reason'):
		print "The error object has the following 'reason' attribute :"
		print e.reason
		print "This usually means the server doesn't exist,"
		print "is down, or we don't have an internet connection."
	sys.exit()

else:
	print 'Here are the headers of the page :'
	print handle.info()
	# handle.read() returns the page
	# handle.geturl() returns the true url of the page fetched
	# (in case urlopen has followed any redirects, which it sometimes does)

print
print 'These are the cookies we have received so far :'
for index, cookie in enumerate(cj):
	print index, '  :  ', cookie
	cj.save(COOKIEFILE)

       
############################

'''
# Create an OpenerDirector with support for Basic HTTP Authentication...
auth_handler = urllib2.HTTPPasswordMgrWithDefaultRealm()
auth_handler.add_password(realm=None,
                          uri='http://www.testrugby.com/s1408/situation.asp',
                          user='Clan McLachlan',
                          passwd='Parmenion')
opener = urllib2.build_opener(auth_handler)
# ...and install it globally so it can be used with urlopen.
urllib2.install_opener(opener)

res = urllib2.urlopen("http://www.testrugby.com/s1408/situation.asp")
page = res.read()

print page
'''