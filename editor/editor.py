#!/usr/bin/python
import sys
from xml.dom import minidom
import gtk
import toolkit

class key(toolkit.object):
	def __init__(self, code, x, y, w, h):
		toolkit.object.__init__( self, code, int((x-(w/2.0))*30.0), int((y-(h/2.0))*30.0), int(w*30.0), int(h*30.0) )

	def __str__(self):
		w = self.w / 30.0
		h = self.h / 30.0
		str = "<key>\n"
		str = str + "\t<code>%s</code>\n" % self.name
		str = str + "\t<xpos>%s</xpos>\n" % ( ( self.x + (self.w/2.0) ) / 30.0 )
		str = str + "\t<ypos>%s</ypos>\n" % ( ( self.y + (self.h/2.0) ) / 30.0 )
		if w != 2:
			str = str + "\t<width>%s</width>\n" % w
		if h != 2:
			str = str + "\t<height>%s</height>\n" % h
		str = str + "</key>\n"
		return str

class keyboard(toolkit.scene):
	def __init__(self, file, widget):
		xmldoc = minidom.parse(file)
		keyboard = xmldoc.getElementsByTagName('keyboard')[0]
		w = keyboard.getElementsByTagName('width')[0].firstChild.data
		h = keyboard.getElementsByTagName('height')[0].firstChild.data
		w = int ( float(w) * 30.0 )
		h = int ( float(h) * 30.0 )
		toolkit.scene.__init__( self, widget, w, h )
		keys = keyboard.getElementsByTagName('key')
		for k in keys:
			codes = k.getElementsByTagName('code')
			if len(codes) > 0:
				code = codes[0].firstChild.data
			else:
				code = k.getElementsByTagName('action')[0].getElementsByTagName('command')[0].firstChild.data
			x = float(k.getElementsByTagName('xpos')[0].firstChild.data)
			y = float(k.getElementsByTagName('ypos')[0].firstChild.data)
			try:
				w = float(k.getElementsByTagName('width')[0].firstChild.data)
			except IndexError:
				w = 2
			try:
				h = float(k.getElementsByTagName('height')[0].firstChild.data)
			except IndexError:
				h = 2
			self.add( key(code, x, y, w, h) )

	def __str__(self):
		str = "<keyboard>\n"
		for k in self.objects:
			str = str + k.__str__()
		str = str + "</keyboard>\n"
		return str

def gg( widget, kbd ):
	print kbd
	gtk.main_quit()


window = gtk.Window( gtk.WINDOW_TOPLEVEL )
window.set_title( "Editor" )
window.set_default_size( 320, 200 )
area = gtk.DrawingArea()
area.show()
window.add(area)
kbd = keyboard( sys.argv[1], area )
kbd.setgrid( 30, 30 )
window.connect( "destroy", gg, kbd )

window.show_all()
gtk.main()
