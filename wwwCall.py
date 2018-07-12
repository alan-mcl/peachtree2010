#!/usr/bin/env python
"""
	wwwCall Module v0.1
	Copyright (C) 2007 - Romain Gaucher - http://rgaucher.info
	Handle:
		- http/https
		- Cookies
		- Authorization (Basic, Digest)
	Installation: None, put it in your folder!
	Utilization:
		example:
			# create the object
			http = wwwCall('http://rgaucher.info')
			# add the features you want (cookies,auth)
			http.setCookieFile('./the_path/file.cookie')
			# reaching a logging URL and saving the cookie
			http.post("http://rgaucher.info/login.php",{'username' : 'foo', 'password' : 'bar'})
			# register the username/password for the basic authentification
			http.setAuthBasic("romain","mypassword")
			# print the content of the protected page
			print http.get("http://rgaucher.info/401protected").read()

	The functions get/post return a handle like urllib2.open
"""
import os.path
import cookielib,urllib2,urllib
"""
	http = wwwCall("http://root.url.com")
	http.setCookieFile(cookieFile)
	http.setAuthBasic(username, pass)
	http.setAuthDigest(username, pass)
	http.get ("http://root.url.com/site/?plop=foo")
	http.get ("http://root.url.com/site/","plop=foo")
	http.post("http://root.url.com/site/?plop=foo")
	http.post("http://root.url.com/site/","plop=foo")
"""
class wwwCall(object):
	def __init__(self, url = "http://default.for.passwd.manager"):
		self.rootUrl = url
		self.cookie,self.cookieJar,self.cookieFile = None,cookielib.LWPCookieJar(),"site.cookie"
		self.authBasic,	self.authDigest = False, False
		self.passMgr = urllib2.HTTPPasswordMgrWithDefaultRealm()
		self.urlopen = urllib2.urlopen
		self.request = urllib2.Request
		self.opener  =  None
		self.useProxy = False
		self.proxy = {}
		self.referer = "http://asdf.com/?q=you_have_been_wwwCalled!"
		self.header  = {'User-agent' : 'wwwCall/0.1 (X11; U; Linux i686; en-US; rv:1.7)', 'Referer' : self.referer}
	def setHeader(self, Name = "wwwCall", Version = "0.1"):
		self.header  = {'User-agent' : Name + '/'+ Version +' (X11; U; Linux i686; en-US; rv:1.7)', 'Referer' : self.referer}
	def setReferer(self, strReferer = "http://asdf.com/?q=you_have_been_wwwCalled!"):
		self.referer = strReferer
	# Core setting
	def _register(self):
		handlers = []
		build_openerArgs = None
		if self.cookie:
			if os.path.isfile(self.cookieFile):
				self.cookieJar.load(self.cookieFile)
				handlers.append(urllib2.HTTPCookieProcessor(self.cookieJar))
		if self.authBasic:
			handlers.append(urllib2.HTTPBasicAuthHandler(self.passMgr))
		if self.authBasic:
			handlers.append(urllib2.HTTPDigestAuthHandler(self.passMgr))
		if self.useProxy:
			handlers.append(urllib2.ProxyHandler({'http' : self.proxy['url']}))
		if len(handlers) > 0:
			self.opener = urllib2.build_opener(handlers[0])
			for a in range(1,len(handlers)):
				self.opener.add_handler(handlers[a])
			urllib2.install_opener(self.opener)
	# Configuration Setters Interface
	def setRootURL(self, rootURL):
		self.rootUrl = rootURL
	def setTimeout(self, timeoutValue = 30):
		import socket
		socket.setdefaulttimeout(timeoutValue)
	def setCookieFile(self, fileName):
		self.cookie = True
		self.cookieFile = fileName
	def setAuthBasic(self,username, password):
		self.authBasic = True
		givenRootUrl = self.rootUrl.replace('http://','')
		self.passMgr.add_password(None, givenRootUrl, username, password)
	def setAuthDigest(self,username, password):
		self.authDigest = True
		givenRootUrl = self.rootUrl.replace('http://','')
		self.passMgr.add_password(None, givenRootUrl, username, password)
	def setProxy(self, proxy, username = None, password = None):
		self.useProxy = True
		self.proxy['url'] = proxy
		if username:
			self.proxy['username'] = username
		if password:
			self.proxy['password'] = password
	# switch on/off the information (ability to run without cookies and came back with after)
	def disableProxy(self):
		self.useProxy = False
	def disableCookie(self):
		self.cookie = False
	def disableAuthBasic(self):
		self.authBasic = False
	def disableAuthDigest(self):
		self.authDigest = False
	def enableProxy(self):
		if len(self.proxy) > 0:
			self.useProxy = True
	def enableCookie(self):
		self.cookie = True
	def enableAuthBasic(self):
		self.authBasic = True
	def enableAuthDigest(self):
		self.authDigest = True
	# HTTP GET call
	def get(self,url, data = None):
		self._register()
		if data != None:
			url = url + "?" + (data)
		try:
			req = self.request(url, None, self.header)
			handle = self.urlopen(req)
			self.cookieJar.save(self.cookieFile)
			return handle
		except IOError, e:
			if hasattr(e, 'code'):
				return ('error', e.code)
			else:
				return None
		return None
	# HTTP POST call
	def post(self,url,data=None):
		self._register()
		if data != None:
			data = urllib.urlencode(data)
		try:
			req = self.request(url, data, self.header)
			handle = self.urlopen(req)
			self.cookieJar.save(self.cookieFile)
			return handle
		except IOError, e:
			if hasattr(e, 'code'):
				return ('error', e.code)
			else:
				return None
		return None
