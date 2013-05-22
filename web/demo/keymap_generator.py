import gtk.gdk
from xml.dom import minidom
import os
import re
import cgi
import os.path

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
			return "%s" % cgi.escape(self.label)
		else:
			return "<style>%s</style>" % cgi.escape(self.ref)

if os.path.isfile("symbols.xml"):
	source = "symbols.xml"
else:
	source = "/usr/share/florence/styles/default/symbols.xml"
if os.getenv("DATADIR"):
	source = "%s/florence/styles/default/symbols.xml" % os.getenv("DATADIR")
elif os.getenv("PREFIX"):
	source = "%s/share/florence/styles/default/symbols.xml" % os.getenv("PREFIX")
xmldoc = minidom.parse(source)
list = xmldoc.getElementsByTagName('symbol')

def get_modifier_map():
	output = os.popen('xmodmap -pm')
	next(output)
	next(output)
	mods = []
	for line in output:
		mod = " ".join(line.split()[1:]).split(",")
		mod = [ m.split()[0] for m in mod if m != '' ]
		mods.append(mod)
	return mods[:8]
mods = get_modifier_map()

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

kmap = gtk.gdk.keymap_get_default()
for i in range(256):
	keymaps = []
	symbols = ""
	mod = 0
	locker = False
	for j in range(256):
		keymap = kmap.translate_keyboard_state(i,j,0)
		if keymap == None:
			continue
		keymap = keymap[0] 
		if keymap in [ k[1] for k in keymaps ]:
			cont = False
			dup = [ k for k in keymaps if k[1]==keymap ]
			for d in dup:
				if (d[0] & j) >= d[0]:
					cont = True
			if cont: continue
		keymaps.append((j,keymap))

		value = None
		val = gtk.gdk.keyval_name(keymap)

		e = 1
		for m in mods:
			if val in m:
				mod=e
				if val.endswith('_Lock'): locker=True
				break
			e = e << 1
			
		if val and val[:5] == "dead_":
			if val == "dead_circumflex":
				symbols += '        <symbol mod="%d">^</symbol>\n' % (j)
				continue
			else:
				keymap = gtk.gdk.keyval_from_name(val[5:])

		ks = ""
		if val in ['BackSpace', 'Return', 'Tab', 'ISO_Left_Tab', 'Left', 'Right', 'Up', 'Down']:
			ks = ' sym="%s"' % (val)
		found = False
		for s in syms:
			if s.matches( val ):
				symbols += '        <symbol mod="%d"%s>%s</symbol>\n' % (j, ks, str( s ))
				found = True
				break
		if not(found):
			if gtk.gdk.keyval_to_unicode(keymap):
				symbols += '        <symbol mod="%d"%s>%s</symbol>\n' % (j, ks, cgi.escape(unichr(gtk.gdk.keyval_to_unicode(keymap))))
			elif val:
				symbols += '        <symbol mod="%d"%s><name>%s</name></symbol>\n' % (j, ks, cgi.escape(val))

	if len(keymaps) > 0:
		if mod == 0: mod=""
		else: mod=' mod="%d"' % (mod)
		if locker: locker=' locker="yes"'
		else: locker=""
		print '    <key code="%d"%s%s>\n%s    </key>' % ( i, mod, locker, symbols )

print '</keymap>'
