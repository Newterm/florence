import gtk.gdk
from xml.dom import minidom
import os
import re
import cgi

class symbol:
	def __init__(self, ref, name):
		self.ref = ref
		self.name = name
		self.hasLabel = False
	
	def matches(self, name):
		if name and self.name:
			return re.compile( "^%s$" % self.name).match(name)
		else:
			return False

	def setlabel(self, label):
		self.label = label
		self.hasLabel = True

	def __str__(self):
		if self.hasLabel:
			return "<text>%s</text>" % cgi.escape(self.label)
		else:
			return "<symbol>%s</symbol>" % cgi.escape(self.ref)

source = "/usr/share/florence/styles/default/symbols.xml"
if os.getenv("DATADIR"):
	source = "%s/florence/styles/default/symbols.xml" % os.getenv("DATADIR")
elif os.getenv("PREFIX"):
	source = "%s/share/florence/styles/default/symbols.xml" % os.getenv("PREFIX")
xmldoc = minidom.parse(source)
list = xmldoc.getElementsByTagName('symbol')

print '<?xml version="1.0" encoding="UTF-8"?>'
print '<keymap>'

print '   <symbols>'
i = 0
syms = []
for node in list:
	try:
		name = node.getElementsByTagName('name')[0].firstChild.data
	except Exception:
		name = None
	sym = symbol("ref" + str(i), name)
	try:
		sym.setlabel( node.getElementsByTagName('label')[0].firstChild.data )
	except Exception:
		if name:
			print '      <symbol id="ref%d">%s</symbol>' % ( i, name )
	syms.append( sym )
	i = i + 1
print '   </symbols>'

print '   <keys>'
kmap = gtk.gdk.keymap_get_default()
for i in range(256):
	keymap = kmap.lookup_key(i,0,0)
	val = gtk.gdk.keyval_name(keymap)
	if val and val[:5] == "dead_":
		if val == "dead_circumflex":
			val = "^"
		else:
			val = val[5:]
	found = False
	for s in syms:
		if s.matches( val ):
			val = str( s )
			found = True
			break
	if not(found):
		if gtk.gdk.keyval_to_unicode(keymap):
			val = "<text>%s</text>" % cgi.escape(unichr(gtk.gdk.keyval_to_unicode(keymap)))
		elif val:
#			val = "<text>%s</text>" % cgi.escape(val)
			val = "<text/>"
	if val:
		print '      <key code="%d">%s</key>' % ( i, val )
print '   </keys>'

print '</keymap>'
